#include "SpectrogramWidget.h"
#include <QDebug>
#include <QtMath>

static_assert((SpectrogramWidget::GRID_DEPTH& (SpectrogramWidget::GRID_DEPTH - 1)) == 0,
    "GRID_DEPTH must be a power of two so the ring-buffer index can use a bitmask.");

static const char* waterfallVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform sampler2D dataTexture;  
    uniform float headOffset;

    out float heightValue;          

    void main() {
        float u = (aPos.x + 1.0) * 0.5;
        float v = fract((aPos.z + 1.0) * 0.5 + headOffset);

        float height = texture(dataTexture, vec2(u, v)).r;
        heightValue = height;

        vec3 displacedPos = vec3(aPos.x, height * 1.5, aPos.z);
        gl_Position = projection * view * model * vec4(displacedPos, 1.0);
    }
)";

static const char* waterfallFragmentShader = R"(
    #version 330 core
    in float heightValue;
    out vec4 FragColor;

    uniform sampler1D colormap;

    void main() {
        vec3 color = texture(colormap, heightValue).rgb;
        FragColor = vec4(color, 1.0);
    }
)";

static const char* lineVertexShader = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aColor;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec3 lineColor;

    void main() {
        lineColor = aColor;
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

static const char* lineFragmentShader = R"(
    #version 330 core
    in vec3 lineColor;
    out vec4 FragColor;
    void main() { 
        FragColor = vec4(lineColor, 1.0); 
    }
)";

static bool buildShader(QOpenGLShaderProgram& prog, const char* vsrc, const char* fsrc, const QString& name)
{
    if (!prog.addShaderFromSourceCode(QOpenGLShader::Vertex, vsrc)) {
        qCritical() << name << "Vertex Shader Compile Failed:" << prog.log();
        return false;
    }
    if (!prog.addShaderFromSourceCode(QOpenGLShader::Fragment, fsrc)) {
        qCritical() << name << "Fragment Shader Compile Failed:" << prog.log();
        return false;
    }
    if (!prog.link()) {
        qCritical() << name << "Shader Linking Failed:" << prog.log();
        return false;
    }
    return true;
}

SpectrogramWidget::SpectrogramWidget(QWidget* parent)
    : QOpenGLWidget(parent),
    m_vbo(QOpenGLBuffer::VertexBuffer),
    m_ebo(QOpenGLBuffer::IndexBuffer),
    m_axesVbo(QOpenGLBuffer::VertexBuffer),
    m_writeRow(0),
    m_frameCount(0),
    m_currentFps(0.0f),
    m_cameraRadius(3.5f),
    m_cameraYaw(45.0f),
    m_cameraPitch(30.0f)
{
    m_hud = new HudOverlay(this);
    m_hud->setGridSize(GRID_WIDTH, GRID_DEPTH);

    connect(this, &QOpenGLWidget::resized, this, [this]() {
        if (m_hud) {
            m_hud->resize(this->size());
        }
        });

    m_fpsTimer.start();
}

SpectrogramWidget::~SpectrogramWidget()
{
    makeCurrent();
    m_vao.destroy();
    m_vbo.destroy();
    m_ebo.destroy();
    m_axesVao.destroy();
    m_axesVbo.destroy();
    doneCurrent();
}

void SpectrogramWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.01f, 0.01f, 0.02f, 1.0f);

    buildShader(m_waterfallShader, waterfallVertexShader, waterfallFragmentShader, "Waterfall Engine");
    buildShader(m_lineShader, lineVertexShader, lineFragmentShader, "Grid-Line Engine");

    setupGrid();
    setupAxes();
    setupTextures();

    m_model.setToIdentity();
}

void SpectrogramWidget::resizeGL(int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, static_cast<float>(w) / h, 0.1f, 100.0f);
}

