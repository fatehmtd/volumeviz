#include "OpenCLVolumeRenderer.h"
#include <windows.h>
#include <qopengl.h>
#include <fstream>
#include <QDebug>

const char* getOCLErrorString(cl_int error)
{
	switch (error) {
		// run-time and JIT compiler errors
	case 0: return "CL_SUCCESS";
	case -1: return "CL_DEVICE_NOT_FOUND";
	case -2: return "CL_DEVICE_NOT_AVAILABLE";
	case -3: return "CL_COMPILER_NOT_AVAILABLE";
	case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case -5: return "CL_OUT_OF_RESOURCES";
	case -6: return "CL_OUT_OF_HOST_MEMORY";
	case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case -8: return "CL_MEM_COPY_OVERLAP";
	case -9: return "CL_IMAGE_FORMAT_MISMATCH";
	case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case -11: return "CL_BUILD_PROGRAM_FAILURE";
	case -12: return "CL_MAP_FAILURE";
	case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
	case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
	case -15: return "CL_COMPILE_PROGRAM_FAILURE";
	case -16: return "CL_LINKER_NOT_AVAILABLE";
	case -17: return "CL_LINK_PROGRAM_FAILURE";
	case -18: return "CL_DEVICE_PARTITION_FAILED";
	case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

		// compile-time errors
	case -30: return "CL_INVALID_VALUE";
	case -31: return "CL_INVALID_DEVICE_TYPE";
	case -32: return "CL_INVALID_PLATFORM";
	case -33: return "CL_INVALID_DEVICE";
	case -34: return "CL_INVALID_CONTEXT";
	case -35: return "CL_INVALID_QUEUE_PROPERTIES";
	case -36: return "CL_INVALID_COMMAND_QUEUE";
	case -37: return "CL_INVALID_HOST_PTR";
	case -38: return "CL_INVALID_MEM_OBJECT";
	case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case -40: return "CL_INVALID_IMAGE_SIZE";
	case -41: return "CL_INVALID_SAMPLER";
	case -42: return "CL_INVALID_BINARY";
	case -43: return "CL_INVALID_BUILD_OPTIONS";
	case -44: return "CL_INVALID_PROGRAM";
	case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
	case -46: return "CL_INVALID_KERNEL_NAME";
	case -47: return "CL_INVALID_KERNEL_DEFINITION";
	case -48: return "CL_INVALID_KERNEL";
	case -49: return "CL_INVALID_ARG_INDEX";
	case -50: return "CL_INVALID_ARG_VALUE";
	case -51: return "CL_INVALID_ARG_SIZE";
	case -52: return "CL_INVALID_KERNEL_ARGS";
	case -53: return "CL_INVALID_WORK_DIMENSION";
	case -54: return "CL_INVALID_WORK_GROUP_SIZE";
	case -55: return "CL_INVALID_WORK_ITEM_SIZE";
	case -56: return "CL_INVALID_GLOBAL_OFFSET";
	case -57: return "CL_INVALID_EVENT_WAIT_LIST";
	case -58: return "CL_INVALID_EVENT";
	case -59: return "CL_INVALID_OPERATION";
	case -60: return "CL_INVALID_GL_OBJECT";
	case -61: return "CL_INVALID_BUFFER_SIZE";
	case -62: return "CL_INVALID_MIP_LEVEL";
	case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
	case -64: return "CL_INVALID_PROPERTY";
	case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
	case -66: return "CL_INVALID_COMPILER_OPTIONS";
	case -67: return "CL_INVALID_LINKER_OPTIONS";
	case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

		// extension errors
	case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
	case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
	case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
	case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
	case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
	case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
	default: return "Unknown OpenCL error";
	}
}

void checkOCLError(cl_int error)
{
	if (error != CL_SUCCESS)
	{
		qDebug() << getOCLErrorString(error);
	}
	assert(error != CL_SUCCESS);
}

void OpenCLVolumeRenderer::setGLTexture(unsigned int textureId)
{
	AbstractVolumeRenderer::setGLTexture(textureId);
	_outputImage = cl::ImageGL(_context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, textureId);
}

void OpenCLVolumeRenderer::init()
{
	std::vector<cl::Platform> platforms;
	cl::Platform::get(&platforms);

	// Select the default platform and create a context using this platform and the GPU
	cl_context_properties cps[] = {
		// opencl platform
		CL_CONTEXT_PLATFORM, (cl_context_properties)(platforms[platforms.size() - 1])(),
		// opengl platform
		CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
		CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
		0, 0
	};

	_context = cl::Context(CL_DEVICE_TYPE_GPU, cps);

	// Get a list of devices on this platform
	auto _devices = _context.getInfo<CL_CONTEXT_DEVICES>();

	// Create a command queue and use the first device
	_commandQueue = cl::CommandQueue(_context, _devices[0], CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);

	// Read the kernel source from the file
	std::ifstream fhandle("kernels/kernel.cl");
	std::string kernel_src = std::string(std::istreambuf_iterator<char>(fhandle), std::istreambuf_iterator<char>());

	// Create and build the program
	cl::Program program = cl::Program(_context, kernel_src);
	auto error = program.build();

	if (error != CL_SUCCESS)
	{
		qDebug() << "Compilation error : " << error << program.getBuildInfo< CL_PROGRAM_BUILD_LOG>(_devices[0]).c_str();
		checkOCLError(error);
	}

	_volumeRenderingKernel = cl::Kernel(program, "volumeRenderingKernelProgressiveAlt");
	_postProcessingKernel = cl::Kernel(program, "postProcessingKernel");
	_ssaoKernel = cl::Kernel(program, "ssaoKernel");

	// Create the buffers
	_invModelViewProjectionMatrixBuffer = cl::Buffer(_context, CL_MEM_READ_ONLY, sizeof(glm::float4) * 4);
}

