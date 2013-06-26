/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#ifndef CL_MANAGER_H_
#define CL_MANAGER_H_

#include <CL/cl.hpp>
#include <map>
#include <string>

namespace osp {

class Texture;
class TransferFunction;
class CLProgram;
struct KernelConstants;

class CLManager {
public:
  static CLManager * New();
  ~CLManager();

  enum QueueIndex { EXECUTE, TRANSFER, NUM_QUEUE_INDICES };
  enum TextureType { TEXTURE_1D, TEXTURE_2D, TEXTURE_3D };
  enum Permissions { READ_ONLY, WRITE_ONLY, READ_WRITE };

  bool InitPlatform();
  bool InitDevices();
  bool CreateContext();
  bool CreateCommandQueue();
  bool CreateProgram(std::string _programName, std::string _fileName);
  bool BuildProgram(std::string _programName);
  bool CreateKernel(std::string _programName);

  bool AddTexture(std::string _programName, unsigned int _argNr,
                  Texture *_texture, TextureType _textureType,
                  Permissions _permissions);
  bool AddTransferFunction(std::string _programName, unsigned int _argNr,
                           TransferFunction *_transferFunction);
  bool AddKernelConstants(std::string _programName, unsigned int _argNr,
                          KernelConstants *_kernelConstants);
  bool AddIntArray(std::string _programName, unsigned int _argNr,
                   int *_intArray, unsigned int _size,
                   Permissions _permissions);
  
  // Aquire any shared textures, set up kernel arguments ets
  bool PrepareProgram(std::string _programName);

  // Launch program kernel (returns immediately)
  bool LaunchProgram(std::string _programName);

  // Wait for kernel to finish, releaste any shared resources
  bool FinishProgram(std::string _programName);

  // Finish all commands in a command queue
  // Can be used e.g. to sync a DMA transfer operation
  bool FinishQueue(QueueIndex _queueIndex);
   
  friend class CLProgram;

private:
  CLManager();
  CLManager(const CLManager&);

  // Read kernel source from file, return char array
  // Store number of chars in _numChar argument
  char * ReadSource(std::string _fileName, int &_numChars) const;
  // Check state in a cl_int, print error if state is not CL_SUCCESS
  bool CheckSuccess(cl_int _error, std::string _location) const;
  // Translate cl_int enum to readable string
  std::string ErrorString(cl_int _error) const;

  static const unsigned int MAX_PLATFORMS = 32;
  static const unsigned int MAX_DEVICES = 32;
  static const unsigned int MAX_NAME_LENGTH = 128;

  // Used for storing and reading error state during function calls
  cl_int error_;
  cl_uint numPlatforms_;
  cl_platform_id platforms_[MAX_PLATFORMS];
  cl_uint numDevices_;
  cl_device_id devices_[MAX_DEVICES];
  // Maximum allocatable size for each device
  cl_ulong maxMemAllocSize_[MAX_DEVICES];
  char deviceName_[MAX_NAME_LENGTH];
  cl_context context_;
  cl_command_queue commandQueues_[NUM_QUEUE_INDICES];

  // Programs are mapped using strings
  std::map<std::string, CLProgram*> clPrograms_;

};

}

#endif
  


