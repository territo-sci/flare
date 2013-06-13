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

class Texture;
class TransferFunction;

class CLHandler {
public:
  static CLHandler * New(); 
  ~CLHandler();

  // Bundle cl_mem and its size
  struct MemArg {
    size_t size_;
    cl_mem mem_;
  };

  // Buffer permissions
  enum Permissions { READ_ONLY, WRITE_ONLY, READ_WRITE };

  // For different types of textures
  enum TextureType { TEXTURE_1D, TEXTURE_2D, TEXTURE_3D };

  // Indices for double memory buffer setup
  enum MemIndex { FIRST = 0, SECOND, NUM_MEM_INDICES };

  // Indices for double command queue setup
  enum QueueIndex { EXECUTE = 0, TRANSFER, NUM_QUEUE_INDICES };

  bool InitPlatform();
  bool InitDevices();
  bool CreateContext();
  bool CreateProgram(std::string _filename);
  bool BuildProgram();
  bool CreateKernel();
  bool CreateCommandQueues();

  // Add an OGL texture
  bool AddTexture(unsigned int _argNr, 
                  Texture *_texture, 
                  TextureType _textureType,
                  Permissions _p);
  // Add a transfer function
  bool AddTransferFunction(unsigned int _argNr, TransferFunction *_tf);
  // Add a box list
  bool AddBoxList(unsigned int _argNr, std::vector<int> _boxList);
  // Add kernel constants
  bool AddConstants(unsigned int _argNr, KernelConstants *_kernelConstants);

  /*  
  // Init the double buffer setup
  bool InitBuffers(unsigned int _argNr,
                   VoxelData<float> *_voxelData);
  // Update host memory with data
  bool UpdateHostMemory(MemIndex _memIndex, 
                        VoxelData<float> *_data,
                        unsigned int _timestep);
  

  // Transfer from host to device
  bool WriteToDevice(MemIndex _index);
   
  // Set index used for rendering
  bool SetActiveIndex(MemIndex _memoryIndex);
  */


  // Aquire shared OGL textures and set up kernel arguments
  bool PrepareRaycaster();

  // Launch kernel (asynchronously, returns immediately)
  bool LaunchRaycaster();

  // Wait for kernel to finish, release shared OGL textures
  virtual bool FinishRaycaster();

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
  cl_ulong maxMemAllocSize_[32];
  char deviceName_[128];
  cl_context context_;
  cl_command_queue commandQueues_[NUM_QUEUE_INDICES];
  cl_program program_;
  cl_kernel kernel_;

  // Stores device side OGL textures together with their kernel arg number
  std::map<cl_uint, cl_mem> OGLTextures_;

  // Host side textures for volume data
  //std::vector<Texture3D*> hostTextures_;
  
  // Device side textures for volume data
  //std::vector<MemArg> deviceTextures_;
  
  // Handles for PBOs
  //std::vector<unsigned int> pixelBufferObjects_;

  // Stores non-texture memory buffer arguments
  std::map<cl_uint, MemArg> memArgs_;
  
  // Keep track of which memory buffer to use for rendering
  //MemIndex activeIndex_;
  
  // Argument number for voxel data
  //cl_uint voxelDataArgNr_;
  
  // Timer members
  bool useTimers_;
  boost::timer::cpu_timer timer_;
  const double BYTES_PER_GB = 1073741824.0; 
 
  // Size of buffers for float data
  size_t bufferSize_;

  // Size of copy buffer
  // TODO Do a "dry run" at init and determine this value
  const unsigned int copyBufferSize_ = 262144;
};

}

#endif
