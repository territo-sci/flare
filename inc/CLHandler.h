#ifndef CL_HANDLER_H
#define CL_HANDLER_H

#include <string>
#include <map>
#include <CL/cl.hpp>
#include <VoxelData.h>
#include <KernelConstants.h>

namespace osp {

class Texture2D;
class TransferFunction;

class CLHandler {
public:
  static CLHandler * New();
	
	struct MemKernelArg {
		size_t size_;
		cl_mem value_;
	};

  bool Init();
  bool CreateContext();
	bool BindTexture2D(unsigned int _argIndex, Texture2D * _texture, bool _readOnly);

	bool CreateProgram(std::string _filename);
	bool BuildProgram();
	bool CreateKernel();
	bool CreateCommandQueue();
	bool BindTransferFunction(unsigned int _argIndex,
	                          TransferFunction *_tf);
	bool BindVoxelData(unsigned int _argIndex, VoxelData<float> *_voxelData);
	bool RunRaycaster();
	bool BindConstants(unsigned int _argIndex, 
	                   KernelConstants *kernelConstants_);
	bool BindTimestepOffset(unsigned int _argIndex,
	                        unsigned int _timestepOffset);
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
	cl_program program_;
	cl_kernel kernel_;
  static unsigned int instances_;

	// Stores textures together with their kernel argument number
	std::map<cl_uint, cl_mem> GLTextures_;

	// Stores non-shared (non-texture) memory buffer arguments
	std::map<cl_uint, MemKernelArg> memKernelArgs_;

	// Stores unsigned int kernel argument
	std::map<cl_uint, unsigned int> uintArgs_;

};

}

#endif
