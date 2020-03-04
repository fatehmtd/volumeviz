#pragma once

#define GL_GLEXT_PROTOTYPES
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <qtimer.h>

#include "AbstractVolumeRenderer.h"

class RenderWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions
{
public:
	RenderWidget(QWidget* parent);
	~RenderWidget();

	void setVolumeRenderer(AbstractVolumeRenderer* volumeRenderer);
protected:
	virtual void initializeGL() override;
	virtual void resizeGL(int w, int h) override;
	virtual void paintGL() override;
	void createTexture(int w, int h);
	void createScreenQuad();
protected:
	AbstractVolumeRenderer* _volumeRenderer = nullptr;
	QTimer _timer;
	unsigned int _vao, _vbo, _textureId;
	unsigned int _shaderProgram, _vertexShader, _fragmentShader;
};

