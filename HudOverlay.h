#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>

class HudOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit HudOverlay(QWidget* parent = nullptr);
    QWidget* createDataRow(const QString& key, QLabel*& valueLabel, const QString& valueColor);
    void setFps(float fps);
    void setGridSize(int w, int d);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QLabel* m_titleLabel;
    QLabel* m_fpsLabel;
    QLabel* m_gridLabel;

    QWidget* makeRow(const QString& key, QLabel*& valueLabel, const QString& valueColor);
};
