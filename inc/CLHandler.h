#ifndef CL_HANDLER_H
#define CL_HANDLER_H

#include <string>
#include <CL/cl.hpp>

namespace osp {

class Raycaster;

class CLHandler {
public:
  static CLHandler * New();
  bool Init();
  bool InitInterop(Raycaster * _raycaster);
private:
  CLHandler();
  std::string GetErrorString(cl_int _error);
  cl_int error_;
  cl_uint numPlatforms_;
  cl_platform_id platforms_[32];
  cl_uint numDevices_;
  cl_device_id devices_[32];
  char deviceName_[1024];
  cl_context context_;
  cl_command_queue commandQueue_;
  cl_mem readFromImage_;
  cl_mem writeToImage_;
  static unsigned int instances_;

};

}

#endif