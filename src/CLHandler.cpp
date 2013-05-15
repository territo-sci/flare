/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 * 
 */

#include <GL/glew.h>
#include <GL/glx.h>
#include <Utils.h>
#include <CLHandler.h>
#include <Texture2D.h>
//#include <Texture3D.h>
#include <VoxelData.h>
#include <TransferFunction.h>
#include <sstream>
#include <fstream>
#include <cmath>
#include <boost/timer/timer.hpp>

using namespace osp;

unsigned int CLHandler::instances_ = 0;

CLHandler * CLHandler::New() {
  if (instances_ == 0) {
    instances_++;
    return new CLHandler();
  } else {
    ERROR("Can't create another instance of CLHandler");
    return NULL;
  }
}

CLHandler::CLHandler() 
  : error_(CL_SUCCESS), 
    mappedMemory_(NULL),
    memoryMapped_(false),
    numPlatforms_(0),
    numDevices_(0) {
}

CLHandler::~CLHandler() {
  clReleaseKernel(kernel_);
  clReleaseProgram(program_);
  clReleaseCommandQueue(commandQueue_);
  clReleaseContext(context_);

  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin();
       it != OGLTextures_.end(); it++) {
    clReleaseMemObject(it->second);
  }

  for (std::map<cl_uint, MemKernelArg>::iterator it = memKernelArgs_.begin();
       it != memKernelArgs_.end(); it++) {
    clReleaseMemObject(it->second.mem_);
  }

  for (std::vector<MemKernelArg>::iterator it = pinnedMemory_.begin();
       it != pinnedMemory_.end(); it++) {
    clReleaseMemObject(it->mem_);
  }

  if (mappedMemory_) delete mappedMemory_;
}

std::string CLHandler::ErrorString(cl_int _error) {
  switch (_error) {
    case CL_SUCCESS:
      return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:
      return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:
      return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:
      return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
      return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
      return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
      return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
      return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
      return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
      return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
      return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
      return "CL_MAP_FAILURE";
    case CL_INVALID_VALUE:
      return "CL_INVALID_VALUE";
    case CL_INVALID_DEVICE_TYPE:
      return "CL_INVALID_DEVICE_TYPE";
    case CL_INVALID_PLATFORM:
      return "CL_INVALID_PLATFORM";
    case CL_INVALID_DEVICE:
      return "CL_INVALID_DEVICE";
    case CL_INVALID_CONTEXT:
      return "CL_INVALID_CONTEXT";
    case CL_INVALID_QUEUE_PROPERTIES:
      return "CL_INVALID_QUEUE_PROPERTIES";
    case CL_INVALID_COMMAND_QUEUE:
      return "CL_INVALID_COMMAND_QUEUE";
    case CL_INVALID_HOST_PTR:
      return "CL_INVALID_HOST_PTR";
    case CL_INVALID_MEM_OBJECT:
      return "CL_INVALID_MEM_OBJECT";
    case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
      return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case CL_INVALID_IMAGE_SIZE:
      return "CL_INVALID_IMAGE_SIZE";
    case CL_INVALID_SAMPLER:
      return "CL_INVALID_SAMPLER";
    case CL_INVALID_BINARY:
      return "CL_INVALID_BINARY";
    case CL_INVALID_BUILD_OPTIONS:
      return "CL_INVALID_BUILD_OPTIONS";
    case CL_INVALID_PROGRAM:
      return "CL_INVALID_PROGRAM";
    case CL_INVALID_PROGRAM_EXECUTABLE:
      return "CL_INVALID_PROGRAM_EXECUTABLE";
    case CL_INVALID_KERNEL_NAME:
      return "CL_INVALID_KERNEL_NAME";
    case CL_INVALID_KERNEL_DEFINITION:
      return "CL_INVALID_KERNEL_DEFINITION";
    case  CL_INVALID_KERNEL:
      return "CL_INVALID_KERNEL";
    case CL_INVALID_ARG_INDEX:
      return "CL_INVALID_ARG_INDEX";
    case CL_INVALID_ARG_VALUE:
      return "CL_INVALID_ARG_VALUE";
    case CL_INVALID_ARG_SIZE:
      return "CL_INVALID_ARG_SIZE";
    case CL_INVALID_KERNEL_ARGS:
      return "CL_INVALID_KERNEL_ARGS";
    case CL_INVALID_WORK_DIMENSION:
      return "CL_INVALID_WORK_DIMENSION";
    case CL_INVALID_WORK_GROUP_SIZE:
      return "CL_INVALID_WORK_GROUP_SIZE";
    case CL_INVALID_WORK_ITEM_SIZE:
      return "CL_INVALID_WORK_ITEM_SIZE";
    case CL_INVALID_GLOBAL_OFFSET:
      return "CL_INVALID_GLOBAL_OFFSET";
    case CL_INVALID_EVENT_WAIT_LIST:
      return "CL_INVALID_EVENT_WAIT_LIST";
    case CL_INVALID_EVENT:
      return "CL_INVALID_EVENT";
    case CL_INVALID_OPERATION:
      return "CL_INVALID_OPERATION";
    case CL_INVALID_GL_OBJECT:
      return "CL_INVALID_GL_OBJECT";
    case  CL_INVALID_BUFFER_SIZE:
      return "CL_INVALID_BUFFER_SIZE";
    case CL_INVALID_MIP_LEVEL:
      return "CL_INVALID_MIP_LEVEL";
    case CL_INVALID_GLOBAL_WORK_SIZE:
      return "CL_INVALID_GLOBAL_WORK_SIZE";
    default:
      std::stringstream ss;
      std::string code;
      ss << "Unknown OpenCL error code - " << _error;
      return ss.str();
  }
}


