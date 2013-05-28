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
#include <algorithm>

using namespace osp;

CLHandler::CLHandler() 
  : error_(CL_SUCCESS), 
    numPlatforms_(0),
    numDevices_(0),
    activeIndex_(FIRST),
    voxelDataArgNr_(0),
    useTimers_(false) {
}

CLHandler * CLHandler::New() {
  return new CLHandler();
}

CLHandler::~CLHandler() {

  for (auto it=OGLTextures_.begin(); it!=OGLTextures_.end(); ++it) {
    clReleaseMemObject(it->second);
  }

  for (auto it=memArgs_.begin(); it!=memArgs_.end(); ++it) {
    clReleaseMemObject(it->second.mem_);
  }

  for (auto it=hostTextures_.begin(); it!=hostTextures_.end(); ++it) {
    delete *it;
  }

  clReleaseKernel(kernel_);
  clReleaseProgram(program_);
  for (unsigned int i=0; i<NUM_QUEUE_INDICES; ++i) {
    clReleaseCommandQueue(commandQueues_[i]);
  }
  clReleaseContext(context_);
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
  
  // TODO support more platforms?
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
  if (CheckSuccess(error_, "InitDevices() getting IDs")) {
    INFO("Number of CL devices: " << numDevices_);
  } else {
    return false;
  }

  // Loop over found devices and print info
  for (unsigned int i=0; i<numDevices_; i++) {
    error_ = clGetDeviceInfo(devices_[i], CL_DEVICE_NAME, 
                             sizeof(deviceName_), deviceName_, NULL);
    if (CheckSuccess(error_, "InitDevices() printing info")) {
      INFO("Device " << i << " name: " << deviceName_);
    } else {
      return false;
    }
  }

  for (unsigned int i=0; i<numDevices_; i++) {
    error_ = clGetDeviceInfo(devices_[i], CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                             sizeof(maxMemAllocSize_[i]), 
                             &(maxMemAllocSize_[i]), NULL);
    if (CheckSuccess(error_, "InitDevices() finding maxMemAllocSize")) {
      //INFO("Max memory alloc size: " << maxMemAllocSize_[i]);
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
                                       NULL, &error_);
  free(source);
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
      free(log);                       
    }

    return false;
  }
}


bool CLHandler::CreateKernel() {
  kernel_ = clCreateKernel(program_, "Raycaster", &error_);
  return CheckSuccess(error_, "CreateKernel()");
}


bool CLHandler::CreateCommandQueues() {
  for (unsigned int i=0; i<NUM_QUEUE_INDICES; ++i) {
    commandQueues_[i] = clCreateCommandQueue(context_, devices_[0], 0, &error_);
    if (!CheckSuccess(error_, "CreateCommandQueue")) {
      return false;
    }
  }
  return true;
}


bool CLHandler::InitBuffers(unsigned int _argNr,
                            VoxelData<float> *_voxelData) {

  voxelDataArgNr_ = _argNr;  
  unsigned int numVoxels = _voxelData->NumVoxelsPerTimestep();
  unsigned int index;

  bufferSize_ = numVoxels*sizeof(float);

  // Create and initialize PBOs
  pixelBufferObjects_.resize(NUM_MEM_INDICES);
  index = 0;
  for (auto it=pixelBufferObjects_.begin(); it!=pixelBufferObjects_.end(); ++it) {
    glGenBuffers(1, &(*it));
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, *it);
    unsigned int timestepOffset = _voxelData->TimestepOffset(index);
    float *data = _voxelData->DataPtr(timestepOffset);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, numVoxels*sizeof(float),
                 reinterpret_cast<GLvoid*>(data), GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    index++;
    if (CheckGLError("InitBuffers() Init PBOs") != GL_NO_ERROR) {
      return false;
    }
  }

  // Create host-side textures
  hostTextures_.resize(NUM_MEM_INDICES);
  std::vector<unsigned int> dims;
  dims.push_back(_voxelData->ADim());
  dims.push_back(_voxelData->BDim());
  dims.push_back(_voxelData->CDim());
  index = 0;
  for (std::vector<Texture3D*>::iterator it = hostTextures_.begin();
      it != hostTextures_.end(); ++it) {
      *it = Texture3D::New(dims);
      unsigned int timestepOffset = _voxelData->TimestepOffset(index);
      float *data = _voxelData->DataPtr(timestepOffset);
      (*it)->Init(data);
      index++;
  }

  // Allocate device side textures
  deviceTextures_.resize(NUM_MEM_INDICES);
  index = 0;
  for (auto it=deviceTextures_.begin(); it!=deviceTextures_.end(); ++it) {
    it->mem_ = clCreateFromGLTexture3D(context_,
                                      CL_MEM_READ_ONLY,
                                      GL_TEXTURE_3D,
                                      0, 
                                      hostTextures_[index]->Handle(),
                                      &error_);
    it->size_ = sizeof(cl_mem);
    index++;
    if (!CheckSuccess(error_, "InitBuffers() init device textures")) {
      return false;
    }
  }
  
  return true;
}


