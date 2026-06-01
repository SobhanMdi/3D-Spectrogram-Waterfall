#include "HudOverlay.h"

HudOverlay::HudOverlay(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    mainLayout->setContentsMargins(15, 15, 0, 0);
    mainLayout->setSpacing(0);

    auto* container = new QWidget(this);
    container->setObjectName(QStringLiteral("hudContainer"));
    container->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    container->setMinimumWidth(120);
    container->setStyleSheet(QStringLiteral(
        "QWidget#hudContainer {"
        "    background-color: rgba(10, 15, 25, 200);"
        "    border-left: 2px solid #00ffcc;"
        "    border-radius: 4px;"
        "}"
        "QLabel {"
        "    background: transparent;"
        "    font-family: 'Consolas', 'Courier New', monospace;"
        "}"
    ));

    auto* contentLayout = new QVBoxLayout(container);
    contentLayout->setContentsMargins(8, 6, 12, 6);
    contentLayout->setSpacing(2);

    m_titleLabel = new QLabel(QStringLiteral("3D Waterfall"), container);
    m_titleLabel->setStyleSheet(QStringLiteral(
        "color: #00ffcc;"
        "font-size: 10px;"
        "font-weight: bold;"
        "letter-spacing: 1px;"
        "padding-bottom: 4px;"
    ));
    contentLayout->addWidget(m_titleLabel);

    contentLayout->addWidget(createDataRow(QStringLiteral("FPS"), m_fpsLabel, QStringLiteral("#00ff66")));
    contentLayout->addWidget(createDataRow(QStringLiteral("RES"), m_gridLabel, QStringLiteral("#8892b0")));

    mainLayout->addWidget(container, 0, Qt::AlignTop | Qt::AlignLeft);
    mainLayout->addStretch();
}

QWidget* HudOverlay::createDataRow(const QString& key, QLabel*& valueLabel, const QString& valueColor)
{
    auto* rowWidget = new QWidget();
    auto* layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* keyLabel = new QLabel(key, rowWidget);
    keyLabel->setStyleSheet(QStringLiteral("color: #64f3a6; font-size: 10px; font-weight: bold;"));

    valueLabel = new QLabel(QStringLiteral("0.0"), rowWidget);
    valueLabel->setStyleSheet(QString("color: %1; font-size: 10px; font-weight: bold;").arg(valueColor));

    layout->addWidget(keyLabel);
    layout->addWidget(valueLabel);
    layout->addStretch();

    return rowWidget;
}

void HudOverlay::paintEvent(QPaintEvent*) {}

void HudOverlay::setFps(float fps) {
    m_fpsLabel->setText(QString("%1 Hz").arg(fps, 0, 'f', 1));
}

void HudOverlay::setGridSize(int w, int d) {
    m_gridLabel->setText(QString("%1x%2").arg(w).arg(d));
}