void OpenCLVolumeRenderer::cleanup()
{

}

void OpenCLVolumeRenderer::setVolumeData(VolumeData* vdata)
{
	if (vdata == nullptr) return;
	_volumeDataImage = cl::Image3D(_context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		cl::ImageFormat(CL_INTENSITY, CL_UNORM_INT8),
		vdata->_nxyz.x, vdata->_nxyz.y, vdata->_nxyz.z,
		0, 0, vdata->_data);
	AbstractVolumeRenderer::setVolumeData(vdata);
	//requestBuffersUpdate();
}

void OpenCLVolumeRenderer::setViewport(int x, int y, int w, int h)
{
	AbstractVolumeRenderer::setViewport(x, y, w, h);

	_depthMapImage = cl::Image2D(_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_INTENSITY, CL_FLOAT), w, h);
	_opacityMapImage = cl::Image2D(_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_INTENSITY, CL_FLOAT), w, h);
	_colorMapImage = cl::Image2D(_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT), w, h);
	_normalMapImage = cl::Image2D(_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT), w, h);
	_densityMapImage = cl::Image2D(_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_INTENSITY, CL_FLOAT), w, h);
	_positionMapImage = cl::Image2D(_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_FLOAT), w, h);
	_occlusionMapImage = cl::Image2D(_context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_INTENSITY, CL_FLOAT), w, h);
	requestBuffersUpdate();
}

void OpenCLVolumeRenderer::mainRenderPass()
{
	int result = 0;

	cl::Event event;

	////////////////////////////////////////////////////////////////////////////////////////////
	// Ray marching kernel

	// Write the parameters to the gpu
	result = _commandQueue.enqueueWriteBuffer(_invModelViewProjectionMatrixBuffer, true, 0, sizeof(glm::float4) * 4, &_invModelViewProjectionMatrix[0], nullptr);
	checkOCLError(result);

	// Set the kernel arguments
	result = _volumeRenderingKernel.setArg(0, _invModelViewProjectionMatrixBuffer);
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(1, glm::float4(_position, 0.0f));
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(2, glm::int4(_x, _y, _width, _height));
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(3, _volumeDataImage);
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(4, glm::int4(_vdata->_nxyz, 0));
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(5, glm::float4(_vdata->_sxyz, 0));
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(6, glm::float2(_vdata->_min, _vdata->_max));
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(7, _transferFunctionImage);
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(8, _depthMapImage);
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(9, _opacityMapImage);
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(10, _colorMapImage);
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(11, _normalMapImage);
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(12, _densityMapImage);
	checkOCLError(result);
	result = _volumeRenderingKernel.setArg(13, _positionMapImage);
	checkOCLError(result);

	result = _volumeRenderingKernel.setArg(14, (int)_updateRequested);
	checkOCLError(result);

	//*/
	// Launch the kernel
	cl::NDRange localRange(8, 8);
	cl::NDRange globalRange(2048, 2048);

	result = _commandQueue.enqueueNDRangeKernel(_volumeRenderingKernel, cl::NullRange, globalRange, localRange, nullptr, &event);
	checkOCLError(result);
	result = event.wait();
	checkOCLError(result);
}

void OpenCLVolumeRenderer::ssaoPass()
{
	// Launch the kernel
	cl::NDRange localRange(8, 8);
	cl::NDRange globalRange(2048, 2048);
	cl::Event event;

	int result = _ssaoKernel.setArg(0, _occlusionMapImage);
	checkOCLError(result);
	result = _ssaoKernel.setArg(1, _depthMapImage);
	checkOCLError(result);
	result = _ssaoKernel.setArg(2, _opacityMapImage);
	checkOCLError(result);

	// launch the kernel
	result = _commandQueue.enqueueNDRangeKernel(_ssaoKernel, cl::NullRange, globalRange, localRange, nullptr, &event);
	checkOCLError(result);
	result = event.wait();
	checkOCLError(result);
}

