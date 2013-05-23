/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <GL/glew.h>
#include <CLHandlerPBO.h>
#include <Texture3D.h>
#include <PixelBuffer.h>
#include <Utils.h>

using namespace osp;

CLHandlerPBO * CLHandlerPBO::New() {
  return new CLHandlerPBO();
}

CLHandlerPBO::CLHandlerPBO() : CLHandler() { }

CLHandlerPBO::~CLHandlerPBO() {

  /*
  for (std::vector<PixelBuffer*>::iterator it = pixelBuffers_.begin();
       it != pixelBuffers_.end(); ++it) {
    delete *it;
  }
  */

  for (std::vector<Texture3D*>::iterator it = hostTextures_.begin();
       it != hostTextures_.end(); ++it) {
    delete *it;
  }

  for (std::vector<MemArg>::iterator it = deviceTextures_.begin();
       it != deviceTextures_.end(); ++it) {
    clReleaseMemObject(it->mem_);
  }
}

bool CLHandlerPBO::InitBuffers(unsigned int _argNr,
                               VoxelData<float> *_voxelData) {

  voxelDataArgNr_ = _argNr;  
  unsigned int numVoxels = _voxelData->NumVoxelsPerTimestep();
  unsigned int index;

  bufferSize_ = numVoxels*sizeof(float);

  // Create and initialize PBOs
  pixelBufferObjects_.resize(NUM_MEM_INDICES);
  index = 0;
  for (std::vector<unsigned int>::iterator it = pixelBufferObjects_.begin();
       it != pixelBufferObjects_.end(); ++it) {
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
  for (std::vector<MemArg>::iterator it = deviceTextures_.begin();
       it != deviceTextures_.end(); ++it) {
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


bool CLHandlerPBO::UpdateHostMemory(MemIndex _memIndex,
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

bool CLHandlerPBO::WriteToDevice(MemIndex _memIndex) {

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


bool CLHandlerPBO::PrepareRaycaster() {

  // Let OpenCL take control of the shared 2D GL objects
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
                          deviceTextures_[activeIndex_].size_,
                          &(deviceTextures_[activeIndex_].mem_));
  if (!CheckSuccess(error_, "Kernel argument for voxel data buffer")) {
    return false;
  }

  return true;
}


bool CLHandlerPBO::FinishRaycaster() {

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