bool CLHandler::CheckSuccess(cl_int _error, std::string _location) {
  if (_error == CL_SUCCESS) {
    return true;
  } else {
    ERROR(_location);
    ERROR(ErrorString(_error));
    return false;
  }
}

bool CLHandler::InitPlatform() {
  error_ = clGetPlatformIDs(32, platforms_, &numPlatforms_);
  if (error_ == CL_SUCCESS) {
    INFO("Number of CL platforms: " << numPlatforms_);
  } else {
    CheckSuccess(error_, "InitPlatform()");
    return false;
  }
  // TODO
  if (numPlatforms_ > 1) {
    WARNING("More than one platform found, this is unsupported");
  }
  return true;
}

bool CLHandler::InitDevices() {
  if (numPlatforms_ < 1) {
    ERROR("Number of platforms is < 1");
    return false;
  }

  // Find devices
  error_ = clGetDeviceIDs(platforms_[0], CL_DEVICE_TYPE_ALL,
                          sizeof(devices_), devices_, &numDevices_);
  if (CheckSuccess(error_, "InitDevices()")) {
    INFO("Number of CL devices: " << numDevices_);
  } else {
    return false;
  }

  // Loop over found devices and print info
  for (unsigned int i=0; i<numDevices_; i++) {
    error_ = clGetDeviceInfo(devices_[i], CL_DEVICE_NAME, 
                             sizeof(deviceName_), deviceName_, NULL);
    if (CheckSuccess(error_, "InitDevices()")) {
      INFO("Device " << i << " name: " << deviceName_);
    } else {
      return false;
    }
  }

  return true;
}
/*
bool CLHandler::Init() {
  // Find platform(s)
  error_ = clGetPlatformIDs(32, platforms_, &numPlatforms_);
  if (error_ == CL_SUCCESS) {
    INFO("Number of CL platforms: " << numPlatforms_);
  } else {
    ERROR("Failed to get CL platforms");
    ERROR(ErrorString(error_));
    return false;
  }

  // TODO Assume only one found platform for now
  if (numPlatforms_ != 1) {
    WARNING("numPlatforms not equal to 1, did not count on that");
  }

  // Find devices
  error_ = clGetDeviceIDs(platforms_[0], CL_DEVICE_TYPE_ALL, 
                          sizeof(devices_), devices_, &numDevices_);
  if (error_ == CL_SUCCESS) {
    INFO("Number of CL devices: " << numDevices_);
  } else {
    ERROR("Failed to get CL devices");
    ERROR(ErrorString(error_));
    return false;
  }

  // Loop over devices
  for (unsigned int i=0; i<numDevices_; i++) {
    error_ = clGetDeviceInfo(devices_[i], CL_DEVICE_NAME, sizeof(deviceName_),
                             deviceName_, NULL);
    if (error_ == CL_SUCCESS) {
      INFO("Device " << i << " name: " << deviceName_);
    } else {
      ERROR("Failed to get device name");
      ERROR(ErrorString(error_));
      return false;
    }
  }

  return true;
}
*/

bool CLHandler::CreateContext() {

  if (numPlatforms_ < 1) {
    ERROR("Number of platforms < 1, can't create context");
    return false;
  }

  if (numDevices_ < 1) {
    ERROR("Number of devices < 1, can't create context");
    return false;
  }

  // Create an OpenCL context with a reference to an OpenGL context
  cl_context_properties contextProperties[] = {
    CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
    CL_CONTEXT_PLATFORM, (cl_context_properties)platforms_[0],
    0};
 
  // TODO Use more than one device
  context_ = clCreateContext(contextProperties, 1, &devices_[0], NULL,
                             NULL, &error_);

  return CheckSuccess(error_, "CreateContext()");
}



