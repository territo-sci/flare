/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */
#include <GL/glew.h>
#include <PixelBuffer.h>
#include <Utils.h>
#include <algorithm>

 using namespace osp;

 PixelBuffer * PixelBuffer::New(unsigned int _numFloats) {
   return new PixelBuffer(_numFloats);
 }

PixelBuffer::PixelBuffer(unsigned int _numFloats) 
  : numFloats_(_numFloats), 
    mappedPointer_(NULL),
    initialized_(false) {}

bool PixelBuffer::Init(float *_data) {
  
  // Reset any errors
  glGetError();

  if (initialized_) {
    WARNING("PixelBuffer already initialized");
    return true;
  }

  glGenBuffers(1, &handle_);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, handle_);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, numFloats_*sizeof(float),
               reinterpret_cast<GLvoid*>(_data), GL_STREAM_DRAW);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  if (!CheckGLError("PixelBuffer::Init()") == GL_NO_ERROR) {
    return false;
  }

  initialized_ = true;
  return true;
  
}

bool PixelBuffer::MapPointer() {

  // Reset errors
  glGetError();
  
  if (!initialized_) {
    ERROR("Pixelbuffer not initialized");
    return false;
  }

  if (mappedPointer_ != NULL) {
    ERROR("Pointer already mapped");
    return false;
  }

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, handle_);

  // Map the PBO into CPU controller memory
  mappedPointer_ = reinterpret_cast<float*>(
    glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  if (mappedPointer_ == NULL) {
    ERROR("Mapped pointer is NULL");
    return false;
  }

  return (CheckGLError("PixelBuffer::MapPointer()") == GL_NO_ERROR);
}

bool PixelBuffer::UnmapPointer() {
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, handle_);
  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  mappedPointer_ = NULL;
  return (CheckGLError("PixelBuffer::UnmapPointer()") == GL_NO_ERROR);
}

bool PixelBuffer::Update(float *_data) {

   if (!initialized_) {
     ERROR("Pixelbuffer not initialized");
     return false;
   }

   if (mappedPointer_ == NULL) {
     ERROR("Pointer not mapped");
     return false;
   }

   std::copy(_data, _data+numFloats_, mappedPointer_);
   
   return true;
}

/*

bool PixelBuffer::Update(float *_data) {

  // Reset errors
  glGetError();

  if (!initialized_) {
    ERROR("PixelBuffer not initialized");
    return false;
  }

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, handle_);
  
  // To avoid glMapBuffer to stall if the GPU is working with this buffer,
  // call glBufferData with NULL argument to discard previous data in the PBO
  // and glMapBuffer will return immediately. 
  //glBufferData(GL_PIXEL_UNPACK_BUFFER, numFloats_*sizeof(float),
    //           NULL, GL_STREAM_DRAW);

  // Map the PBO into CPU controller memory
  float * floatPtr = reinterpret_cast<float*>(
    glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));

  if (floatPtr == NULL) {
    ERROR("Mapped host pointer is NULL");
    return false;
  }
   
  //DEBUG("Copying voxel data");
  std::copy(_data, _data+numFloats_, floatPtr); 
  //DEBUG("Completed copying voxel data");

  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  return (CheckGLError("PixelBuffer::Update()") == GL_NO_ERROR);
}

*/
