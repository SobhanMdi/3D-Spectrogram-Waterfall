#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core> 
#include <QOpenGLBuffer>             
#include <QOpenGLVertexArrayObject>  
#include <QOpenGLShaderProgram>      
#include <QOpenGLTexture>            
#include <QMatrix4x4>                
#include <QPoint>                    
#include <QMouseEvent>               
#include <QWheelEvent>               
#include <QElapsedTimer>
#include <memory> 
#include <vector>

#include "HudOverlay.h"

class SpectrogramWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    explicit SpectrogramWidget(QWidget* parent = nullptr);
    ~SpectrogramWidget() override;

    static constexpr int GRID_WIDTH = 1024;
    static constexpr int GRID_DEPTH = 512;

public slots:
    void addNewData(const std::shared_ptr<std::vector<float>>& spectrum);

signals:
    void fpsUpdated(float fps);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupGrid();
    void setupAxes();
    void setupTextures();

    int m_indexCount = 0;
    int m_writeRow = 0;

    HudOverlay* m_hud = nullptr;

    QOpenGLVertexArrayObject m_vao;
    QOpenGLBuffer m_vbo;
    QOpenGLBuffer m_ebo;
    QOpenGLShaderProgram m_waterfallShader;

    QOpenGLVertexArrayObject m_axesVao;
    QOpenGLBuffer m_axesVbo;
    QOpenGLShaderProgram m_lineShader;

    std::unique_ptr<QOpenGLTexture> m_dataTexture;
    std::unique_ptr<QOpenGLTexture> m_colormapTexture;

    QMatrix4x4 m_projection;
    QMatrix4x4 m_view;
    QMatrix4x4 m_model;

    float m_cameraRadius;
    float m_cameraYaw;
    float m_cameraPitch;
    QPoint m_lastMousePos;

    QElapsedTimer m_fpsTimer;
    int m_frameCount;
    float m_currentFps;
};
