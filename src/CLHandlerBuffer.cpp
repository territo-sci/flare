/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <CLHandlerBuffer.h>
#include <Utils.h>

using namespace osp;

CLHandlerBuffer * CLHandlerBuffer::New() {
  return new CLHandlerBuffer();
}

CLHandlerBuffer::CLHandlerBuffer() : CLHandler() {}

CLHandlerBuffer::~CLHandlerBuffer() {
  
  for (std::vector<float*>::iterator it = pinnedPointers_.begin();
       it != pinnedPointers_.end(); ++it) {
    float* temp = *it;
    delete temp;
  }

  for (std::vector<MemArg>::iterator it = pinnedHostMemory_.begin();
       it != pinnedHostMemory_.end(); ++it) {
    clReleaseMemObject(it->mem_);
  }

  for (std::vector<MemArg>::iterator it = deviceBuffers_.begin();
       it != deviceBuffers_.end(); ++it) {
    clReleaseMemObject(it->mem_);
  }
  
}

bool CLHandlerBuffer::InitBuffers(unsigned int _argNr,
                            VoxelData<float> *_voxelData) {

  voxelDataArgNr_ = _argNr;

  pinnedHostMemory_.resize(NUM_MEM_INDICES);
  deviceBuffers_.resize(NUM_MEM_INDICES);
  pinnedPointers_.resize(NUM_MEM_INDICES);

  bufferSize_ = static_cast<size_t>(
    _voxelData->NumVoxelsPerTimestep()*sizeof(float));

  // Allocate the pinned memory host buffers
  for (std::vector<MemArg>::iterator it = pinnedHostMemory_.begin();
       it != pinnedHostMemory_.end(); ++it) {
    it->mem_ = clCreateBuffer(context_,
                              CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
                              bufferSize_, NULL, &error_);
    it->size_ = sizeof(cl_mem);
    if (!CheckSuccess(error_, "InitBuffers() allocating pinned mem")) {
      return false;
    }
  }

  // Allocate device buffers
  for (std::vector<MemArg>::iterator it = deviceBuffers_.begin();
       it != deviceBuffers_.end(); ++it) {
    it->mem_ = clCreateBuffer(context_, CL_MEM_READ_ONLY,
                              bufferSize_, NULL, &error_);
    it->size_ = sizeof(cl_mem);
    if (!CheckSuccess(error_, "InitBuffers() allocating device mem")) {
      return false;
    }
  }

  // Map standard pointers to reference the pinned host memory
  for (unsigned int i=0; i<NUM_MEM_INDICES; ++i) {
    pinnedPointers_[i] = static_cast<float*>(
      clEnqueueMapBuffer(commandQueues_[TRANSFER], 
                         pinnedHostMemory_[i].mem_,
                         CL_TRUE, CL_MAP_WRITE, 0, bufferSize_, 0, 
                         NULL, NULL, &error_));
    if (!CheckSuccess(error_, "InitBuffers() mapping pointers")) {
      return false;
    }
  }

  // Init the first buffer with data from timestep 0
  if (!UpdateHostMemory(FIRST, _voxelData, 0)) return false;
  if (!WriteToDevice(FIRST)) return false;
  SetActiveIndex(FIRST);

  return true;
}


bool CLHandlerBuffer::UpdateHostMemory(MemIndex _memIndex,
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

  // Copy in parts for performance reasons 
  for (unsigned int i=0; i<numParts; ++i) {
    std::copy(data+i*copyBufferSize_, 
              data+(i+1)*copyBufferSize_, 
              pinnedPointers_[_memIndex]+i*copyBufferSize_);
  }
  
  if (useTimers_) {
    double time = (double)timer_.elapsed().wall / 1.0e9;
    double size = bufferSize_ / BYTES_PER_GB;
    INFO("Copy to pinned mem: " << time << " s, " << size/time << "GB/s");
  }

  return true;
}


bool CLHandlerBuffer::WriteToDevice(MemIndex _memIndex) {

  cl_bool blocking = CL_FALSE;
  if (useTimers_) {   
    timer_.start();
    blocking = CL_TRUE;
  }

  error_ = clEnqueueWriteBuffer(commandQueues_[TRANSFER], 
                                deviceBuffers_[_memIndex].mem_,
                                blocking, 0, bufferSize_, 
                                pinnedPointers_[_memIndex], 0, NULL, NULL);
  
  if (useTimers_) {
    timer_.stop();
    double time = (double)timer_.elapsed().wall / 1.0e9;
    double size = bufferSize_ / BYTES_PER_GB;
    INFO("Write to device: " << time << " s, " << size/time << " GB/s");
  }

  return (CheckSuccess(error_, "WriteToDevice()"));
}

bool CLHandlerBuffer::PrepareRaycaster() {

  // Let OpenCL take control of the shared GL objects
  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin(); 
       it != OGLTextures_.end(); it++) {
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

  // Set up kernel arguments of non-shared items
  for (std::map<cl_uint, MemArg>::iterator it = memArgs_.begin();
       it != memArgs_.end(); it++) {
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
                          deviceBuffers_[activeIndex_].size_,
                          &(deviceBuffers_[activeIndex_].mem_));
  if (!CheckSuccess(error_, "Kernel argument for voxel data buffer")) {
    return false;
  }

  return true;
}


bool CLHandlerBuffer::FinishRaycaster() {

  // Make sure kernel is done
  clFinish(commandQueues_[EXECUTE]);

  // Release the shared GL objects
  for (std::map<cl_uint, cl_mem>::iterator it = OGLTextures_.begin(); 
       it != OGLTextures_.end(); it++) {
    error_ = clEnqueueReleaseGLObjects(commandQueues_[EXECUTE], 1, 
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