void OpenCLVolumeRenderer::postProcessingPass()
{
	////////////////////////////////////////////////////////////////////////////////////////////
	// Post Processing kernel
	// Enqueue the OpenGL shared objects
	std::vector<cl::Memory> memObjects;
	memObjects.push_back(_outputImage);

	cl::Event event;

	// Launch the kernel
	cl::NDRange localRange(8, 8);
	cl::NDRange globalRange(2048, 2048);

	// Acquire the opengl texture so it can be used by the kernel
	int result = _commandQueue.enqueueAcquireGLObjects(&memObjects, nullptr, &event);
	checkOCLError(result);
	result = event.wait();
	checkOCLError(result);

	result = _postProcessingKernel.setArg(0, _outputImage);
	checkOCLError(result);
	result = _postProcessingKernel.setArg(1, _depthMapImage);
	checkOCLError(result);
	result = _postProcessingKernel.setArg(2, _opacityMapImage);
	checkOCLError(result);
	result = _postProcessingKernel.setArg(3, _colorMapImage);
	checkOCLError(result);
	result = _postProcessingKernel.setArg(4, _normalMapImage);
	checkOCLError(result);
	result = _postProcessingKernel.setArg(5, _densityMapImage);
	checkOCLError(result);
	result = _postProcessingKernel.setArg(6, _positionMapImage);
	checkOCLError(result);
	result = _postProcessingKernel.setArg(7, _occlusionMapImage);
	checkOCLError(result);
	result = _postProcessingKernel.setArg(8, _transferFunctionImage);
	checkOCLError(result);

	// launch the kernel
	result = _commandQueue.enqueueNDRangeKernel(_postProcessingKernel, cl::NullRange, globalRange, localRange, nullptr, &event);
	checkOCLError(result);
	result = event.wait();
	checkOCLError(result);

	// Wait for the kernel to finish and release the OpenGL shared objects
	result = _commandQueue.enqueueReleaseGLObjects(&memObjects, nullptr, nullptr);
	checkOCLError(result);
	result = event.wait();
	checkOCLError(result);
}

void OpenCLVolumeRenderer::setTransferFunction(const QVector<QPair<QPointF, QColor>>& colors)
{
	// OpenCL structures
	struct TFControlPoint
	{
		cl_float4 value_opacity;
		cl_float4 rgb;
	};

	_numTFControlPoints = colors.size();

	TFControlPoint* controlPoints = new TFControlPoint[_numTFControlPoints];
	for (int i = 0; i < _numTFControlPoints; i++)
	{
		auto& cPoint = controlPoints[i];
		const auto& inputCPoint = colors[i];
		cPoint.value_opacity.x = inputCPoint.first.x();
		cPoint.value_opacity.y = inputCPoint.first.y();
		cPoint.rgb.x = inputCPoint.second.redF();
		cPoint.rgb.y = inputCPoint.second.greenF();
		cPoint.rgb.z = inputCPoint.second.blueF();
		cPoint.rgb.w = inputCPoint.first.y();
	}

	const int resolution = 1024; // the resolution of the 1d texture that holds the transfer function colors
	const int nchannels = 4; // RGBA 4-channels

	const float invResolution = 1.0f / (float)resolution;

	float* tfColorsBuffer = new float[resolution * nchannels];

	for (int i = 0; i < resolution; i++)
	{
		const int colorIndex = i * nchannels;
		const float t = i * invResolution;
		
		// linear interpolation
		
		bool segmentFound = false;
		for (int j = 0; j < _numTFControlPoints - 1; j++)
		{
			if (controlPoints[j].value_opacity.x <= t && controlPoints[j + 1].value_opacity.x >= t)
			{
				const auto& currentCP = controlPoints[j];
				const auto& nextCP = controlPoints[j + 1];

				const float interp = (t - currentCP.value_opacity.x) / (nextCP.value_opacity.x - currentCP.value_opacity.x);

				tfColorsBuffer[colorIndex] = glm::lerp(currentCP.rgb.x, nextCP.rgb.x, interp);
				tfColorsBuffer[colorIndex + 1] = glm::lerp(currentCP.rgb.y, nextCP.rgb.y, interp);
				tfColorsBuffer[colorIndex + 2] = glm::lerp(currentCP.rgb.z, nextCP.rgb.z, interp);
				tfColorsBuffer[colorIndex + 3] = glm::lerp(currentCP.value_opacity.y, nextCP.value_opacity.y, interp);
				segmentFound = true;
				break;
			}
		}

		if (!segmentFound)
		{
			const auto& lastCP = controlPoints[_numTFControlPoints - 1];
			tfColorsBuffer[colorIndex] = lastCP.rgb.x;
			tfColorsBuffer[colorIndex + 1] = lastCP.rgb.y;
			tfColorsBuffer[colorIndex + 2] = lastCP.rgb.z;
			tfColorsBuffer[colorIndex + 3] = lastCP.value_opacity.y;
		}
		//*/
	}
	_transferFunctionImage = cl::Image1D(_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_RGBA, CL_FLOAT), resolution, tfColorsBuffer);

	delete[] tfColorsBuffer;
	delete[] controlPoints;

	requestBuffersUpdate();
}

void OpenCLVolumeRenderer::render()
{
	if (!_renderingStatus) return;
	if (!_updateRequested) return;
	if (_vdata == nullptr) return;
	if (_numTFControlPoints < 2) return;

	mainRenderPass();
	ssaoPass();
	postProcessingPass();	

	_updateRequested = false;
}
