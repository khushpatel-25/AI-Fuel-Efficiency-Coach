#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "TripController.h"
#include "AIController.h"
#include <QStandardPaths>
#include <QDir>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFileInfo>
#include <QSaveFile>
#include <QPointer>
#include <QtConcurrent>
#include <functional>
#include <iostream>

#include "llama.h"
#include "common.h"
#include "sampling.h"

void handle_ai(AIController *controller, QGuiApplication *app) {
    QNetworkAccessManager *nman = new QNetworkAccessManager(app);

    QString modelDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/models";

    QString modelPath = modelDir + "/granite-4.0-micro-Q4_K_M.gguf";

    QUrl modelUrl("https://huggingface.co/ibm-granite/granite-4.0-micro-GGUF/resolve/main/granite-4.0-micro-Q4_K_M.gguf?download=true");

    controller->ensureDownloaded(
        nman,
        modelUrl,
        modelPath,
        /* onProgress */ [controller](qint64 received, qint64 total) {
            // Update a QML/Qt progress bar here
            // total may be -1 if server doesn't send content-length
            controller->setDownloadProgress(received);
        },
        /* onDone */ [modelPath, controller](bool ok, const QString& err) {
            if (!ok) {
                qWarning() << "Model download failed:" << err;
                return;
            }

            qDebug() << "Model ready at:" << modelPath;
            QMetaObject::invokeMethod(controller, [modelPath, controller]() {
                controller->setPageIndex(0);
            }, Qt::QueuedConnection);

            try {
                QtConcurrent::run([modelPath, controller]() {
                    try {
                        controller->query_granite(modelPath, "...");
                    } catch (const std::exception &e) {
                        qWarning() << "query_granite exception:" << e.what();
                    }
                });
                return;
            } catch (const std::exception &e) {
                qWarning() << "query_granite exception:" << e.what();
            } catch (...) {
                qWarning() << "query_granite unknown exception";
            }
        });
}

int main(int argc, char *argv[])
{

    QGuiApplication app(argc, argv);
    QQmlApplicationEngine engine;

    auto aiController = std::make_unique<AIController>();
    engine.rootContext()->setContextProperty("aiController", aiController.get());


    TripController tripController;
    tripController.setAIController(aiController.get());
    engine.rootContext()->setContextProperty("tripController", &tripController);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("FuelEfficiencyCoach", "Main");

    QObject *root = engine.rootObjects().first();
    aiController->setRootObject(root);

    handle_ai(aiController.get(), &app);

    if (engine.rootObjects().isEmpty())
        return -1;

    int result = app.exec();
    aiController.reset();  // Explicitly delete before app exits
    return result;
}
