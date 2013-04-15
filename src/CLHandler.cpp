#include <GL/glew.h>
#include <GL/glx.h>
#include <Utils.h>
#include <CLHandler.h>

using namespace osp;

unsigned int CLHandler::instances_ = 0;

CLHandler * CLHandler::New() {
  if (instances_ == 0) {
    return new CLHandler();
  } else {
    ERROR("Can't create another instance of CLHandler");
    return NULL;
  }
}

CLHandler::CLHandler() {
}


std::string CLHandler::GetErrorString(cl_int _error) {
  return "hej";
}

bool CLHandler::Init() {
    // Find platform(s)
    error_ = clGetPlatformIDs(32, platforms_, &numPlatforms_);
    if (error_ == CL_SUCCESS) {
      INFO("Number of CL platforms: " << numPlatforms_);
    } else {
      ERROR("Failed to get CL platforms");
      ERROR(GetErrorString(error_));
      return false;
    }

    // TODO Assume only one found platform for now
    if (numPlatforms_ != 1) {
      WARNING("numPlatforms not equal to 1, did not count on that");
    }

    // Find devices
    error_ = clGetDeviceIDs(platforms_[0], 
                            CL_DEVICE_TYPE_ALL,
                            sizeof(devices_),
                            devices_,
                            &numDevices_);
    if (error_ == CL_SUCCESS) {
      INFO("Number of CL devices: " << numDevices_);
    } else {
      ERROR("Failed to get CL devices");
      ERROR(GetErrorString(error_));
      return false;
    }

    // Loop over devices
    for (unsigned int i=0; i<numDevices_; i++) {
      error_ = clGetDeviceInfo(devices_[i],
                               CL_DEVICE_NAME,
                               sizeof(deviceName_),
                               deviceName_,
                               NULL);
      if (error_ == CL_SUCCESS) {
        INFO("Device " << i << " name: " << deviceName_);
      } else {
        ERROR("Failed to get device name");
        ERROR(GetErrorString(error_));
        return false;
      }
    }

  return true;
}

bool CLHandler::InitInterop(Raycaster * _raycaster) {
  // Create an OpenCL context with a reference to an OpenGL context
  cl_context_properties contextProperties[] = {
    CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
    CL_CONTEXT_PLATFORM, (cl_context_properties)platforms_[0],
    0};

  context_ = clCreateContext(contextProperties,
                             1,
                             &devices_[0], // TODO only use one device now
                             NULL,
                             NULL,
                             &error_);
  if (error_ == CL_SUCCESS) {
    INFO("CL context set up properly");
  } else {
    ERROR("Failed to set up CL context");
    ERROR(GetErrorString(error_));
    return false;
  }

  return true;
}
