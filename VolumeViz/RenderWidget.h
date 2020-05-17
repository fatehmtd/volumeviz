#pragma once

#define GL_GLEXT_PROTOTYPES
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <qtimer.h>
#include <QMouseEvent>
#include <QWheelEvent>

#include "AbstractVolumeRenderer.h"

class RenderWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions
{
public:
	RenderWidget(QWidget* parent);
	~RenderWidget();
	void setVolume(VolumeData* volumeData);
	void setVolumeRenderer(AbstractVolumeRenderer* volumeRenderer);
	void setTransferFunction(const TransferFunction& tfColors);
	AbstractVolumeRenderer* getCurrentVolumeRenderer() const;
protected:
	virtual void initializeGL() override;
	virtual void resizeGL(int w, int h) override;
	virtual void paintGL() override;
	void createTexture(int w, int h);
	void createScreenQuad();

	virtual void mousePressEvent(QMouseEvent* event) override;
	virtual void mouseReleaseEvent(QMouseEvent* event) override;
	virtual void mouseMoveEvent(QMouseEvent* event) override;
	virtual void wheelEvent(QWheelEvent* event) override;

protected:
	VolumeData* _volumeData = nullptr;
	AbstractVolumeRenderer* _volumeRenderer = nullptr;
	QTimer _timer;
	unsigned int _vao, _vbo, _textureId;
	unsigned int _shaderProgram, _vertexShader, _fragmentShader;
	float _zoom = 500.0f;
	float _minZoom = 100.0f;
	float _angleX = 0.0f, _angleY = 0.0f;
	bool _leftButtonPressed = false;
	bool _rightButtonPressed = false;
	QPoint _prevClick;
	TransferFunction _transferFunction;
};

