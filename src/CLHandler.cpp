/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 * 
 */

#include <GL/glew.h>
#include <GL/glx.h>
#include <Utils.h>
#include <CLHandler.h>
#include <Texture2D.h>
#include <Texture3D.h>
#include <VoxelData.h>
#include <TransferFunction.h>
#include <sstream>
#include <fstream>
#include <cmath>

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

CLHandler::CLHandler() : error_(CL_SUCCESS) {
}

std::string CLHandler::GetErrorString(cl_int _error) {
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

bool CLHandler::Init() {
  // Find platform(s)
  error_ = clGetPlatformIDs(32, platforms_, &numPlatforms_);
  if (error_ == CL_SUCCESS) {
    INFO("Number of CL platforms: " << numPlatforms_);
  } else {
    ERROR("Failed to get CL platforms");
    ERROR(GetErrorString(error_));
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
    ERROR(GetErrorString(error_));
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
      ERROR(GetErrorString(error_));
      return false;
    }
  }

  return true;
}

bool CLHandler::CreateContext() {

  // Create an OpenCL context with a reference to an OpenGL context
  cl_context_properties contextProperties[] = {
    CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
    CL_CONTEXT_PLATFORM, (cl_context_properties)platforms_[0],
    0};
 
  // TODO Use more than one device
  context_ = clCreateContext(contextProperties, 1, &devices_[0], NULL,
                             NULL, &error_);
  if (error_ == CL_SUCCESS) {
    INFO("CL context set up properly");
  } else {
    ERROR("Failed to set up CL context");
    ERROR(GetErrorString(error_));
    return false;
  }

  return true;
}

bool CLHandler::AddTexture2D(unsigned int _argIndex, Texture2D *_texture, 
                              bool _readOnly) {
  
  // Remove anything already associated with argument index
  if (GLTextures_.find((cl_uint)_argIndex) != GLTextures_.end()) {
    INFO("Erasing texture at kernel argument " << _argIndex);
    GLTextures_.erase((cl_uint)_argIndex);
  }
 
  cl_mem_flags flag = _readOnly ? CL_MEM_READ_ONLY : CL_MEM_WRITE_ONLY;
  cl_mem texture = clCreateFromGLTexture2D(context_,
                                           flag,
                                           GL_TEXTURE_2D,
                                           0,
                                           _texture->Handle(),
                                           &error_);
  if (error_ != CL_SUCCESS) {
    ERROR("Failed to add GL texture at argument index " << _argIndex);
    ERROR(GetErrorString(error_));
    return false;
  }

  INFO("Inserting kernel argument " << _argIndex);
  GLTextures_.insert(std::make_pair((cl_uint)_argIndex, texture));

  return true;
}


bool CLHandler::AddTexture3D(unsigned int _argIndex, Texture3D *_texture,
                             bool _readOnly) {

  // Remove anything already associated with argument index
  if (GLTextures_.find((cl_uint)_argIndex) != GLTextures_.end()) {
    INFO("Erasing texture at kernel argument " << _argIndex);
    GLTextures_.erase((cl_uint)_argIndex);
  }

  cl_mem_flags flag = _readOnly ? CL_MEM_READ_ONLY : CL_MEM_WRITE_ONLY;
  cl_mem texture = clCreateFromGLTexture3D(context_, flag, GL_TEXTURE_3D,
                                           0, _texture->Handle(), &error_);

  if (error_ != CL_SUCCESS) {
    ERROR("Failed to create cl_mem object for argument index " << _argIndex);
    ERROR(GetErrorString(error_));
    return false;
  }
  
  INFO("Inserting kernel argument " << _argIndex);
  GLTextures_.insert(std::make_pair((cl_uint)_argIndex, texture));

  return true;
}

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
  if (error_ == CL_SUCCESS) {
    INFO("Created CL program successfully");
    return true;
  } else {
    ERROR("Failed to create CL program");
    ERROR(GetErrorString(error_));
    return false;
  }
}

