/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */
 
#ifndef CL_HANDLER_H
#define CL_HANDLER_H
 
#include <CL/cl.hpp> 
#include <string>
#include <map>
#include <vector>
#include <VoxelData.h>
#include <KernelConstants.h>
#include <boost/timer/timer.hpp>

namespace osp {

class Texture2D;
class TransferFunction;

class CLHandler {
public:
  static CLHandler * New();

  ~CLHandler();

  struct MemKernelArg {
    size_t size_;
    cl_mem mem_;
  };

  enum Permissions {
    READ_ONLY = 0,
    WRITE_ONLY,
    READ_WRITE
  };

  enum MemoryIndex {
    FIRST = 0,
    SECOND,
    NUM_MEM_INDICES
  };
  
  bool InitPlatform();
  bool InitDevices();
  bool CreateContext();

  bool InitBuffers(unsigned int _argNr, VoxelData<float> *_voxelData);
  bool UpdatePinnedMemory(MemoryIndex _index, 
                          VoxelData<float> *_voxelData,
                          unsigned int _timestep);
  bool WriteToDevice(MemoryIndex _index);
  
  bool CreateProgram(std::string _filename);
  bool BuildProgram();
  bool CreateKernel();
  bool CreateCommandQueue();
 
  bool AddTexture2D(unsigned int _argNr, Texture2D *_texture, Permissions _p);
  bool AddTransferFunction(unsigned int _argNr, TransferFunction *_tf);
  bool AddConstants(unsigned int _argNr, KernelConstants *_kernelConstants);
  
  // Sync command queue
  bool Finish();

  bool RunRaycaster();

  bool ToggleTimers();

private:
  CLHandler();

  // Read a kernel source from file, return a char array
  // Store number of characters in _numChar argument
  char * ReadSource(std::string _filename, int &_numChars) const;

  // Translate cl_int enum to readable string
  std::string ErrorString(cl_int _error);

  // Check for success/error stored in cl_int, print error along with 
  // location if not equal to CL_SUCCESS
  bool CheckSuccess(cl_int _error, std::string _location);

  bool UnmapPinnedPointers();

  cl_int error_;
  cl_uint numPlatforms_;
  cl_platform_id platforms_[32];
  cl_uint numDevices_;
  cl_ulong maxMemAllocSize_;
  cl_device_id devices_[32];
  char deviceName_[128];
  cl_context context_;
  cl_command_queue commandQueue_;
  cl_program program_;
  cl_kernel kernel_;
  static unsigned int instances_;

  // Stores textures together with their kernel argument number
  std::map<cl_uint, cl_mem> OGLTextures_;

  // Stores non-texture memory buffer arguments
  std::map<cl_uint, MemKernelArg> memKernelArgs_;

  MemoryIndex activeIndex_;
  cl_uint voxelDataArgNr_;

  // Page-locked memory on host
  std::vector<MemKernelArg> pinnedHostMemory_;
  // Device buffers in which to load into from pinned memory
  std::vector<MemKernelArg> deviceBuffer_;
  // Mapped pointers, referencing pinned host memory
  std::vector<float*> pinnedPointer_;
  // Size of buffers for float data
  size_t bufferSize_;

  // Size for buffer used to optimize copying to the pinned memory.
  // This might very well be system-dependant and a
  // TODO is to choose this by some init test or similar.
  static const unsigned int copyBufferSize_ = 262144;

  bool useTimers_;
  boost::timer::cpu_timer timer_;

  // Constants
  static const double BYTES_PER_GB = 1073741824.0;

};

}

#endif























/*

#ifndef CL_HANDLER_H
#define CL_HANDLER_H


// Author: Victor Sand (victor.sand@gmail.com)
// Gathers OpenCL functionality in one place.
// Note that this is very much work in progress.


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
  bool AddTexture2D(unsigned int _argIndex, Texture2D * _texture, 
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

  bool InitHostBuffers(unsigned int _bufferSize);
  bool MapBufferToPinnedMemory(unsigned int _bufferIndex);
  bool UnmapBuffer(unsigned int _bufferIndex);
  bool CopyToMappedMemory(float *_data, unsigned int _size);
  bool SetVoxelDataArgIndex(unsigned int _argIndex);

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

  // Keep two buffers for pinning (mapping), used for DMA transfers
  std::vector<cl_mem> pinnedMemory_;
  // Pointer to the currently mapped memory
  float *mappedMemory_;
  bool memoryIsMapped_;

};

}

#endif
*/
