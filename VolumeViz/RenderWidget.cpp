#include "RenderWidget.h"
#include <qdebug.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <QSurface>
#include <QSurfaceFormat>

#define PRINT_GL_ERROR() {auto err= glGetError(); if(err != GL_NO_ERROR){ qDebug() << "Error : "<< err <<", LINE : " << __LINE__ ;}}

//////////////////////////////////////////////////////////////////////////
RenderWidget::RenderWidget(QWidget* parent) : QOpenGLWidget(parent)
{
	QSurfaceFormat format;
	format.setSamples(0);
	format.setRenderableType(QSurfaceFormat::RenderableType::OpenGL);
	QOpenGLWidget::setFormat(format);

	//setAutoFillBackground(false);
	setMinimumSize(640, 480);

	connect(&_timer, &QTimer::timeout, [=]()
		{
			update();
		});

	const float fps = 60.0f;
	_timer.start(1000.0f / fps);
}

RenderWidget::~RenderWidget()
{
	glDeleteTextures(1, &_textureId);
	glDeleteBuffers(1, &_vbo);
	glDeleteVertexArrays(1, &_vao);
	glDeleteShader(_vertexShader);
	glDeleteShader(_fragmentShader);
	glDeleteProgram(_shaderProgram);
}

void RenderWidget::setVolume(VolumeData* volumeData)
{
	_volumeData = volumeData;
	if (_volumeRenderer != nullptr)
	{
		_volumeRenderer->setVolumeData(volumeData);
	}
}

void RenderWidget::setVolumeRenderer(AbstractVolumeRenderer* volumeRenderer)
{
	_volumeRenderer = volumeRenderer;
	if (_volumeRenderer != nullptr)
	{
		_volumeRenderer->setGLTexture(_textureId);
		//_volumeRenderer->setTransferFunction(_transferFunction);
		//_volumeRenderer->setVolumeData(_volumeData);
	}
}

void RenderWidget::setTransferFunction(const TransferFunction& tfColors)
{
	if (tfColors.size() < 2) return;

	_transferFunction = tfColors;

	if (_volumeRenderer != nullptr)
	{
		_volumeRenderer->setTransferFunction(tfColors);
	}
}

AbstractVolumeRenderer* RenderWidget::getCurrentVolumeRenderer() const
{
	return _volumeRenderer;
}

void RenderWidget::initializeGL()
{
	initializeOpenGLFunctions();
	QOpenGLWidget::initializeGL();
	//glDisable(GL_MULTISAMPLE);
	createScreenQuad();
}

void RenderWidget::resizeGL(int w, int h)
{
	QOpenGLWidget::resizeGL(w, h);
	glViewport(0, 0, w, h);
	createTexture(w, h);
}

void RenderWidget::paintGL()
{
	glm::quat rotation(glm::float3(_angleX, _angleY, 0));

	const glm::vec3 upVector(0, 1, 0);
	glm::float3 _position(0, 0, _zoom), _target(0, 0, 0);

	glm::float3 vecDir3 = _target - _position;
	_position = _target + rotation * vecDir3;

	const glm::mat4 viewMatrix = glm::lookAt(_position, _target, rotation * upVector);

	if (_volumeRenderer != nullptr)
	{
		_volumeRenderer->setViewPosition(_position);
		_volumeRenderer->setMatrices(
			viewMatrix,
			glm::perspective(35.0f * 3.14f / 180.0f, float(width()) / float(height()), 1.0f, 999999.0f));
	}

	glClearColor(0, 0, 0, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	if (_volumeRenderer != nullptr) // check if there's a volume renderer
	{
		// perform rendering
		_volumeRenderer->render();
	}

	//////////////////////////////////////////////////////////////////////////
	glClearColor(0.2f, 0.4f, 0.6f, 1.0f);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glUseProgram(_shaderProgram);

	auto texture_uniform = glGetUniformLocation(_shaderProgram, "bg_texture");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _textureId);
	glUniform1i(texture_uniform, 0);

	glBindVertexArray(_vao);
	glDrawArrays(GL_QUADS, 0, 4);
	glFinish();
}