/*
bool CLHandler::AddTexture3D(unsigned int _argNr, Texture3D *_texture,
                             bool _readOnly) {

  // Remove anything already associated with argument index
  if (GLTextures_.find((cl_uint)_argNr) != GLTextures_.end()) {
    INFO("Erasing texture at kernel argument " << _argNr);
    GLTextures_.erase((cl_uint)_argNr);
  }

  cl_mem_flags flag = _readOnly ? CL_MEM_READ_ONLY : CL_MEM_WRITE_ONLY;
  cl_mem texture = clCreateFromGLTexture3D(context_, flag, GL_TEXTURE_3D,
                                           0, _texture->Handle(), &error_);

  if (error_ != CL_SUCCESS) {
    ERROR("Failed to create cl_mem object for argument index " << _argNr);
    ERROR(ErrorString(error_));
    return false;
  }
  
  INFO("Inserting kernel argument " << _argNr);
  GLTextures_.insert(std::make_pair((cl_uint)_argNr, texture));

  return true;
}
*/

char * CLHandler::ReadSource(std::string _filename, int &_numChars) const {
  FILE *in;
  char *content = NULL;
  in = fopen(_filename.c_str(), "r");
  int count;
  if (in != NULL) {
    fseek(in, 0, SEEK_END);
    count = ftell(in);
    rewind(in);
    content = (char *)malloc(sizeof(char)*(count+1));
    count = fread(content, sizeof(char), count, in);
    content[count] = '\0';
    fclose(in);
  } else {
    ERROR("Could not read source from file " << _filename);
  }
  _numChars = count;
  return content;
}


bool CLHandler::CreateProgram(std::string _filename) {
  int numChars;
  char * source = ReadSource(_filename, numChars);
  program_ = clCreateProgramWithSource(context_, 1, (const char**)&source,
                                       (const size_t*)&numChars, &error_);
  delete source;
  return CheckSuccess(error_, "CreateProgram()");
}


bool CLHandler::BuildProgram() {
  error_ = clBuildProgram(program_, (cl_uint)0, NULL, NULL, NULL, NULL);

  if (CheckSuccess(error_, "BuildProgram()")) {
    return true;
  } else {

    // Print build log
    char * log;
    size_t logSize = 0;
    clGetProgramBuildInfo(program_, devices_[0], CL_PROGRAM_BUILD_LOG,
                          0, NULL, &logSize);
    if (logSize > 0) {
      INFO("Build log:");
      log = new char[logSize+1];
      clGetProgramBuildInfo(program_, devices_[0], CL_PROGRAM_BUILD_LOG,
                            logSize, log, NULL);
      log[logSize] = '\0';
      INFO(log);
      delete log;                       
    }

    return false;
  }
}

bool CLHandler::CreateKernel() {
  kernel_ = clCreateKernel(program_, "Raycaster", &error_);
  return CheckSuccess(error_, "CreateKernel()");
}

bool CLHandler::CreateCommandQueue() {
  commandQueue_ = clCreateCommandQueue(context_, devices_[0], 0, &error_);
  return CheckSuccess(error_, "CreateCommandQueue");
}

bool CLHandler::AddTexture2D(unsigned int _argNr, Texture2D *_texture, 
                             Permissions _permissions) {
  
  // Remove anything already associated with argument index
  if (OGLTextures_.find((cl_uint)_argNr) != OGLTextures_.end()) {
    OGLTextures_.erase((cl_uint)_argNr);
  }
 
  cl_mem_flags flag;
  switch (_permissions) {
    case READ_ONLY:
      flag = CL_MEM_READ_ONLY;
      break;
    case WRITE_ONLY:
      flag = CL_MEM_WRITE_ONLY;
      break;
    case READ_WRITE:
      flag = CL_MEM_READ_WRITE;
      break;
    default:
      ERROR("Unknown permission type");
      return false;
  }

  cl_mem texture = clCreateFromGLTexture2D(context_, flag, GL_TEXTURE_2D,
                                           0, _texture->Handle(), &error_);

  if (!CheckSuccess(error_, "AddTexture2D()")) {
    ERROR("Failed to add GL texture at argument index " << _argNr);
    return false;
  }

  OGLTextures_.insert(std::make_pair((cl_uint)_argNr, texture));

  return true;
}
bool CLHandler::AddTransferFunction(unsigned int _argNr, 
                                     TransferFunction *_tf) {

  // Remove old TF already bound to this argument index 
  if (memKernelArgs_.find((cl_uint)_argNr) != memKernelArgs_.end()) {
    memKernelArgs_.erase((cl_uint)_argNr);
  }

  MemKernelArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.mem_ = clCreateBuffer(context_,
                             CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             _tf->Width()*4*sizeof(float),
                             _tf->FloatData(),
                             &error_);

  if (!CheckSuccess(error_, "AddTransferFunction()")) {
    ERROR("Failed to bind transfer functions to arg nr " << _argNr);
    return false;
  }

  memKernelArgs_.insert(std::make_pair((cl_uint)_argNr, mka));
  return true;
}

