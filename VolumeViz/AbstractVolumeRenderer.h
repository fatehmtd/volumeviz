#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/detail/setup.hpp>

#include "VolumeData.h"

class AbstractVolumeRenderer
{
public:
	enum RenderType
	{
		Shaded,
		Unshaded,
		Opacity,
		Depth
	};
	virtual void init() = 0;
	virtual void cleanup() = 0;
	virtual void render() = 0;
	virtual void setGLTexture(unsigned int);
	virtual void setViewport(int x, int y, int width, int height);
	virtual void setRenderType(RenderType type);
	virtual void setMatrices(glm::mat4x4 modelViewMatrix, glm::mat4x4 projectionMatrix);
	virtual void setVolumeData(VolumeData* vdata);
protected:
	VolumeData* _vdata = nullptr;
	unsigned int _glTexture = -1;
	RenderType _renderType;
	glm::mat4x4 _modelViewMatrix, _projectionMatrix, _invModelViewProjectionMatrix;
	int _width = 0, _height = 0;
	int _x = 0, _y = 0;
};