void RenderWidget::createTexture(int w, int h)
{
	if (glIsTexture(_textureId))
		glDeleteTextures(1, &_textureId);

	glGenTextures(1, &_textureId);
	glBindTexture(GL_TEXTURE_2D, _textureId);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	auto pixels = new unsigned char[4 * w * h];
	memset(pixels, 255, sizeof(unsigned char) * 4 * w * h);

	glm::vec4 gradientTop(211, 0, 211, 255);
	glm::vec4 gradientBottom(255, 255, 255, 255);

	for (int j = 0; j < h; j++)
	{
		const float t = (float)j / (float)h;
		auto color = t * gradientBottom + gradientTop * (1.0f - t);

		for (int i = 0; i < w; i++)
		{
			auto pixel = &pixels[(i + j * w) * 4];
			pixel[0] = color.r;
			pixel[1] = color.g;
			pixel[2] = color.b;
			pixel[3] = color.a;
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	delete[] pixels;


	if (_volumeRenderer != nullptr)
	{
		_volumeRenderer->setGLTexture(_textureId);
		_volumeRenderer->setViewport(0, 0, w, h);

		if (_volumeRenderer != nullptr)
			_volumeRenderer->requestBuffersUpdate();
	}
}

void RenderWidget::createScreenQuad()
{
	//////////////////////////////////////////////////////////////////////////
	static const float vertices[] = {
		 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		 0.0f, 1, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f, 1.0f,
		 1, 1, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
		 1, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
	};

	const char* vertexShaderSource = "#version 420\n"
		"layout(location = 0) in vec3 position;\n"
		"layout(location = 1) in vec3 color;\n"
		"layout(location = 2) in vec2 in_texcoords;\n"
		"out vec3 vcolor;\n"
		"out vec2 tex_coords;\n"
		"uniform mat4 modelViewProjectionMatrix;\n"
		"void main()\n"
		"{\n"
		"gl_Position = modelViewProjectionMatrix * vec4(position.xyz, 1.0);\n"
		"vcolor = color;"
		"tex_coords = in_texcoords;"
		"}\n";

	const char* fragmentShaderSource = "#version 420\n"
		"out vec4 FragColor;\n"
		"in vec3 vcolor;\n"
		"in vec2 tex_coords;\n"
		"uniform sampler2D bg_texture;\n"
		"void main()\n"
		"{\n"
		"\tgl_FragColor = vec4(texture(bg_texture, tex_coords.xy).rgb, 1);\n"
		"}\n";

	//////////////////////////////////////////////////////////////////////////
	// shaders
	_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(_vertexShader, 1, &vertexShaderSource, nullptr);
	glCompileShader(_vertexShader);
	PRINT_GL_ERROR();
	_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(_fragmentShader, 1, &fragmentShaderSource, nullptr);
	glCompileShader(_fragmentShader);
	PRINT_GL_ERROR();
	_shaderProgram = glCreateProgram();
	glAttachShader(_shaderProgram, _vertexShader);
	glAttachShader(_shaderProgram, _fragmentShader);
	glLinkProgram(_shaderProgram);

	glUseProgram(_shaderProgram);

	auto projMatrix = glm::ortho<float>(0, 1, 1, 0, -1, 1);
	auto mvpMatrixUniform = glGetUniformLocation(_shaderProgram, "modelViewProjectionMatrix");
	glUniformMatrix4fv(mvpMatrixUniform, 1, false, glm::value_ptr(projMatrix));

	// buffer objects
	glGenBuffers(1, &_vbo);

	glGenVertexArrays(1, &_vao);
	glBindVertexArray(_vao);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // position
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // color
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); // texcoords

	glEnableVertexAttribArray(0); // position
	glEnableVertexAttribArray(1); // color
	glEnableVertexAttribArray(2); // texcoords
}

void RenderWidget::mousePressEvent(QMouseEvent* event)
{
	_prevClick = event->pos();
	switch (event->button())
	{
	case Qt::MouseButton::LeftButton:
		_leftButtonPressed = true;
		break;
	case Qt::MouseButton::RightButton:
		_rightButtonPressed = true;
		break;
	}
}

void RenderWidget::mouseReleaseEvent(QMouseEvent* event)
{
	_rightButtonPressed = false;
	_leftButtonPressed = false;
}

void RenderWidget::mouseMoveEvent(QMouseEvent* event)
{
	if (_leftButtonPressed)
	{
		_angleY -= 3.75f * 3.14f * (float)(event->pos().x() - _prevClick.x()) / (float)width();
		_angleX -= 3.75f * 3.14f * (float)(event->pos().y() - _prevClick.y()) / (float)height();
		_prevClick = event->pos();
	}

	if (_rightButtonPressed)
	{
		_zoom -= 5550.0f * (float)(event->pos().y() - _prevClick.y()) / (float)height();
		if (_zoom < _minZoom)
			_zoom = _minZoom;
		_prevClick = event->pos();
	}

	if (_volumeRenderer != nullptr)
		_volumeRenderer->requestBuffersUpdate();
}

void RenderWidget::wheelEvent(QWheelEvent* event)
{
	_zoom -= (float)event->delta() * 0.5f;
	if (_zoom < _minZoom)
		_zoom = _minZoom;

	if (_volumeRenderer != nullptr)
		_volumeRenderer->requestBuffersUpdate();
}
