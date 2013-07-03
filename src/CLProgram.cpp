/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <GL/glew.h>
#include <CLProgram.h>
#include <CLManager.h>
#include <TransferFunction.h>
#include <Texture.h>
#include <Utils.h>

using namespace osp;

CLProgram::CLProgram(const std::string &_programName, CLManager *_clManager) 
  : programName_(_programName), clManager_(_clManager) {
}

CLProgram * CLProgram::New(const std::string &_programName, 
                           CLManager *_clManager) {
  return new CLProgram(_programName, _clManager);
}

CLProgram::~CLProgram() {
  clReleaseKernel(kernel_);
  clReleaseProgram(program_);
}

bool CLProgram::CreateProgram(std::string _fileName) {
  int numChars;
  char *source = ReadSource(_fileName, numChars);
  program_ = clCreateProgramWithSource(clManager_->context_, 1, 
                                       (const char**)&source,
                                       NULL, &error_);
  free(source);
  return (error_ == CL_SUCCESS);
}


bool CLProgram::BuildProgram() {
  error_ = clBuildProgram(program_, (cl_uint)0,
                          NULL, NULL, NULL, NULL);
  if (error_ != CL_SUCCESS) {
    // Print build log
    char * log;
    size_t logSize = 0;
    clGetProgramBuildInfo(program_, clManager_->devices_[0], 
                          CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
    if (logSize > 0) {
      INFO("Build log:");
      log = new char[logSize+1];
      clGetProgramBuildInfo(program_, clManager_->devices_[0], 
                            CL_PROGRAM_BUILD_LOG, logSize, log, NULL);
      log[logSize] = '\0';
      INFO(log);
      free(log);                       
    }
    return false;
  }
  return true;
}


bool CLProgram::CreateKernel() {
  kernel_ = clCreateKernel(program_, programName_.c_str(), &error_);
  return (error_ == CL_SUCCESS);
}


bool CLProgram::AddTexture(unsigned int _argNr, Texture *_texture,
                           GLuint _textureType,
                           cl_mem_flags _permissions) {

  // Remove anything already associated with argument index
  if (OGLTextures_.find((cl_uint)_argNr) != OGLTextures_.end()) {
    OGLTextures_.erase((cl_uint)_argNr);
  }
 
  cl_mem texture;
  switch (_textureType) {
    case GL_TEXTURE_1D:
      ERROR("Texture 1D unimplemented");
      return false;
      break;
    case GL_TEXTURE_2D:
      texture = clCreateFromGLTexture2D(clManager_->context_, _permissions, 
                                        GL_TEXTURE_2D, 0, 
                                        _texture->Handle(), &error_);
      break;
    case GL_TEXTURE_3D:
      texture = clCreateFromGLTexture3D(clManager_->context_, _permissions, 
                                        GL_TEXTURE_3D, 0, 
                                        _texture->Handle(), &error_);
      break;
    default:
      ERROR("Unknown GL texture type");
      return false;
  }

  if (error_ != CL_SUCCESS) return false;
 
  OGLTextures_.insert(std::make_pair((cl_uint)_argNr, texture));

  return true;

}


bool CLProgram::AddTransferFunction(unsigned int _argNr,
                                    TransferFunction *_tf) {
  // Remove old TF already bound to this argument index 
  if (memArgs_.find((cl_uint)_argNr) != memArgs_.end()) {
    memArgs_.erase((cl_uint)_argNr);
  }

  // TODO implement TF using 1D texture
  MemArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.mem_ = clCreateBuffer(clManager_->context_,
                             CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                             _tf->Width()*4*sizeof(float),
                             _tf->FloatData(),
                             &error_);
 
  if (error_ != CL_SUCCESS) return false;
  memArgs_.insert(std::make_pair((cl_uint)_argNr, mka));
  return true;
}


bool CLProgram::AddKernelConstants(unsigned int _argNr,
                                   KernelConstants *_kernelConstants) {
  // Delete any old data already bound to argument index
  if (memArgs_.find((cl_uint)_argNr) != memArgs_.end()) {
    memArgs_.erase((cl_uint)_argNr);
  }

  MemArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.mem_ = clCreateBuffer(clManager_->context_,
                            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            sizeof(KernelConstants),
                            _kernelConstants,
                            &error_);

  if (error_ != CL_SUCCESS) return false;
  memArgs_.insert(std::make_pair((cl_uint)_argNr, mka));
  return true;

}


bool CLProgram::AddTraversalConstants(unsigned int _argNr,
                                  TraversalConstants *_traversalConstants) {
  // Delete any old data already bound to argument index
  if (memArgs_.find((cl_uint)_argNr) != memArgs_.end()) {
    memArgs_.erase((cl_uint)_argNr);
  }

  MemArg mka;
  mka.size_ = sizeof(cl_mem);
  mka.mem_ = clCreateBuffer(clManager_->context_,
                            CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                            sizeof(TraversalConstants),
                            _traversalConstants,
                            &error_);

  if (error_ != CL_SUCCESS) return false;
  memArgs_.insert(std::make_pair((cl_uint)_argNr, mka));
  return true;
}

bool CLProgram::AddBuffer(unsigned int _argNr,
                          void *_hostPtr,
                          unsigned int _sizeInBytes,
                          cl_mem_flags _allocMode,
                          cl_mem_flags _permissions) {
  if (memArgs_.find((cl_uint)_argNr) != memArgs_.end()) {
    memArgs_.erase((cl_uint)_argNr);
  }
  MemArg ma;
  ma.size_ = sizeof(cl_mem);
  ma.mem_ = clCreateBuffer(clManager_->context_,
                           _allocMode | _permissions,
                           (size_t)_sizeInBytes,
                           _hostPtr,
                           &error_);
  if (error_ != CL_SUCCESS) return false;
  memArgs_.insert(std::make_pair((cl_uint)_argNr, ma));
  return true;
}

bool CLProgram::ReadBuffer(unsigned int _argNr,
                           void *_hostPtr,
                           unsigned int _sizeInBytes,
                           cl_bool _blocking) {
  if (memArgs_.find((cl_uint)_argNr) == memArgs_.end()) {
    ERROR("ReadArray(): Could not find mem arg " << _argNr);
    return false;
  }
  error_ = clEnqueueReadBuffer(
    clManager_->commandQueues_[CLManager::EXECUTE],
    memArgs_[(cl_uint)_argNr].mem_, _blocking, 0, _sizeInBytes,
    _hostPtr, 0, NULL, NULL);
  return (error_ == CL_SUCCESS);
}

bool CLProgram::PrepareProgram() {

  // Let OpenCL take control of the shared GL textures
  for (auto it = OGLTextures_.begin(); it != OGLTextures_.end(); ++it) {
    error_ = clEnqueueAcquireGLObjects(
      clManager_->commandQueues_[CLManager::EXECUTE], 1, 
      &(it->second), 0, NULL, NULL);

    if (error_ != CL_SUCCESS) {
      ERROR("Failed to enqueue GL object aqcuisition");
      ERROR("Failing object: " << it->first);
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
      ERROR("Failed to set texture kernel argument " << it->first);
      return false;
    }
  }

  return true;
}


bool CLProgram::LaunchProgram() {
  // TODO don't hardcode
  size_t globalSize[] = { 512, 512 };
  size_t localSize[] = { 16, 16 };

  error_ = clEnqueueNDRangeKernel(
    clManager_->commandQueues_[CLManager::EXECUTE], kernel_, 2, NULL,
    globalSize, localSize, 0, NULL, NULL);
  return (error_ == CL_SUCCESS);
}

bool CLProgram::FinishProgram() {
  // Make sure kernel is done
  clFinish(clManager_->commandQueues_[CLManager::EXECUTE]);

  // Release shared OGL objects
  for (auto it=OGLTextures_.begin(); it!=OGLTextures_.end(); ++it) {
    error_ = clEnqueueReleaseGLObjects(
      clManager_->commandQueues_[CLManager::EXECUTE], 1, 
                                 &(it->second), 0, NULL, NULL);
    if (error_ != CL_SUCCESS) {
      ERROR("Failed to release GL object");
      ERROR("Failed object: " << it->first);
      return false;
    }
  }
  return true;
}


char * CLProgram::ReadSource(std::string _filename, int &_numChars) const {
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
