#ifndef RAYCASTER_H
#define RAYCASTER_H

/*
Author: Victor Sand (victor.sand@gmail.com)
Mammoth workhorse class that ties all parts of the raycaster application
together. Manages state, render calls and a lot of initializations.
TODO: Iteratively break away parts from it into other classes.
*/

#include <Renderer.h>
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <CL/cl.hpp>
#include <KernelConstants.h>
#include <boost/timer/timer.hpp>
#include <TSP.h>

namespace osp {

class ShaderProgram;
class Texture2D;
class Texture3D;
class TransferFunction;
class Animator;
class BrickManager;
class CLManager;

class Raycaster : public Renderer {
public:
  
  static Raycaster * New();
  virtual ~Raycaster();
  virtual bool Render(float _timestep);

  // Reload GLSL shaders
  bool ReloadShaders();
  // Reload constant configuration file
  bool ReloadConfig();
  // Reload transfer function file
  bool ReloadTransferFunctions();
  // Initialize model, view and projection matrices.
  // TODO Move hardcoded init values to config file
  bool InitMatrices();
  // Initialize the color cube used for raycasting.
  // TODO Make cube class.
  bool InitCube();
  // Init the quad used for output texture rendering
  bool InitQuad();
  // Run all of CLHandlers initialization functions
  bool InitCL();
  // Set up buffers for the different rendering stages
  bool InitFramebuffers();
  // Set up two pixel buffers for texture streaming
  bool InitPixelBuffers();
  // Update matrices with current view parameters
  bool UpdateMatrices();
  // Bind transformation matrices to a ShaderProgram
  bool BindTransformationMatrices(ShaderProgram *_program);
  // Read shader config from file
  bool ReadShaderConfig(const std::string &_filename);
  bool HandleMouse();
  bool HandleKeyboard();
  // Read kernel config from file and voxel data,
  // update the constants that get sent to the kernel every frame
  bool UpdateKernelConfig();
  // For keyboard handling, retrieve the last known state of a key
  bool KeyLastState(int _key) const;
  // Add a transfer function to the transfer function list
  // TODO Actually support and make use of multiple TFs
  void AddTransferFunction(TransferFunction *_transferFunction);

  Texture2D * CubeFrontTexture() const { return cubeFrontTex_; }
  Texture2D * CubeBackTexture() const { return cubeBackTex_; }
  Texture2D * QuadTexture() const { return quadTex_; }

  void SetKernelConfigFilename(const std::string &_filename);
  void SetCubeFrontTexture(Texture2D *_cubeFrontTexture);
  void SetCubeBackTexture(Texture2D *_cubeBackTexture);
  void SetQuadTexture(Texture2D *_quadTexture);
  void SetCubeShaderProgram(ShaderProgram *_cubeShaderProgram);
  void SetQuadShaderProgram(ShaderProgram *_quadShaderProgram);
  void SetConfigFilename(std::string _configFilename);
  void SetKeyLastState(int, bool _pressed);
  void SetAnimator(Animator *_animator);
  void SetCLManager(CLManager *_clManager);
  void SetBrickManager(BrickManager *_brickManager);
  void SetTSP(TSP *_tsp);

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
  Texture3D *volumeTex_;
  // View params
  float pitch_;
  float yaw_;
  float roll_;
  float startZoom_;
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
  bool pingPong_; // texture streaming
  // Extra keyboard state to prevent key repeat
  std::map<int, bool> keysLastState_; 
  // Helper for pressing button without repeat
  bool KeyPressedNoRepeat(int _key);
  // Animator to control update rate and looping
  Animator * animator_;
  // Used to see if it is time to update frames 
  unsigned int lastTimestep_;
  // Used for ping pong memory buffering
  unsigned int pingPongIndex_;
  // Kernel constants
  KernelConstants kernelConstants_;
  std::string kernelConfigFilename_;
  // Transfer functions
  std::vector<TransferFunction*> transferFunctions_;
 
  // Brick manager with access to brick data
  BrickManager *brickManager_;

  // TSP tree structure (not actual data)
  TSP *tsp_;
  
  // Entry point for all things OpenCL
  CLManager *clManager_;

  // For the corresponding CL kernel
  static const unsigned int cubeFrontArg_ = 0;
  static const unsigned int cubeBackArg_ = 1;
  static const unsigned int quadArg_ = 2;
  static const unsigned int textureAtlasArg_ = 3;
  static const unsigned int constantsArg_ = 4;
  static const unsigned int transferFunctionArg_ = 5; 
  static const unsigned int boxListArg_ = 6;

  static const unsigned int tspCubeFrontArg_ = 0;
  static const unsigned int tspCubeBackArg_ = 1;
  static const unsigned int tspConstantsArg_ = 2;
  static const unsigned int tspTSPArg_ = 3;
  static const unsigned int tspBrickListArg_ = 4;
  
  // Timer and timer constants 
  boost::timer::cpu_timer timer_;
  const double BYTES_PER_GB = 1073741824.0;

};

}

#endif
