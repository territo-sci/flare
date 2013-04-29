#ifndef CL_HANDLER_H
#define CL_HANDLER_H

#include <string>
#include <map>
#include <CL/cl.hpp>
#include <VoxelData.h>
#include <KernelConstants.h>

namespace osp {

class Texture2D;

class CLHandler {
public:
  static CLHandler * New();
  bool Init();
  bool CreateContext();
	bool AddGLTexture(unsigned int _argNr, Texture2D * _texture, bool _readOnly);

	bool CreateProgram(std::string _filename);
	bool BuildProgram();
	bool CreateKernel();
	bool CreateCommandQueue();
	bool BindFloatData(unsigned int _argNr, 
	                   float *_floatData, 
	                   unsigned int _size);
	bool BindData(unsigned int _argNr, VoxelData<float> *_voxelData);
	bool RunRaycaster(unsigned int _timestepOffset);
	bool BindConstants(KernelConstants *kernelConstants_);
private:
  CLHandler();
	char * ReadSource(std::string _filename, int &_numChars) const;
  std::string GetErrorString(cl_int _error);
  cl_int error_;
  cl_uint numPlatforms_;
  cl_platform_id platforms_[32];
  cl_uint numDevices_;
  cl_device_id devices_[32];
  char deviceName_[1024];
  cl_context context_;
  cl_command_queue commandQueue_;
  cl_mem cubeFront_;
	cl_mem cubeBack_;
  cl_mem output_;
	cl_program program_;
	cl_kernel kernel_;
  static unsigned int instances_;

	// Stores textures together with their kernel argument number
	std::map<cl_uint, cl_mem> GLTextures_;

	// Stores float data together with their kernel argument number
	std::map<cl_uint, cl_mem> floatData_;

  // Constants
	cl_mem constants_;
	
};

}

#endif