/*
bool CLHandler::BindVoxelData(unsigned int _argNr, 
                              VoxelData<float> *_voxelData) { 
  
  // Remove old data already bound to this argument index
  if (memKernelArgs_.find((cl_uint)_argNr) != memKernelArgs_.end()) {
    memKernelArgs_.erase((cl_uint)_argNr);
  }

  MemKernelArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.mem_ = clCreateBuffer(context_,
                              CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              _voxelData->NumVoxelsTotal()*sizeof(float),
                              _voxelData->DataPtr(),
                              &error_);

  if (error_ != CL_SUCCESS) {
    ERROR("Failed to bind voxel data to arg index " << _argNr);
    ERROR(ErrorString(error_));
    return false;
  }

  memKernelArgs_.insert(std::make_pair((cl_uint)_argNr, mka));
  return true;
}
*/

bool CLHandler::AddConstants(unsigned int _argNr, 
                              KernelConstants *_kernelConstants) {

  // Delete any old data already bound to argument index
  if (memKernelArgs_.find((cl_uint)_argNr) != memKernelArgs_.end()) {
    memKernelArgs_.erase((cl_uint)_argNr);
  }

  MemKernelArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.mem_ = clCreateBuffer(context_,
                              CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              sizeof(KernelConstants),
                              _kernelConstants,
                              &error_);

  if (!CheckSuccess(error_, "BindConstants()")) {
    ERROR("Failed to bind constants to kernel arg nr " << _argNr);
    return false;
  }

  memKernelArgs_.insert(std::make_pair((cl_uint)_argNr, mka));
  return true;
}

bool CLHandler::RunRaycaster() {

  boost::timer::auto_cpu_timer t(6, "%w RunRaycaster()\n"); 

  if (pinnedMemory_.size() != 2) {
    ERROR("Pinned memory size != 2");
    return false;
  }

  // TODO Don't hardcode
  size_t globalSize[] = { 512, 512 };
  size_t localSize[] = { 16, 16 };

  // Let OpenCL take control of the shared GL objects
  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin(); 
       it != OGLTextures_.end(); it++) {
    error_ = clEnqueueAcquireGLObjects(commandQueue_, 1, &(it->second),
                                       0, NULL, NULL);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to enqueue GL object aqcuisition");
      ERROR("Failing object: " << it->first);
      ERROR(ErrorString(error_));
      return false;
    }
  }

  // Set up kernel arguments of non-shared items
  for (std::map<cl_uint, MemKernelArg>::iterator it = memKernelArgs_.begin();
       it != memKernelArgs_.end(); it++) {
    error_ = clSetKernelArg(kernel_,
                            it->first,
                            (it->second).size_,
                            &((it->second).mem_));
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to set kernel argument " << it->first);
      ERROR(ErrorString(error_));
      return false;
    }
  }
  
  
  // Set up kernel arguments for textures
  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin(); 
       it != OGLTextures_.end(); it++) {
    error_ = clSetKernelArg(kernel_,
                            it->first,
                            sizeof(cl_mem),
                            &(it->second));
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to set kernel argument " << it->first);
      ERROR(ErrorString(error_));
      return false;
    }
  }

  // Set up kernel argument for the active voxel data buffer
  error_ = clSetKernelArg(kernel_, 
                          voxelDataArgNr_, 
                          pinnedMemory_[activeIndex_].size_,
                          &(pinnedMemory_[activeIndex_].mem_));
  if (!CheckSuccess(error_, "Kernel argument for voxel data buffer")) {
    return false;
  } 

  // Set up unsigned integer kernel arguments
  /*
  for (std::map<cl_uint, unsigned int>::iterator it = uintArgs_.begin();
       it != uintArgs_.end();
       it++) {
    error_ = clSetKernelArg(kernel_,
                            it->first,
                            sizeof(unsigned int),
                            &(it->second));
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to set kernel argument " << it->first);
      ERROR(ErrorString(error_));
      return false;
    }
  }
  */

  // Set up kernel execution
  error_ = clEnqueueNDRangeKernel(commandQueue_, kernel_, 2, NULL, globalSize,
                                  localSize, 0, NULL, NULL);
  if (error_ != CL_SUCCESS) {
    ERROR("Failed to enqueue kernel");
    ERROR(ErrorString(error_));
    return false;
  }

  // Release the shared GL objects
  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin(); 
       it != OGLTextures_.end(); it++) {
    error_ = clEnqueueReleaseGLObjects(commandQueue_, 1, &(it->second), 0,
                                       NULL, NULL);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to release GL object");
      ERROR("Failed object: " << it->first);
      ERROR(ErrorString(error_));
      return false;
    }
  }
  
  return true;
}


