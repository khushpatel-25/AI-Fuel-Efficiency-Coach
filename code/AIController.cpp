#include "AIController.h"

AIController::AIController(QObject *parent)
    : QObject(parent) {}

double AIController::aiDownloadedPercent() const { return m_aiDownloadProgress; }

/**
 * @brief ensureDownloaded When running this application we need the LLM model to support it, in macOS this is in the Application Support folder and Windows I believe this is
 * %APPDATA%, this will check if the .gguf file exists and if not will make a request to download it from huggingface.
 *
 * @param nam A QNetworkAccessManager instance
 * @param url The huggingface URL
 * @param destPath The path to download to
 * @param onProgress What to do when progress gets made
 * @param onDone What to do when the task is complete.
 */
void AIController::ensureDownloaded(
    QNetworkAccessManager* nam,
    const QUrl& url,
    const QString& destPath,
    std::function<void(qint64 received, qint64 total)> onProgress,
    std::function<void(bool ok, const QString& errorOrEmpty)> onDone
    ) {

    // Checking if the model is already downloaded
    if (QFileInfo::exists(destPath) && QFileInfo(destPath).size() > 0) {
        if (onDone) onDone(true, {});
        return;
    }

    // Make parent dirs
    QDir().mkpath(QFileInfo(destPath).absolutePath());

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = nam->get(req);

    // Write to a temp file and atomically commit at the end
    auto file = new QSaveFile(destPath, reply);
    if (!file->open(QIODevice::WriteOnly)) {
        reply->abort();
        reply->deleteLater();
        if (onDone) onDone(false, "Failed to open destination file for writing: " + destPath);
        return;
    }

    QObject::connect(reply, &QNetworkReply::downloadProgress,
                     reply, [this, onProgress](qint64 received, qint64 total) {
                        this->setDownloadTotal(total);
                        if (onProgress) onProgress(received, total);
                     });

    QObject::connect(reply, &QIODevice::readyRead, reply, [reply, file]() {
        const QByteArray chunk = reply->readAll();
        if (!chunk.isEmpty()) file->write(chunk);
    });

    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, file, onDone]() {
        const auto err = reply->error();
        const QString errStr = reply->errorString();

        // Ensure any remaining bytes are flushedx
        const QByteArray tail = reply->readAll();
        if (!tail.isEmpty()) file->write(tail);

        reply->deleteLater();

        if (err != QNetworkReply::NoError) {
            file->cancelWriting();
            file->deleteLater();
            if (onDone) onDone(false, "Download failed: " + errStr);
            return;
        }

        if (!file->commit()) {
            const QString why = file->errorString();
            file->deleteLater();
            if (onDone) onDone(false, "Failed to commit file: " + why);
            return;
        }

        file->deleteLater();
        if (onDone) onDone(true, {});
    });
}

std::vector<llama_token> AIController::tokenize_all(const llama_vocab * vocab, const std::string & text) {
    int cap = (int)text.size() + 16;
    std::vector<llama_token> out(cap);

    bool add_special = false;
    bool parse_special = true;

    int n = llama_tokenize(vocab,
                           text.c_str(), (int)text.size(),
                           out.data(), (int)out.size(),
                           add_special,
                           true);

    if (n < 0) {
        // if it fails at first we might as well try increasing the buffer size
        out.resize((size_t)(-n));
        n = llama_tokenize(vocab,
                           text.c_str(), (int)text.size(),
                           out.data(), (int)out.size(),
                           true, true);
    }

    if (n < 0) {
        // if it still fails instead of thinking forever and throwing an error it just fails nicely
        qWarning() << "llama_tokenize failed, code:" << n;
        return {};
    }

    out.resize((size_t)n);
    return out;
}

QString AIController::query_granite(const QString& modelPath, const std::string& prompt) {
    llama_backend_init();

    QByteArray pathUtf8 = modelPath.toUtf8();
    llama_model * model = llama_model_load_from_file(pathUtf8.constData(), llama_model_default_params());
    if (!model) { qWarning() << "[LLM] Failed to load model"; return QString("Error 001."); }

    llama_context_params cp = llama_context_default_params();
    cp.n_ctx = 2048;
    cp.n_batch = 512;
    llama_context * ctx = llama_init_from_model(model, cp);
    if (!ctx) { qWarning() << "[LLM] Failed to init context"; return QString("Error 002."); }

    const llama_vocab * vocab = llama_model_get_vocab(model);
    if (!vocab) { qWarning() << "[LLM] Failed to get vocab"; return QString("Error 003."); }

    common_params_sampling sparams;
    sparams.temp = 1.0f;
    sparams.top_p = 0.0f;
    common_sampler * sampler = common_sampler_init(model, sparams);
    qDebug() << "[LLM] sampler ptr =" << (void*)sampler;
    if (!sampler) { qWarning() << "[LLM] Failed to init sampler"; return QString("Error 004."); }
    qDebug() << "promppty: " << prompt;
    auto tok = tokenize_all(vocab, prompt);
    if (tok.empty()) {
        qWarning() << "[LLM] No tokens produced; aborting generation";
        common_sampler_free(sampler);
        llama_free(ctx);
        llama_model_free(model);
        return QString("Error 005.");
    }
    int n = (int)tok.size();

    llama_batch batch = llama_batch_init(n, 0, 1);

    for (int i = 0; i < n; i++) {
        batch.token[i]   = tok[i];
        batch.pos[i]     = i;
        batch.seq_id[i][0] = 0;
        batch.n_seq_id[i]= 1;
        batch.logits[i]  = true;
    }
    batch.n_tokens = n;

    int rc = llama_decode(ctx, batch);
    llama_batch_free(batch);

    if (rc != 0) {
        qWarning() << "[LLM] decode(prompt) failed rc =" << rc;
        common_sampler_free(sampler);
        llama_free(ctx);
        llama_model_free(model);
        return QString("Error 006.");
    }

    int pos = n;

    // Print token text (same as you had)
    QString out;
    out.reserve(4096); // optional
    for (int i = 0; i < 200; i++) {
        llama_token t = common_sampler_sample(sampler, ctx, 0);
        common_sampler_accept(sampler, t, true);

        if (t == llama_vocab_eos(vocab)) {
            break;
        }

        char buf[4096];
        int l = llama_token_to_piece(vocab, t, buf, sizeof(buf), 0, true);


        if (l > 0) {
            out += QString::fromUtf8(buf, l);
        }

        llama_batch b = llama_batch_init(1, 0, 1);
        b.token[0] = t;
        b.pos[0] = pos++;
        b.seq_id[0][0] = 0;
        b.n_seq_id[0] = 1;
        b.logits[0] = true;
        b.n_tokens = 1;

        int rc2 = llama_decode(ctx, b);
        llama_batch_free(b);

        if (rc2 != 0) {
            qWarning() << "[LLM] decode(gen) failed rc =" << rc2;
            break;
        }
    }

    common_sampler_free(sampler);
    llama_free(ctx);
    llama_model_free(model);

    return out;
}
