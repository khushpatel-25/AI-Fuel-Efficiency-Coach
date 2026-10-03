#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFileInfo>
#include <QSaveFile>
#include <QPointer>

#include "llama.h"
#include "common.h"
#include "sampling.h"

#ifndef AICONTROLLER_H
#define AICONTROLLER_H

class AIController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double downloadPercentage READ getDownloadPercentage NOTIFY downloadProgressUpdate)

public:
    explicit AIController(QObject *parent = nullptr);

    double aiDownloadedPercent() const;
    void ensureDownloaded(QNetworkAccessManager* nam,
                          const QUrl& url,
                          const QString& destPath,
                          std::function<void(qint64 received, qint64 total)> onProgress,
                          std::function<void(bool ok, const QString& errorOrEmpty)> onDone);
    QString query_granite(const QString& modelPath, const std::string& prompt);
    void setDownloadProgress(qint64 percent) {
        m_aiDownloadProgress = percent;
        emit downloadProgressUpdate();
    }
    double getDownloadPercentage() {
        return static_cast<double>(m_aiDownloadProgress);
    }
    void setDownloadTotal(qint64 file_size) {
        m_aiDownloadTotal = file_size;
    }

    Q_INVOKABLE double getDownloadTotal() {
        return static_cast<double>(m_aiDownloadTotal);
    }
    void setRootObject(QObject *root) {
        m_rootObject = root;
    }
    void setPageIndex(int index) {
        m_rootObject->setProperty("pageIndex", index);
    }
private:
    mutable qint64 m_aiDownloadProgress = 0.0;
    mutable qint64 m_aiDownloadTotal = 3.0;
    std::vector<llama_token> tokenize_all(const llama_vocab * vocab, const std::string & text);
    QObject *m_rootObject = nullptr;
signals:
    void downloadProgressUpdate();
};

#endif // AICONTROLLER_H
