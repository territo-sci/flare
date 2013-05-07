#ifndef TEXTURE3D_H_
#define TEXTURE3D_H_

/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 * 
 */

#include <Texture.h>
#include <vector>

namespace osp {

class ShaderProgram;

class Texture3D : public Texture {
public:
  static Texture3D * New(std::vector<unsigned int> _dim);
  virtual bool Init(float *_data);
  // TODO unused, rethink structure
  // remove from base class and make 2D exclusive
  virtual bool Bind(ShaderProgram *_shaderProgram,
                    std::string _uniformName,
                    unsigned int _texUnit) const {};
private:
  Texture3D(std::vector<unsigned int> _dim);
};

}

#endif

