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
#include <algorithm>
#include <pthread.h>

void * CopyThread(void *_copyThreadArgs);
struct CopyThreadArgs {
  unsigned int numParts_;
  unsigned int bufferSize_;
  float *source_;
  float *target_;
};

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
    numPlatforms_(0),
    activeIndex_(FIRST),
    useTimers_(false),
    numDevices_(0) {
}

CLHandler::~CLHandler() {

  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin();
       it != OGLTextures_.end(); it++) {
    clReleaseMemObject(it->second);
  }

  for (std::map<cl_uint, MemKernelArg>::iterator it = memKernelArgs_.begin();
       it != memKernelArgs_.end(); it++) {
    clReleaseMemObject(it->second.mem_);
  }

  UnmapPinnedPointers();

  for (std::vector<float*>::iterator it = pinnedPointer_.begin();
       it != pinnedPointer_.end(); ++it) {
    float* temp = *it;
    delete temp;
  }

  for (std::vector<MemKernelArg>::iterator it = pinnedHostMemory_.begin();
       it != pinnedHostMemory_.end(); ++it) {
    clReleaseMemObject(it->mem_);
  }

  for (std::vector<MemKernelArg>::iterator it = deviceBuffer_.begin();
       it != deviceBuffer_.end(); ++it) {
    clReleaseMemObject(it->mem_);
  }

  clReleaseKernel(kernel_);
  clReleaseProgram(program_);
  for (unsigned int i=0; i<NUM_QUEUE_INDICES; ++i) {
    clReleaseCommandQueue(commandQueue_[i]);
  }
  clReleaseContext(context_);

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

  for (unsigned int i=0; i<numDevices_; i++) {
    error_ = clGetDeviceInfo(devices_[i], CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                             sizeof(maxMemAllocSize_), 
                             &maxMemAllocSize_, NULL);
    if (CheckSuccess(error_, "InitDevices()")) {
      INFO("Max memory alloc size: " << maxMemAllocSize_);
    } else {
      return false;
    }
  }

  return true;

}

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
  for (unsigned int i=0; i<NUM_QUEUE_INDICES; ++i) {
    commandQueue_[i] = clCreateCommandQueue(context_, devices_[0], 0, &error_);
    if (!CheckSuccess(error_, "CreateCommandQueue")) {
      return false;
    }
  }
  return true;
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


bool CLHandler::PrepareRaycaster() {

  // Let OpenCL take control of the shared GL objects
  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin(); 
       it != OGLTextures_.end(); it++) {
    error_ = clEnqueueAcquireGLObjects(commandQueue_[EXECUTE], 1, 
                                       &(it->second),
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
                          deviceBuffer_[activeIndex_].size_,
                          &(deviceBuffer_[activeIndex_].mem_));
  if (!CheckSuccess(error_, "Kernel argument for voxel data buffer")) {
    return false;
  }

  return true;
}

bool CLHandler::Finish(QueueIndex _queueIndex) {
  clFinish(commandQueue_[_queueIndex]);
  return true;
}

bool CLHandler::RunRaycaster() {
  
  // TODO Don't hardcode
  size_t globalSize[] = { 512, 512 };
  size_t localSize[] = { 16, 16 };


  if (useTimers_) {
    timer_.start();
  }

  error_ = clEnqueueNDRangeKernel(commandQueue_[EXECUTE], kernel_, 2, NULL, 
                                  globalSize, localSize, 0, NULL, NULL);

  if (useTimers_) {
    clFinish(commandQueue_[EXECUTE]);
    timer_.stop();
    double time = (double)timer_.elapsed().wall / 1.0e9;
    INFO("Kernel execution: " << time << " s");
  }

  if (error_ != CL_SUCCESS) {
    ERROR("Failed to enqueue kernel");
    ERROR(ErrorString(error_));
    return false;
  }

  return true;
}

bool CLHandler::FinishRaycaster() {

  // Make sure kernel is done
  clFinish(commandQueue_[EXECUTE]);

  // Release the shared GL objects
  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin(); 
       it != OGLTextures_.end(); it++) {
    error_ = clEnqueueReleaseGLObjects(commandQueue_[EXECUTE], 1, 
                                       &(it->second), 0, NULL, NULL);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to release GL object");
      ERROR("Failed object: " << it->first);
      ERROR(ErrorString(error_));
      return false;
    }
  }
  return true;
}