bool CLHandler::UpdateHostMemory(MemIndex _memIndex,
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
  

  // Map buffer to CPU controlled memory
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBufferObjects_[_memIndex]);
  float *mappedPointer = reinterpret_cast<float*>(
    glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));

  // Copy data in parts for perfomance reasons
  unsigned int numParts = numVoxels/copyBufferSize_;
  for (unsigned int i=0; i<numParts; ++i) {
    std::copy(data+i*copyBufferSize_,
              data+(i+1)*copyBufferSize_,
              mappedPointer+i*copyBufferSize_);
  }

  if (useTimers_) {
    double time = (double)timer_.elapsed().wall / 1.0e9;
    double size = bufferSize_ / BYTES_PER_GB;
    INFO("Copy to PBO mapped mem: " << time << " s, " << size/time << "GB/s");
  }


  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
   
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
  if (memArgs_.find((cl_uint)_argNr) != memArgs_.end()) {
    memArgs_.erase((cl_uint)_argNr);
  }

  MemArg mka;
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

  memArgs_.insert(std::make_pair((cl_uint)_argNr, mka));
  return true;
}


bool CLHandler::AddConstants(unsigned int _argNr, 
                              KernelConstants *_kernelConstants) {

  // Delete any old data already bound to argument index
  if (memArgs_.find((cl_uint)_argNr) != memArgs_.end()) {
    memArgs_.erase((cl_uint)_argNr);
  }

  MemArg mka;
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

  memArgs_.insert(std::make_pair((cl_uint)_argNr, mka));
  return true;
}


bool CLHandler::FinishQueue(QueueIndex _queueIndex) {
  clFinish(commandQueues_[_queueIndex]);
  return true;
}


bool CLHandler::WriteToDevice(MemIndex _memIndex) {

  // Read and write from pixel buffer
  glBindTexture(GL_TEXTURE_3D, hostTextures_[_memIndex]->Handle());
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pixelBufferObjects_[_memIndex]);  
  
  glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0,
                  hostTextures_[_memIndex]->Dim(0),
                  hostTextures_[_memIndex]->Dim(1),
                  hostTextures_[_memIndex]->Dim(2),
                  GL_RED, GL_FLOAT, 0);

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glBindTexture(GL_TEXTURE_3D, 0);
 
  return CheckGLError("CLHandlerPBO: WriteToDevice()") == GL_NO_ERROR;
                      
}


bool CLHandler::PrepareRaycaster() {

  // Let OpenCL take control of the shared GL textures
  for (auto it = OGLTextures_.begin(); it != OGLTextures_.end(); ++it) {
    error_ = clEnqueueAcquireGLObjects(commandQueues_[EXECUTE], 1, 
                                       &(it->second),
                                       0, NULL, NULL);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to enqueue GL object aqcuisition");
      ERROR("Failing object: " << it->first);
      ERROR(ErrorString(error_));
      return false;
    }
  }

  // Let OpenCL take control of the shaded 3D OGL objects
  for (std::vector<MemArg>::iterator it = deviceTextures_.begin();
       it != deviceTextures_.end(); ++it) {
    error_ = clEnqueueAcquireGLObjects(commandQueues_[EXECUTE], 1,
                                       &(it->mem_), 0, NULL, NULL);
    if (!CheckSuccess(error_, "PrepareRaycaster() aquire 3D GL objects")) {
      return false;
    }
  }

  // Set up kernel arguments of non-shared items
  for (auto it=memArgs_.begin(); it!=memArgs_.end(); ++it) {
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
  for (auto it=OGLTextures_.begin(); it!=OGLTextures_.end(); ++it) {
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
                          deviceTextures_[activeIndex_].size_,
                          &(deviceTextures_[activeIndex_].mem_));
  if (!CheckSuccess(error_, "Kernel argument for voxel data buffer")) {
    return false;
  }

  return true;
}


bool CLHandler::FinishRaycaster() {

  // Make sure kernel is done
  clFinish(commandQueues_[EXECUTE]);

  // Release the shared GL objects
  for (auto it=OGLTextures_.begin(); it!=OGLTextures_.end(); ++it) {
    error_ = clEnqueueReleaseGLObjects(commandQueues_[EXECUTE], 1, 
                                       &(it->second), 0, NULL, NULL);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to release GL object");
      ERROR("Failed object: " << it->first);
      ERROR(ErrorString(error_));
      return false;
    }
  }

  for (std::vector<MemArg>::iterator it = deviceTextures_.begin();
      it != deviceTextures_.end(); ++it) {
    error_ = clEnqueueReleaseGLObjects(commandQueues_[EXECUTE], 1,
                                      &(it->mem_), 0, NULL, NULL);
    if (!CheckSuccess(error_, "FinishRaycaster()")) {
      return false;
   }
  }

  return true;
}

bool CLHandler::LaunchRaycaster() {
  
  // TODO Don't hardcode
  size_t globalSize[] = { 512, 512 };
  size_t localSize[] = { 16, 16 };


  if (useTimers_) {
    timer_.start();
  }

  error_ = clEnqueueNDRangeKernel(commandQueues_[EXECUTE], kernel_, 2, NULL, 
                                  globalSize, localSize, 0, NULL, NULL);

  if (useTimers_) {
    clFinish(commandQueues_[EXECUTE]);
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


bool CLHandler::SetActiveIndex(MemIndex _activeIndex) {
  activeIndex_ = _activeIndex;
}

  
bool CLHandler::ToggleTimers() {
  useTimers_ = !useTimers_;
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
