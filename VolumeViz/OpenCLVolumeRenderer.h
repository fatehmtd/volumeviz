#pragma once

#include "AbstractVolumeRenderer.h"

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_TARGET_OPENCL_VERSION 120
#define CL_VERSION_1_2
#define NOMINMAX
#include <cl/cl.hpp>


class OpenCLVolumeRenderer : public AbstractVolumeRenderer
{
public:
	virtual void render() override;
	virtual void setGLTexture(unsigned int) override;
	virtual void init() override;
	virtual void cleanup() override;
	virtual void setVolumeData(VolumeData* vdata) override;
	virtual void setViewport(int x, int y, int w, int h) override;
private:
	void mainRenderPass();
	void ssaoPass();
	void postProcessingPass();

protected:
	cl::Context _context;
	cl::CommandQueue _commandQueue;
	cl::Kernel _volumeRenderingKernel;
	cl::Kernel _postProcessingKernel;
	cl::Kernel _ssaoKernel;

	// OpenCL Buffers
	cl::Buffer _invModelViewProjectionMatrixBuffer;
	cl::Image3D _volumeDataImage;
	cl::Image1D _transferFunctionImage;

	cl::Image2D _depthMapImage, _colorMapImage, _opacityMapImage, _normalMapImage, _densityMapImage, _positionMapImage, _occlusionMapImage;

	cl::ImageGL _outputImage;

	int _numTFControlPoints = 0;
	cl_float3 _volumeScale;
	cl_int3 _volumeDimensions;

	// Inherited via AbstractVolumeRenderer
	virtual void setTransferFunction(const QVector<QPair<QPointF, QColor>>& colors) override;
};

