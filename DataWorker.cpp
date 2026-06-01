#include "DataWorker.h"
#include <cmath>
#include <QRandomGenerator>
#include <QThread>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

DataWorker::DataWorker(int rowSize, QObject* parent)
    : QObject(parent), m_rowSize(rowSize), m_timer(nullptr), m_phase(0.0f), m_poolIndex(0)
{
    for (auto& buf : m_bufferPool) {
        buf = std::make_shared<std::vector<float>>(m_rowSize, 0.0f);
    }

    m_rng.seed(std::random_device{}());
    m_noiseDist = std::uniform_real_distribution<float>(0.0f, 0.08f);
}

DataWorker::~DataWorker() {
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
    }
}

void DataWorker::startProcessing() {
    m_timer = new QTimer(nullptr);
    m_timer->moveToThread(QThread::currentThread());

    connect(m_timer, &QTimer::timeout, this, &DataWorker::generateData, Qt::DirectConnection);
    m_timer->start(16);
}

void DataWorker::stopProcessing() {
    if (m_timer) m_timer->stop();
}

void DataWorker::generateData()
{
    auto activeBuffer = m_bufferPool[m_poolIndex];
    float* rawDataPtr = activeBuffer->data();

    const float normalizeFactor = 10.0f / (m_rowSize - 1);

    for (int i = 0; i < m_rowSize; ++i) {
        float x = (static_cast<float>(i) * normalizeFactor) - 5.0f;

        float peak1 = std::exp(-(x * x) * 2.0f);

        float sineWave = std::sin(x * 5.0f + m_phase) * 0.2f + 0.2f;

        float noise = m_noiseDist(m_rng);

        rawDataPtr[i] = peak1 + sineWave + noise;
    }

    m_phase += 0.05f;
    if (m_phase > 2.0f * M_PI) {
        m_phase -= 2.0f * M_PI;
    }

    emit dataReady(activeBuffer);
    m_poolIndex = (m_poolIndex + 1) % m_bufferPool.size();
}
