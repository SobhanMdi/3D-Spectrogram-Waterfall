#ifndef DATAWORKER_H
#define DATAWORKER_H

#include <QObject>
#include <QTimer>
#include <vector>
#include <memory>
#include <array>
#include <random> 

class DataWorker : public QObject {
    Q_OBJECT
public:
    explicit DataWorker(int rowSize, QObject* parent = nullptr);
    ~DataWorker();

signals:
    void dataReady(std::shared_ptr<std::vector<float>> rowData);

public slots:
    void startProcessing();
    void stopProcessing();

private slots:
    void generateData();

private:
    int m_rowSize;
    QTimer* m_timer;
    float m_phase;
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_noiseDist;

    std::array<std::shared_ptr<std::vector<float>>, 3> m_bufferPool;
    size_t m_poolIndex;
};

#endif