bool CLHandler::BuildProgram() {

  DEBUG("Building CL program...");

  error_ = clBuildProgram(program_, (cl_uint)0, NULL, NULL, NULL, NULL);

  if (error_ == CL_SUCCESS) {
    INFO("Built CL program successfully");
    return true;
  } else {
    ERROR("Failed to build CL program");
    ERROR(GetErrorString(error_));

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
  if (error_ == CL_SUCCESS) {
    INFO("Created CL kernel successfully");
    return true;
  } else {
    ERROR("Failed to create CL kernel");
    ERROR(GetErrorString(error_));
    return false;
  }
}

bool CLHandler::CreateCommandQueue() {
  commandQueue_ = clCreateCommandQueue(context_, devices_[0], 0, &error_);
  if (error_ == CL_SUCCESS) {
    INFO("Created CL command queue successfully");
    return true;
  } else {
    ERROR("Failed to create CL command queue");
    ERROR(GetErrorString(error_));
    return false;
  }
}

bool CLHandler::BindTransferFunction(unsigned int _argIndex, 
                                     TransferFunction *_tf) {

  // Remove old TF already bound to this argument index 
  if (memKernelArgs_.find((cl_uint)_argIndex) != memKernelArgs_.end()) {
    memKernelArgs_.erase((cl_uint)_argIndex);
  }

  MemKernelArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.value_ = clCreateBuffer(context_,
                             CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             _tf->Width()*4*sizeof(float),
                             _tf->FloatData(),
                             &error_);

  if (error_ != CL_SUCCESS) {
    ERROR("Failed to bind transfer function");
    ERROR(GetErrorString(error_));
    return false;
  }

  memKernelArgs_.insert(std::make_pair((cl_uint)_argIndex, mka));
  return true;
}

bool CLHandler::BindVoxelData(unsigned int _argIndex, 
                              VoxelData<float> *_voxelData) { 
  
  // Remove old data already bound to this argument index
  if (memKernelArgs_.find((cl_uint)_argIndex) != memKernelArgs_.end()) {
    memKernelArgs_.erase((cl_uint)_argIndex);
  }

  MemKernelArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.value_ = clCreateBuffer(context_,
                              CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              _voxelData->NumVoxelsTotal()*sizeof(float),
                              _voxelData->DataPtr(),
                              &error_);

  if (error_ != CL_SUCCESS) {
    ERROR("Failed to bind voxel data to arg index " << _argIndex);
    ERROR(GetErrorString(error_));
    return false;
  }

  memKernelArgs_.insert(std::make_pair((cl_uint)_argIndex, mka));
  return true;
}

bool CLHandler::BindConstants(unsigned int _argIndex, 
                              KernelConstants *_kernelConstants) {

  // Delete any old data already bound to argument index
  if (memKernelArgs_.find((cl_uint)_argIndex) != memKernelArgs_.end()) {
    memKernelArgs_.erase((cl_uint)_argIndex);
  }

  MemKernelArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.value_ = clCreateBuffer(context_,
                              CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              sizeof(KernelConstants),
                              _kernelConstants,
                              &error_);

  if (error_ != CL_SUCCESS) {
    ERROR("Failed to bind constants to kernel arg index " << _argIndex);
    ERROR(GetErrorString(error_));
    return false;
  }

  memKernelArgs_.insert(std::make_pair((cl_uint)_argIndex, mka));
  return true;
}

bool CLHandler::BindTimestepOffset(unsigned int _argIndex,
                                   unsigned int _timestepOffset) {

  // Delete old data bound to argument index
  if (uintArgs_.find((cl_uint)_argIndex) != uintArgs_.end()) {
    uintArgs_.erase((cl_uint)_argIndex);
  }

  uintArgs_.insert(std::make_pair((cl_uint)_argIndex, _timestepOffset));
  return true;
}

bool CLHandler::RunRaycaster() {
  
  // TODO Don't hardcode
  size_t globalSize[] = { 512, 512 };
  size_t localSize[] = { 16, 16 };

  // Let OpenCL take control of the shared GL objects
  for (std::map<cl_uint, cl_mem>::iterator it = GLTextures_.begin(); 
       it != GLTextures_.end(); 
       it++) {
    error_ = clEnqueueAcquireGLObjects(commandQueue_, 1, &(it->second),
                                       0, NULL, NULL);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to enqueue GL object aqcuisition");
      ERROR("Failing object: " << it->first);
      ERROR(GetErrorString(error_));
      return false;
    }
  }

 
  // Set up kernel arguments of non-shared items
  for (std::map<cl_uint, MemKernelArg>::iterator it = memKernelArgs_.begin();
       it != memKernelArgs_.end(); 
       it++) {
    error_ = clSetKernelArg(kernel_,
                            it->first,
                            (it->second).size_,
                            &((it->second).value_));
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to set kernel argument " << it->first);
      ERROR(GetErrorString(error_));
      return false;
    }
  }
  
  
  // Set up kernel arguments for textures
  for (std::map<cl_uint, cl_mem>::iterator it = GLTextures_.begin(); 
       it != GLTextures_.end(); 
       it++) {
    error_ = clSetKernelArg(kernel_,
                            it->first,
                            sizeof(cl_mem),
                            &(it->second));
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to set kernel argument " << it->first);
      ERROR(GetErrorString(error_));
      return false;
    }
  }

  // Set up unsigned integer kernel arguments
  for (std::map<cl_uint, unsigned int>::iterator it = uintArgs_.begin();
       it != uintArgs_.end();
       it++) {
    error_ = clSetKernelArg(kernel_,
                            it->first,
                            sizeof(unsigned int),
                            &(it->second));
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to set kernel argument " << it->first);
      ERROR(GetErrorString(error_));
      return false;
    }
  }

  // Set up kernel execution
  error_ = clEnqueueNDRangeKernel(commandQueue_, kernel_, 2, NULL, globalSize,
                                  localSize, 0, NULL, NULL);
  if (error_ != CL_SUCCESS) {
    ERROR("Failed to enqueue kernel");
    ERROR(GetErrorString(error_));
    return false;
  }

  // Release the shared GL objects
  for (std::map<cl_uint, cl_mem>::iterator it = GLTextures_.begin(); 
       it!=GLTextures_.end(); 
       it++) {
    error_ = clEnqueueReleaseGLObjects(commandQueue_, 1, &(it->second), 0,
                                       NULL, NULL);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to release GL object");
      ERROR("Failed object: " << it->first);
      ERROR(GetErrorString(error_));
      return false;
    }
  }
  
  return true;
}
                                     
