#ifndef RAYCASTER_H
#define RAYCASTER_H
#include <Renderer.h>
#include <map>
#include <string>
#include <glm/glm.hpp>

namespace osp {

class ShaderProgram;
class Texture2D;

class Raycaster : public Renderer {
public:
  static Raycaster * New();
  virtual ~Raycaster();
  virtual bool Render(float _timestep);
  bool ReloadShaders();
  bool ReloadConfig();
  bool InitMatrices();
  bool InitCube();
  bool InitQuad();
  bool InitFramebuffers();
  bool UpdateMatrices();
  bool BindTransformationMatrices(ShaderProgram *_program);
  bool ReadShaderConfig(const std::string &_filename);
  bool HandleMouse();
  bool HandleKeyboard();
  void SetCubeFrontTexture(Texture2D *_cubeFrontTexture);
  void SetCubeBackTexture(Texture2D *_cubeBackTexture);
  void SetQuadTexture(Texture2D *_quadTexture);
  void SetCubeShaderProgram(ShaderProgram *_cubeShaderProgram);
  void SetQuadShaderProgram(ShaderProgram *_quadShaderProgram);
  void SetConfigFilename(std::string _configFilename);
  void SetKeyLastState(std::string _key, bool _pressed);
  bool KeyLastState(std::string _key) const;
private:
  Raycaster();
  std::string configFilename_;
  // Buffer object handles
  unsigned int cubeFrontFBO_;
  unsigned int cubeBackFBO_;
  unsigned int renderbufferObject_;
  unsigned int cubePosbufferObject_;
  unsigned int quadPosbufferObject_;
  unsigned int cubePositionAttrib_;
  unsigned int quadPositionAttrib_;
  // Shaders
  ShaderProgram *cubeShaderProgram_;
  ShaderProgram *quadShaderProgram_;
  // Textures
  Texture2D *cubeFrontTex_;
  Texture2D *cubeBackTex_;
  Texture2D *quadTex_;
  // View params
  float pitch_;
  float yaw_;
  float roll_;
  float zoom_;
  // Tranformation matrices
  glm::mat4 model_;
  glm::mat4 view_;
  glm::mat4 proj_;
  // Shader constants
  std::map<std::string, float> shaderConstants_;
  // State
  bool cubeInitialized_;
  bool quadInitialized_;
  bool matricesInitialized_;
  bool framebuffersInitialized_;
  // Extra keyboard state to prevent key repeat
  std::map<std::string, bool> keysLastState_; 
};

}

#endif
