/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <GL/glew.h>
#include <Texture3D.h>
#include <Utils.h>
#include <PixelBuffer.h>

using namespace osp;

Texture3D::Texture3D(std::vector<unsigned int> _dim) : Texture(_dim) {}

Texture3D * Texture3D::New(std::vector<unsigned int> _dim) {
  if (_dim.size() != 3) {
    ERROR("Texture3D needs a dimension vector of size 3, defaulting to 1x1x1");
    _dim = std::vector<unsigned int>(3, 1);
  }
  return new Texture3D(_dim);
}

bool Texture3D::Init(float *_data) {
  INFO("Initializing Texture3D");
  glGetError();

  if (initialized_) {
    WARNING("Texture3D already initialized, doing nothing");
    return true;
  }

  glEnable(GL_TEXTURE_3D);
  glGenTextures(1, &handle_);
  glBindTexture(GL_TEXTURE_3D, handle_);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, 
               dim_[0], dim_[1], dim_[2],
               0, GL_RED, GL_FLOAT, static_cast<GLvoid*>(_data));
  glBindTexture(GL_TEXTURE_3D, 0);
  
  initialized_ = true;

  CheckGLError("Texture3D::Init()");
  return true;
}

bool Texture3D::Update(PixelBuffer *_pixelBuffer) {
  
  // Reset any errors
  glGetError();

  if (!initialized_) {
    ERROR("Texture not initialized");
    return false;
  }
  
  glBindTexture(GL_TEXTURE_3D, handle_);
  // TODO init check?
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _pixelBuffer->Handle());

  // When a non-zero pixel unpack buffer is bound, the last argument 
  // (data) is treated as an offset into the data.
  glTexSubImage3D(GL_TEXTURE_3D,    // target
                  0,                // level
                  0,                // xoffset
                  0,                // yoffset
                  0,                // zoffset
                  dim_[0],          // width
                  dim_[1],          // height
                  dim_[2],          // depth
                  GL_RED,           // format
                  GL_FLOAT,         // type
                  0);               // data
  
  
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  glBindTexture(GL_TEXTURE_3D, 0);

  return (CheckGLError("Texture3D::Update(PixelBuffer*)") == GL_NO_ERROR);
}