bool CLHandler::InitBuffers(unsigned int _argNr,
                            VoxelData<float> *_voxelData) {
  
  voxelDataArgNr_ = _argNr;
  pinnedHostMemory_.resize(NUM_MEM_INDICES);
  deviceBuffer_.resize(NUM_MEM_INDICES);
  pinnedPointer_.resize(NUM_MEM_INDICES);

  bufferSize_ = static_cast<size_t>(
    _voxelData->NumVoxelsPerTimestep()*sizeof(float));

  // Allocate the pinned memory host buffers
  for (std::vector<MemKernelArg>::iterator it = pinnedHostMemory_.begin();
       it != pinnedHostMemory_.end(); ++it) {
    it->mem_ = clCreateBuffer(context_, 
                              CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                              bufferSize_, NULL, &error_);
    it->size_ = sizeof(cl_mem);
    if (!CheckSuccess(error_, "InitBuffers() allocating pinned mem")) {
      return false;
    }                         
  }

  // Allocate the device buffers
  for (std::vector<MemKernelArg>::iterator it = deviceBuffer_.begin();
      it != deviceBuffer_.end(); ++it) {
    it->mem_ = clCreateBuffer(context_, CL_MEM_READ_ONLY,
                              bufferSize_, NULL, &error_);
    it->size_ = sizeof(cl_mem);
    if (!CheckSuccess(error_, "InitBuffers() allocating device mem")) {
      return false;
    }
  }

  // Map standard pointers to reference the pinned host memory
  // TODO loop
  pinnedPointer_[FIRST] = static_cast<float*>(
    clEnqueueMapBuffer(commandQueue_[TRANSFER], pinnedHostMemory_[FIRST].mem_,
                       CL_TRUE, CL_MAP_WRITE, 0, bufferSize_, 0, NULL, 
                       NULL, &error_));   
  pinnedPointer_[SECOND] = static_cast<float*>(
    clEnqueueMapBuffer(commandQueue_[TRANSFER], pinnedHostMemory_[SECOND].mem_,
                       CL_TRUE, CL_MAP_WRITE, 0, bufferSize_, 0, NULL,
                       NULL, &error_));
  if (!CheckSuccess(error_, "InitBuffer() mapping pointers")) {
    return false;
  }

  // Init the first buffer with data from timestep 0
  UpdatePinnedMemory(FIRST, _voxelData, 0);
  WriteToDevice(FIRST);
  SetActiveIndex(FIRST);

  return true;

}

bool CLHandler::SetActiveIndex(MemoryIndex _activeIndex) {
  activeIndex_ = _activeIndex;
}

bool CLHandler::UnmapPinnedPointers() {
  // TODO loop
  error_ = clEnqueueUnmapMemObject(commandQueue_[TRANSFER], 
                                   pinnedHostMemory_[FIRST].mem_,
                                   pinnedPointer_[FIRST],
                                   0, NULL, NULL);
  error_ = clEnqueueUnmapMemObject(commandQueue_[TRANSFER],
                                   pinnedHostMemory_[SECOND].mem_,
                                   pinnedPointer_[SECOND],
                                   0, NULL, NULL);
  return (CheckSuccess(error_, "UnmapPinnedPointers()"));
}


bool CLHandler::UpdatePinnedMemory(MemoryIndex _index,
                                   VoxelData<float> *_voxelData,
                                   unsigned int _timestep) {
  

  unsigned int numVoxels = _voxelData->NumVoxelsPerTimestep();
  unsigned int timestepOffset = _voxelData->TimestepOffset(_timestep);
  float *data = _voxelData->DataPtr(timestepOffset);

  if (numVoxels % copyBufferSize_ != 0) {
    ERROR("numVoxels needs to be evenly divisable by copyBufferSize_");
    return false;
  }

  if (sizeof(float)*numVoxels != bufferSize_) {
    ERROR("Number of voxels and buffer size do not correspond");
    return false;
  }

  if (useTimers_) {
    timer_.start();
  }

  unsigned int numParts = numVoxels/copyBufferSize_;

  /*
  CopyThreadArgs cta;
  cta.numParts_ = numParts;
  cta.bufferSize_ = copyBufferSize_;
  cta.source_ = data;
  cta.target_ = pinnedPointer_[_index];

  pthread_create(&copyThread_, NULL, CopyThread, 
                 reinterpret_cast<void*>(&cta));
   */
  
  for (unsigned int i=0; i<numParts; ++i) {
    std::copy(data+i*copyBufferSize_, 
              data+(i+1)*copyBufferSize_, 
              pinnedPointer_[_index]+i*copyBufferSize_);
  }
  

  if (useTimers_) {
    double time = (double)timer_.elapsed().wall / 1.0e9;
    double size = bufferSize_ / BYTES_PER_GB;
    INFO("Copy to pinned mem: " << time << " s, " << size/time << "GB/s");
  }

  return true;
}

bool CLHandler::WriteToDevice(MemoryIndex _index) {
  // TODO non-blocking

  cl_bool blocking = CL_FALSE;
  if (useTimers_) {   
    timer_.start();
    blocking = CL_TRUE;
  }

  error_ = clEnqueueWriteBuffer(commandQueue_[TRANSFER], 
                                deviceBuffer_[_index].mem_,
                                blocking, 0, bufferSize_, 
                                pinnedPointer_[_index], 0, NULL, NULL);
  
  if (useTimers_) {
    timer_.stop();
    double time = (double)timer_.elapsed().wall / 1.0e9;
    double size = bufferSize_ / BYTES_PER_GB;
    INFO("Write to device: " << time << " s, " << size/time << " GB/s");
  }

  return (CheckSuccess(error_, "WriteToDevice()"));

}
  
bool CLHandler::ToggleTimers() {
  useTimers_ = !useTimers_;
}
                                     
void * CopyThread(void *_copyThreadArgs) {

  CopyThreadArgs cta = *reinterpret_cast<CopyThreadArgs*>(_copyThreadArgs);
 
  // Lock mutex and wait for signal
  // Mutex will automatically be unlocked while waiting
  //pthread_mutex_lock(&copyMutex_);
  //pthread_cond_wait(&copyCond_, &copyMutex_);
  
  // When signal is reciever, start copying
  for (unsigned int i=0; i<cta.numParts_; ++i) {
    std::copy(cta.source_+i*cta.bufferSize_, 
              cta.source_+(i+1)*cta.bufferSize_, 
              cta.target_+i*cta.bufferSize_);
  }
 
  //pthread_mutex_unlock(&copyMutex_);
  pthread_exit(NULL);

}

bool CLHandler::JoinCopyThread() {
  pthread_join(copyThread_, NULL);
  return true;
}
