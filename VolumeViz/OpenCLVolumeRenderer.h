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
protected:
	cl::Context _context;
	cl::CommandQueue _commandQueue;
	cl::Kernel _volumeRenderingKernel;

	// OpenCL Buffers
	cl::Buffer _invModelViewProjectionMatrixBuffer;
	cl::Buffer _transferFunctionPointsBuffer;
	cl::Buffer _volumeDataBuffer;

	cl::ImageGL _outputImage;
};

