#ifndef TEXTURE3D_H_
#define TEXTURE3D_H_

/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 * 
 */

#include <Texture.h>
#include <vector>

namespace osp {

class PixelBuffer;
class ShaderProgram;

class Texture3D : public Texture {
public:
  static Texture3D * New(std::vector<unsigned int> _dim);
  virtual bool Init(float *_data);
  // Populate the texture with new data
  bool Update(float *_data);
  // Get data from PixelBuffer
  bool Update(PixelBuffer *_pixelBuffer);

private:
  Texture3D(std::vector<unsigned int> _dim);
};

}

#endif