void SpectrogramWidget::setupGrid()
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    vertices.reserve(GRID_WIDTH * GRID_DEPTH * 3);
    indices.reserve((GRID_WIDTH - 1) * (GRID_DEPTH - 1) * 6);

    for (int z = 0; z < GRID_DEPTH; ++z) {
        for (int x = 0; x < GRID_WIDTH; ++x) {
            float xPos = (static_cast<float>(x) / (GRID_WIDTH - 1)) * 2.0f - 1.0f;
            float zPos = (static_cast<float>(z) / (GRID_DEPTH - 1)) * 2.0f - 1.0f;
            vertices.push_back(xPos);
            vertices.push_back(0.0f);
            vertices.push_back(zPos);
        }
    }

    for (int z = 0; z < GRID_DEPTH - 1; ++z) {
        for (int x = 0; x < GRID_WIDTH - 1; ++x) {
            unsigned int topLeft = z * GRID_WIDTH + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomLeft = (z + 1) * GRID_WIDTH + x;
            unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);  indices.push_back(bottomLeft); indices.push_back(topRight);
            indices.push_back(topRight); indices.push_back(bottomLeft); indices.push_back(bottomRight);
        }
    }
    m_indexCount = static_cast<int>(indices.size());

    m_vao.create();
    m_vao.bind();

    m_vbo.create();
    m_vbo.bind();
    m_vbo.allocate(vertices.data(), static_cast<int>(vertices.size() * sizeof(float)));

    m_ebo.create();
    m_ebo.bind();
    m_ebo.allocate(indices.data(), static_cast<int>(indices.size() * sizeof(unsigned int)));

    m_waterfallShader.enableAttributeArray(0);
    m_waterfallShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 3 * sizeof(float));

    m_vao.release();
}

void SpectrogramWidget::setupAxes()
{
    float axesData[] = {
        0.0f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,   1.2f, 0.0f, 0.0f,  1.0f, 0.2f, 0.2f,
        0.0f, 0.0f, 0.0f,  0.2f, 1.0f, 0.2f,   0.0f, 1.0f, 0.0f,  0.2f, 1.0f, 0.2f,
        0.0f, 0.0f, 0.0f,  0.2f, 0.6f, 1.0f,   0.0f, 0.0f, 1.2f,  0.2f, 0.6f, 1.0f
    };

    m_axesVao.create();
    m_axesVao.bind();
    m_axesVbo.create();
    m_axesVbo.bind();
    m_axesVbo.allocate(axesData, sizeof(axesData));

    m_lineShader.enableAttributeArray(0);
    m_lineShader.setAttributeBuffer(0, GL_FLOAT, 0, 3, 6 * sizeof(float));

    m_lineShader.enableAttributeArray(1);
    m_lineShader.setAttributeBuffer(1, GL_FLOAT, 3 * sizeof(float), 3, 6 * sizeof(float));

    m_axesVao.release();
}

void SpectrogramWidget::setupTextures()
{
    m_dataTexture = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
    m_dataTexture->create();
    m_dataTexture->setFormat(QOpenGLTexture::R32F);
    m_dataTexture->setSize(GRID_WIDTH, GRID_DEPTH);
    m_dataTexture->allocateStorage();
    m_dataTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    m_dataTexture->setWrapMode(QOpenGLTexture::DirectionS, QOpenGLTexture::ClampToEdge);
    m_dataTexture->setWrapMode(QOpenGLTexture::DirectionT, QOpenGLTexture::Repeat);

    std::vector<float> emptyData(GRID_WIDTH * GRID_DEPTH, 0.0f);
    m_dataTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, emptyData.data());

    m_colormapTexture = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target1D);
    m_colormapTexture->create();
    m_colormapTexture->setFormat(QOpenGLTexture::RGB8_UNorm);
    m_colormapTexture->setSize(256);
    m_colormapTexture->allocateStorage();
    m_colormapTexture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    m_colormapTexture->setWrapMode(QOpenGLTexture::ClampToEdge);

    std::vector<unsigned char> colormapData(256 * 3);
    for (int i = 0; i < 256; ++i) {
        float t = i / 255.0f;
        colormapData[i * 3 + 0] = static_cast<unsigned char>(qBound(0.0f, 1.5f * t - 0.2f, 1.0f) * 255);
        colormapData[i * 3 + 1] = static_cast<unsigned char>(qBound(0.0f, 2.0f * t - 1.0f, 1.0f) * 255);
        colormapData[i * 3 + 2] = static_cast<unsigned char>(qBound(0.0f, 1.0f - 2.0f * t, 1.0f) * 255);
    }
    m_colormapTexture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, colormapData.data());
}

