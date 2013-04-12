#ifndef TEXTURE_H
#define TEXTURE_H

#include <vector>
#include <string>

namespace osp {

class ShaderProgram;

class Texture {
public:
  // Get x, y or z dimensions (0, 1, 2 etc) if
  // they exist
  unsigned int Dim(unsigned int _axis) const;
  // Return handle for OpenGL use
  unsigned int Handle() const;
  // Initialize, subclasses have to implement
  virtual bool Init() = 0;
  // Bind to a ShaderProgram
  virtual bool Bind(ShaderProgram * _shaderProgram,
                    std::string _uniformName,
                    unsigned int _texUnit) const = 0;
  virtual ~Texture();
protected:
  Texture(std::vector<unsigned int> _dim);
  std::vector<unsigned int> dim_;
  unsigned int handle_;
  bool initialized_;
};

}

#endif