bool CLHandler::InitPinnedMemory(unsigned int _argNr, 
                                 VoxelData<float> *_voxelData) {
  voxelDataArgNr_ = _argNr;
  pinnedMemory_.resize(2);
  for (std::vector<MemKernelArg>::iterator it = pinnedMemory_.begin();
       it != pinnedMemory_.end(); it++) {
    it->mem_ = clCreateBuffer(context_, 
                              CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                              _voxelData->NumVoxelsPerTimestep()*sizeof(float),
                              NULL, &error_);
    it->size_ = sizeof(cl_mem);//sizeof(float)*_voxelData->NumVoxelsPerTimestep();
    if (!CheckSuccess(error_, "InitPinnedMemory()")) {
      return false;
    }
  }
  // Copy first frame to FIRST buffer
  if (!MapPinnedMemory(FIRST, 
                       _voxelData->NumVoxelsPerTimestep()*sizeof(float))) 
    return false;
  if (!VoxelDataToMappedMemory(_voxelData, 0)) return false;
  //if (!UnmapPinnedMemory(FIRST)) return false;
  return true;
}


/*
bool CLHandler::InitHostBuffers(unsigned int _bufferSize) {
  
  // TODO remove one step
  pinnedMemory_.resize(2);
  for (std::vector<cl_mem>::iterator it = pinnedMemory_.begin();
       it != pinnedMemory_.end(); it++) {
    *it = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                         _bufferSize, NULL, &error_);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to create pinned memory");
      ERROR(ErrorString(error_));
      return false;
    }
  }
  return true;
}
*/

bool CLHandler::MapPinnedMemory(MemoryIndex _memoryIndex, unsigned int _size) {

  boost::timer::auto_cpu_timer t(6, "%w MapPinnedMemory()\n");

  if (mappedMemory_) {
    ERROR("MapPinnedMemory(): Memory already mapped");
    return false;
  }

  mappedMemory_ = reinterpret_cast<float*>(
                    clEnqueueMapBuffer(commandQueue_,
                                       pinnedMemory_[_memoryIndex].mem_,
                                       CL_TRUE, // TODO blocking now
                                       CL_MAP_WRITE,
                                       0,
                                       _size,
                                       0,
                                       NULL,
                                       NULL,
                                       &error_));

  return CheckSuccess(error_, "MapBufferToPinnedMemory()");
}

bool CLHandler::UnmapPinnedMemory(MemoryIndex _memoryIndex) {

  boost::timer::auto_cpu_timer t(6, "%w UnmapPinnedMemory()\n");

  if (!mappedMemory_) {
    ERROR("UnmapPinnedMemory(): Memory is not mapped");
    return false;
  }

  error_ = clEnqueueUnmapMemObject(commandQueue_, 
                                   pinnedMemory_[_memoryIndex].mem_,
                                   reinterpret_cast<float*>(mappedMemory_),
                                   0, NULL, NULL);
  mappedMemory_ = NULL;
  return CheckSuccess(error_, "UnmapPinnedMemory()");
}

bool CLHandler::VoxelDataToMappedMemory(VoxelData<float> *_voxelData,
                                        unsigned int _timestep) {

  boost::timer::auto_cpu_timer t(6, "%w VoxelDataToMappedMemory()\n");

  if (!mappedMemory_) {
    ERROR("VoxelDataToMappedMemory(): Memory is not mapped");
    return false;
  }
  unsigned int offset = _voxelData->TimestepOffset(_timestep);
  float * data = _voxelData->DataPtr(offset);
  unsigned int size = _voxelData->NumVoxelsPerTimestep();
  std::copy(data, data+size, mappedMemory_);
  return true;
}


                                     