void SpectrogramWidget::addNewData(const std::shared_ptr<std::vector<float>>& spectrum)
{
    if (!spectrum || spectrum->size() != GRID_WIDTH) return;

    makeCurrent();

    m_dataTexture->bind();
    glTexSubImage2D(GL_TEXTURE_2D, 0,
        0, m_writeRow,
        GRID_WIDTH, 1,
        GL_RED, GL_FLOAT,
        spectrum->data());
    m_dataTexture->release();

    m_writeRow = (m_writeRow + 1) & (GRID_DEPTH - 1);

    doneCurrent();
    update();
}

void SpectrogramWidget::paintGL()
{
    m_frameCount++;
    if (m_fpsTimer.elapsed() >= 1000) {
        m_currentFps = m_frameCount * 1000.0f / m_fpsTimer.elapsed();
        m_frameCount = 0;
        m_fpsTimer.restart();

        if (m_hud) {
            m_hud->setFps(m_currentFps);
        }
        emit fpsUpdated(m_currentFps);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    float yawRad = qDegreesToRadians(m_cameraYaw);
    float pitchRad = qDegreesToRadians(m_cameraPitch);
    QVector3D eye(
        m_cameraRadius * qCos(pitchRad) * qSin(yawRad),
        m_cameraRadius * qSin(pitchRad),
        m_cameraRadius * qCos(pitchRad) * qCos(yawRad)
    );
    m_view.setToIdentity();
    m_view.lookAt(eye, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });

    m_waterfallShader.bind();
    m_waterfallShader.setUniformValue("model", m_model);
    m_waterfallShader.setUniformValue("view", m_view);
    m_waterfallShader.setUniformValue("projection", m_projection);

    float headOffset = static_cast<float>(m_writeRow) / GRID_DEPTH;
    m_waterfallShader.setUniformValue("headOffset", headOffset);

    glActiveTexture(GL_TEXTURE0);
    m_dataTexture->bind();
    m_waterfallShader.setUniformValue("dataTexture", 0);

    glActiveTexture(GL_TEXTURE1);
    m_colormapTexture->bind();
    m_waterfallShader.setUniformValue("colormap", 1);

    m_vao.bind();
    glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    m_vao.release();
    m_waterfallShader.release();

    m_lineShader.bind();
    m_lineShader.setUniformValue("model", m_model);
    m_lineShader.setUniformValue("view", m_view);
    m_lineShader.setUniformValue("projection", m_projection);

    m_axesVao.bind();
    glDrawArrays(GL_LINES, 0, 6);
    m_axesVao.release();
    m_lineShader.release();

    glDisable(GL_CULL_FACE);
}

void SpectrogramWidget::mousePressEvent(QMouseEvent* e)
{
    m_lastMousePos = e->pos();
}

void SpectrogramWidget::mouseMoveEvent(QMouseEvent* e)
{
    if (!(e->buttons() & Qt::LeftButton)) return;

    QPoint delta = e->pos() - m_lastMousePos;
    m_lastMousePos = e->pos();

    m_cameraYaw += delta.x() * 0.4f;
    m_cameraPitch = qBound(-89.0f, m_cameraPitch - delta.y() * 0.4f, 89.0f);

    update();
}

void SpectrogramWidget::wheelEvent(QWheelEvent* e)
{
    m_cameraRadius -= e->angleDelta().y() * 0.005f;
    m_cameraRadius = qBound(1.0f, m_cameraRadius, 10.0f);
    update();
}
