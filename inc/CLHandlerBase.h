/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#ifndef CL_HANDLER_H_
#define CL_HANDLER_H_

#include <CL/cl.hpp>
#include <map>
#include <string>
#include <vector>
#include <VoxelData.h>
#include <KernelConstants.h>
#include <boost/timer/timer.hpp>

namespace osp {

class Texture2D;
class TransferFunction;

class CLHandler {
public:
  virtual ~CLHandler();

  // Bundle cl_mem and its size
  struct MemArg {
    size_t size_;
    cl_mem mem_;
  };

  // Buffer permissions
  enum Permission {
    READ_ONLY,
    WRITE_ONLY,
    READ_WRITE
  };

  // Indices for double memory buffer setup
  enum MemIndex {
    FIRST = 0,
    SECOND,
    NUM_MEM_INDICES
  };

  // Indices for double command queue setup
  enum QueueIndex {
    EXECUTE = 0,
    TRANSFER,
    NUM_QUEUE_INDICES
  };

  bool InitPlatform();
  bool InitDevices();
  bool CreateContext();
  bool CreateProgram(std::string _filename);
  bool BuildProgram();
  bool CreateKernel();
  bool CreateCommandQueues();

  // Add an OGL 2D texture 
  bool AddTexture2D(unsigned int _argNr, Texture2D *_texture, Permissions _p);
  // Add a transfer function
  bool AddTransferFunction(unsigned int _argNr, TransferFunction *_tf);
  // Add kernel constants
  bool AddConstants(unsigned int _argNr, KernelConstants *_kernelConstants);
  
  // Init the double buffer setup
  virtual bool InitBuffers(unsigned int _argNr,
                           VoxelData<float> *_voxelData) = 0;
  // Update host memory with data
  virtual bool UpdateHostMemory(MemoryIndex _memIndex, 
                                VoxelData<float> *_data) = 0;
  // Transfer from host to device
  virtual bool WriteToDevice(MemoryIndex _index) = 0;
   
  // Set index used for rendering
  bool SetActiveIndex(MemoryIndex _memoryIndex);

  // Aquire shared OGL textures and set up kernel arguments
  bool PrepareRaycaster() = 0;
  // Launch kernel (asynchronously, returns immediately)
  bool LaunchRaycaster();
  // Wait for kernel to finish, release shared OGL textures
  bool FinishRaycaster();

  // Toggle timer use. When timers are on, many calls become blocking.
  bool ToggleTimers();

  // Finish all commands in a command queue
  // Can be used e.g. to sync a DMA transfer operation
  bool FinishQueue(QueueIndex _queueIndex);


protected:
  CLHandler();

  // Read a kernel source from file, return a char array.
  // Store number of characters in _numChar argument
  char * ReadSource(std::string _filename, int &_numChars) const;
  
  // Translate cl_int enum to readable string
  std::string ErrorString(cl_int _error);

  // Check state in cl_int, print error if not equal to CL_SUCCESS
  bool CheckSuccess(cl_int _error, std::string _location);
  
  cl_int error_;
  cl_uint numPlatforms_;
  cl_platform_id platforms_[32];
  cl_uint numDevices_;
  cl_device_id devices_[32];
  cl_uint maxMemAllocSize_[32];
  char deviceName_[128];
  cl_context context_;
  cl_command_Queue_[NUM_QUEUE_INDICES];
  cl_program program_;
  cl_kernel kernel_;
  
  // Stores OGL textures together with their kernel argument number
  std::map<cl_uint, cl_mem> OGLTextures_;

  // Stores non-texture memory buffer arguments
  std::map<cl_uint, MemArg> memArgs_;

  // Keep track of which memory buffer to use for rendering
  MemoryIndex activeIndex_;

  // Argument number for voxel data
  cl_uint voxelDataArgNr_;

  // Timer members
  bool useTimers_;
  boost::timer::cpu_timer timer_;
  static const double BYTES_PER_GB = 1073741824.0; 
 
}

#endif