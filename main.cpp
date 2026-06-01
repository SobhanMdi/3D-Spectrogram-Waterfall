#include <QApplication>
#include <QSurfaceFormat>
#include <QMainWindow>
#include <QThread>

#include "SpectrogramWidget.h"
#include "DataWorker.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setSwapInterval(1);
    QSurfaceFormat::setDefaultFormat(format);

    QMainWindow mainWindow;
    mainWindow.setWindowTitle("Anti-Glitch 3D Real-time Spectrogram Waterfall");
    mainWindow.resize(1152, 864);

    SpectrogramWidget* spectrogramWidget = new SpectrogramWidget(&mainWindow);
    mainWindow.setCentralWidget(spectrogramWidget);
    mainWindow.show();

    QThread* workerThread = new QThread;

    DataWorker* worker = new DataWorker(SpectrogramWidget::GRID_WIDTH);
    worker->moveToThread(workerThread);

    QObject::connect(workerThread, &QThread::started, worker, &DataWorker::startProcessing);
    QObject::connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    QObject::connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    QObject::connect(worker, &DataWorker::dataReady,
        spectrogramWidget, &SpectrogramWidget::addNewData,
        Qt::QueuedConnection);

    workerThread->start();

    int execReturn = a.exec();

    workerThread->quit();
    workerThread->wait();

    return execReturn;
}
