#include "OpenCLVolumeRenderer.h"
#include <windows.h>
#include <qopengl.h>
#include <fstream>
#include <QDebug>

extern const char* getErrorString(cl_int error)
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

extern void printError(cl_int error)
{
	if (error != CL_SUCCESS)
		qDebug() << getErrorString(error);

	assert(error == CL_SUCCESS);
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
	}

	_volumeRenderingKernel = cl::Kernel(program, "volumeRenderingKernel");

	// Create the buffers
	_invModelViewProjectionMatrixBuffer = cl::Buffer(_context, CL_MEM_READ_ONLY, sizeof(glm::float4) * 4);
}

void OpenCLVolumeRenderer::cleanup()
{

}

void OpenCLVolumeRenderer::render()
{
	cl::Event event;
	// Enqueue the OpenGL shared objects
	std::vector<cl::Memory> memObjects;
	memObjects.push_back(_outputImage);

	int result = _commandQueue.enqueueAcquireGLObjects(&memObjects, nullptr, &event);
	printError(result);
	result = event.wait();
	printError(result);

	// Prepare the kernel's arguments
	result = _volumeRenderingKernel.setArg(0, _outputImage);
	printError(result);

	result = _commandQueue.enqueueWriteBuffer(_invModelViewProjectionMatrixBuffer, true, 0, sizeof(glm::float4) * 4, &_invModelViewProjectionMatrix[0], nullptr);
	printError(result);

	result = _volumeRenderingKernel.setArg(1, _invModelViewProjectionMatrixBuffer);
	printError(result);

	result = _volumeRenderingKernel.setArg(2, glm::float4(_modelViewMatrix[3][0], _modelViewMatrix[3][1], _modelViewMatrix[3][2], 0.0f));
	printError(result);

	result = _volumeRenderingKernel.setArg(3, glm::int4(_x, _y, _width, _height));
	printError(result);

	// Launch the kernel
	cl::NDRange localRange(16, 16);
	cl::NDRange globalRange(4096, 4096);

	result = _commandQueue.enqueueNDRangeKernel(_volumeRenderingKernel, cl::NullRange, globalRange, localRange, nullptr, &event);
	printError(result);
	result = event.wait();
	printError(result);

	// Wait for the kernel to finish and release the OpenGL shared objects
	result = _commandQueue.enqueueReleaseGLObjects(&memObjects, nullptr, &event);
	printError(result);
	result = event.wait();
	printError(result);
}
