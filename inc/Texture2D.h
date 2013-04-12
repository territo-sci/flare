#ifndef TEXTURE2D_H_
#define TEXTURE2D_H_

#include <Texture.h>
#include <vector>

namespace osp {

class ShaderProgram;

class Texture2D : public Texture {
public:
  static Texture2D * New(std::vector<unsigned int> _dim);
  virtual bool Init();
  virtual bool Bind(ShaderProgram * _shaderProgram,
                    std::string _uniformName,
                    unsigned int _texUnit) const;
private:
  Texture2D(std::vector<unsigned int> _dim);
};

}

#endif
