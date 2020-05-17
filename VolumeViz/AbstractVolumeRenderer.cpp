#include "AbstractVolumeRenderer.h"
#include <qopengl.h>

void AbstractVolumeRenderer::setGLTexture(unsigned int textureId)
{
	this->_glTexture = textureId;
}

void AbstractVolumeRenderer::setViewport(int x, int y, int width, int height)
{
	_x = x;
	_y = y;
	_width = width;
	_height = height;
}

void AbstractVolumeRenderer::setRenderType(RenderType type)
{
	_renderType = type;
}

void AbstractVolumeRenderer::setMatrices(glm::mat4x4 modelViewMatrix, glm::mat4x4 projectionMatrix)
{
	_modelViewMatrix = modelViewMatrix;
	_projectionMatrix = projectionMatrix;
	_invModelViewProjectionMatrix = glm::inverse(_projectionMatrix*_modelViewMatrix);
}

void AbstractVolumeRenderer::setViewPosition(glm::vec3 position)
{
	_position = position;
}

void AbstractVolumeRenderer::setVolumeData(VolumeData* vdata)
{
	if (vdata == nullptr) return;
	_vdata = vdata;
}

void AbstractVolumeRenderer::requestBuffersUpdate()
{
	_updateRequested = true;
}

void AbstractVolumeRenderer::setRenderingStatus(bool status)
{
	this->_renderingStatus = status;
}
