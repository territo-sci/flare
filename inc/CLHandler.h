#ifndef CL_HANDLER_H
#define CL_HANDLER_H

/*
Author: Victor Sand (victor.sand@gmail.com)
Gathers OpenCL functionality in once place.
Note that this is very much work in progress.
*/

// TODO Gather all kernel argument types under the same roof, so that
// they can all be handled in the same map. Would be useful to check for
// checking accidental overwrites of argument indices and to get rid 
// of redundant code.


#include <string>
#include <map>
#include <CL/cl.hpp>
#include <VoxelData.h>
#include <KernelConstants.h>

namespace osp {

class Texture2D;
class Texture3D;
class TransferFunction;

class CLHandler {
public:
  static CLHandler * New();
  
  // Struct to enable organization of cl_mem kernel arguments
  struct MemKernelArg {
    size_t size_;
    cl_mem value_;
  };

  // Find platform and devices 
  bool Init();
  // Create context to cooperate with OpenGL
  bool CreateContext();
  // Create a cl_mem object from a Texture2D, and add it to the list of
  // textures. Textures can be read-only or not. 
  // TODO change name to "Add"
  bool BindTexture2D(unsigned int _argIndex, Texture2D * _texture, 
                     bool _readOnly);
  
  bool AddTexture3D(unsigned int _argIndex, Texture3D *_texture,
                    bool _readOnly);

  // Create an OpenCL program from a text source file
  bool CreateProgram(std::string _filename);
  // Build program, must be called after creating it
  bool BuildProgram();
  // Create kernel, must be called after building program
  bool CreateKernel();
  // Create a command queue, must be called after creating context
  bool CreateCommandQueue();
  // Add a transfer function cl_mem object to the kernel argument list
  // TODO Implement with 1D texture when OpenCL version supports it
  bool BindTransferFunction(unsigned int _argIndex,
                            TransferFunction *_tf);
  // Add float voxel data to cl_mem kernel argument list
  // TODO Implement as 3D textures, possibly discard completely when better
  // data structures are implemented
  bool BindVoxelData(unsigned int _argIndex, VoxelData<float> *_voxelData);
  // Is called by the renderer at every frame
  bool RunRaycaster();
  // Add a KernelConstants structure to the list of cl_mem kernel arguments
  bool BindConstants(unsigned int _argIndex, 
                     KernelConstants *kernelConstants_);
  // Add the timestep offset to the list of cl_mem kernel arguments
  bool BindTimestepOffset(unsigned int _argIndex,
                          unsigned int _timestepOffset);
private:
  CLHandler();

  // Read a kernel source from file, return a char array.
  // Stores number of characters in _numChar argument.
  char * ReadSource(std::string _filename, int &_numChars) const;
  // Translate cl_int enum to readable string
  std::string GetErrorString(cl_int _error);

  // Used for error checking when appropriate 
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

  // TODO try to implement uniform structure for these maps

  // Stores textures together with their kernel argument number
  std::map<cl_uint, cl_mem> GLTextures_;

  // Stores non-shared (non-texture) memory buffer arguments
  std::map<cl_uint, MemKernelArg> memKernelArgs_;

  // Stores unsigned int kernel argument
  std::map<cl_uint, unsigned int> uintArgs_;

};

}

#endif
