/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 * 
 */

#include <GL/glew.h>
#include <GL/glfw.h>
#ifndef _WIN32
#include <GL/glx.h>
#endif
#include <fstream>
#include <Raycaster.h>
#include <Texture2D.h>
#include <Texture3D.h>
#include <TextureAtlas.h>
#include <BrickManager.h>
#include <Utils.h>
#include <ShaderProgram.h>
#include <glm/gtc/matrix_transform.hpp>
#include <TransferFunction.h>
#include <Animator.h>
#include <vector>
#include <CLManager.h>
#include <KernelConstants.h>
#include <Config.h>

using namespace osp;

uint32_t ZOrder(uint16_t xPos, uint16_t yPos, uint16_t zPos) {
  uint32_t x = static_cast<uint32_t>(xPos);
  uint32_t y = static_cast<uint32_t>(yPos);
  uint32_t z = static_cast<uint32_t>(zPos);
  x = (x | (x << 16)) & 0x030000FF;
  x = (x | (x <<  8)) & 0x0300F00F;
  x = (x | (x <<  4)) & 0x030C30C3;
  x = (x | (x <<  2)) & 0x09249249;
  y = (y | (y << 16)) & 0x030000FF;
  y = (y | (y <<  8)) & 0x0300F00F;
  y = (y | (y <<  4)) & 0x030C30C3;
  y = (y | (y <<  2)) & 0x09249249;
  z = (z | (z << 16)) & 0x030000FF;
  z = (z | (z <<  8)) & 0x0300F00F;
  z = (z | (z <<  4)) & 0x030C30C3;
  z = (z | (z <<   2)) & 0x09249249;
  const uint32_t result = x | (y << 1) | (z << 2);
  return result;
}

const double BYTES_PER_GB = 1073741824.0;

Raycaster::Raycaster(Config *_config) 
  : Renderer(),
    config_(_config),
    cubeFrontFBO_(0),
    cubeBackFBO_(0),
    renderbufferObject_(0),
    cubePosbufferObject_(0),
    quadPosbufferObject_(0),
    cubePositionAttrib_(0),
    quadPositionAttrib_(0),
    cubeShaderProgram_(NULL),
    quadShaderProgram_(NULL),
    cubeFrontTex_(NULL),
    cubeBackTex_(NULL),
    quadTex_(NULL),
    pitch_(-30.f),
    yaw_(0.f),
    roll_(30.f),
    zoom_(1.f),
    model_(glm::mat4()),
    view_(glm::mat4()),
    proj_(glm::mat4()),
    cubeInitialized_(false),
    quadInitialized_(false),
    matricesInitialized_(false),
    framebuffersInitialized_(false),
    pingPongIndex_(true),
    animator_(NULL),
    pingPong_(0),
    lastTimestep_(1),
    brickManager_(NULL),
    clManager_(NULL) {
}

Raycaster::~Raycaster() {
}

Raycaster * Raycaster::New(Config *_config) {
  Raycaster *raycaster = new Raycaster(_config);
  raycaster->UpdateConfig();
  return raycaster;
}

// TODO Move out hardcoded values
bool Raycaster::InitMatrices() {
  float aspect = (float)winWidth_/(float)winHeight_;
  proj_ = glm::perspective(40.f, aspect, 0.1f, 100.f);
  matricesInitialized_ = true;
  return true;
}

void Raycaster::SetAnimator(Animator *_animator) {
  animator_ = _animator;
}

void Raycaster::SetBrickManager(BrickManager *_brickManager) {
  brickManager_ = _brickManager;
}

bool Raycaster::InitCube() {
  glGetError();

  float v[] = {
   // front
   1.f, 0.f, 0.f, 1.f,
   0.f, 1.f, 0.f, 1.f,
   0.f, 0.f, 0.f, 1.f,
   1.f, 0.f, 0.f, 1.f,
   1.f, 1.f, 0.f, 1.f,
   0.f, 1.f, 0.f, 1.f,
   // right
   1.f, 0.f, 0.f, 1.f,
   1.f, 0.f, 1.f, 1.f,
   1.f, 1.f, 0.f, 1.f,
   1.f, 0.f, 1.f, 1.f,
   1.f, 1.f, 1.f, 1.f,
   1.f, 1.f, 0.f, 1.f,
   // back
   1.f, 1.f, 1.f, 1.f,
   0.f, 0.f, 1.f, 1.f,
   0.f, 1.f, 1.f, 1.f,
   1.f, 1.f, 1.f, 1.f,
   1.f, 0.f, 1.f, 1.f,
   0.f, 0.f, 1.f, 1.f,
   // left
   0.f, 0.f, 1.f, 1.f,
   0.f, 0.f, 0.f, 1.f,
   0.f, 1.f, 1.f, 1.f,
   0.f, 0.f, 0.f, 1.f,
   0.f, 1.f, 0.f, 1.f,
   0.f, 1.f, 1.f, 1.f,
   // top
   0.f, 1.f, 0.f, 1.f,
   1.f, 1.f, 0.f, 1.f,
   0.f, 1.f, 1.f, 1.f,
   0.f, 1.f, 1.f, 1.f,
   1.f, 1.f, 0.f, 1.f,
   1.f, 1.f, 1.f, 1.f,
   // bottom
   0.f, 0.f, 0.f, 1.f,
   0.f, 0.f, 1.f, 1.f,
   1.f, 0.f, 1.f, 1.f,
   0.f, 0.f, 0.f, 1.f,
   1.f, 0.f, 1.f, 1.f,
   1.f, 0.f, 0.f, 1.f
 };

  glGenBuffers(1, &cubePosbufferObject_);
  glBindBuffer(GL_ARRAY_BUFFER, cubePosbufferObject_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*144, v, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  CheckGLError("InitCube()");
  cubeInitialized_ = true;
  return true;
}

bool Raycaster::InitQuad() {
  glGetError();

  float v[] = {
    -1.f, -1.f, 0.f, 1.f,
     1.f, -1.f, 0.f, 1.f,
    -1.f,  1.f, 0.f, 1.f,
     1.f, -1.f, 0.f, 1.f,
     1.f,  1.f, 0.f, 1.f,
    -1.f,  1.0, 0.f, 1.f,
  };

  glGenBuffers(1, &quadPosbufferObject_);
  glBindBuffer(GL_ARRAY_BUFFER, quadPosbufferObject_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*24, v, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  CheckGLError("InitQuad()");
  quadInitialized_ = true;
  return true;
}

bool Raycaster::InitFramebuffers() {

  glGetError();

  if (winWidth_ == 0 || winHeight_ == 0) {
    ERROR("Raycaster window dimension(s) are zero");
    return false;
  }

  if (cubeFrontTex_ == NULL || cubeBackTex_ == NULL) {
    ERROR("Can't init framebuffers, textures are not set");
    return false;
  }

  // Renderbuffer for depth component
  INFO("Initializing renderbuffer for depth");
  glGenRenderbuffers(1, &renderbufferObject_);
  glBindRenderbuffer(GL_RENDERBUFFER, renderbufferObject_);
  glRenderbufferStorage(GL_RENDERBUFFER,
                        GL_DEPTH_COMPONENT,
                        winWidth_,
                        winHeight_);
  glBindRenderbuffer(GL_RENDERBUFFER, 0);
  CheckGLError("Init renderbuffer");

  // Front cube
  INFO("Initializing front cube framebuffer");
  glGenFramebuffers(1, &cubeFrontFBO_);
  glBindFramebuffer(GL_FRAMEBUFFER, cubeFrontFBO_);
  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D,
                         cubeFrontTex_->Handle(),
                         0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                            GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER,
                            renderbufferObject_);
  GLenum status;
  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    ERROR("Front cube framebuffer not complete");
    CheckGLError("Front cube framebuffer");
    return false;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // Back cube
  INFO("Initializing back cube framebuffer");
  glGenFramebuffers(1, &cubeBackFBO_);
  glBindFramebuffer(GL_FRAMEBUFFER, cubeBackFBO_);
  glFramebufferTexture2D(GL_FRAMEBUFFER,
                         GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D,
                         cubeBackTex_->Handle(),
                         0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                            GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER,
                            renderbufferObject_);
  status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    ERROR("Back cube framebuffer not complete");
    CheckGLError("Back cube framebuffer");
    return false;
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  CheckGLError("InitFramebuffers()");

  INFO("Initializing framebuffers... complete");

  framebuffersInitialized_ = true;
  return true;
}


bool Raycaster::ReloadTransferFunctions() {
  INFO("Reloading transfer functions");
  if (!transferFunctions_[0]->ReadFile()) return false;
  if (!transferFunctions_[0]->ConstructTexture()) return false;

  float* tfData = transferFunctions_[0]->FloatData();
  unsigned int tfSize = sizeof(float)*transferFunctions_[0]->Width()*4;
  if (!clManager_->ReleaseBuffer("RaycasterTSP", transferFunctionArg_)) {
    return false;
  }

  if (!clManager_->AddBuffer("RaycasterTSP", transferFunctionArg_,
                             reinterpret_cast<void*>(tfData),
                             tfSize,
                             CLManager::COPY_HOST_PTR,
                             CLManager::READ_ONLY)) return false;
  return true;
}

bool Raycaster::UpdateConfig() {
  kernelConstants_.stepsize_ = config_->RaycasterStepsize();
  kernelConstants_.intensity_ = config_->RaycasterIntensity();
  kernelConstants_.temporalTolerance_ = config_->TemporalErrorTolerance(); 
  kernelConstants_.spatialTolerance_ = config_->SpatialErrorTolerance();
  traversalConstants_.stepsize_ = config_->TSPTraversalStepsize();
  traversalConstants_.temporalTolerance_ = config_->TemporalErrorTolerance();
  traversalConstants_.spatialTolerance_ = config_->SpatialErrorTolerance(); 
  return true;
}

bool Raycaster::UpdateMatrices() {
  model_ = glm::mat4(1.f);
  model_ = glm::translate(model_, glm::vec3(0.5f, 0.5f, 0.5f));
  model_ = glm::rotate(model_, roll_, glm::vec3(1.f, 0.f, 0.f));
  model_ = glm::rotate(model_, -pitch_, glm::vec3(0.f, 1.f, 0.0));
  model_ = glm::rotate(model_, yaw_, glm::vec3(0.f, 0.f, 1.f));
  model_ = glm::translate(model_, glm::vec3(-0.5f, -0.5f, -0.5f));
  view_ = glm::rotate(glm::mat4(1.f), 180.f, glm::vec3(1.f, 0.f, 0.f));
  view_ = glm::translate(view_, glm::vec3(-0.5f, -0.5f, zoom_));
  return true;
}

bool Raycaster::BindTransformationMatrices(ShaderProgram * _program)
{
  if (!_program->BindMatrix4f("modelMatrix", &model_[0][0])) return false;
  if (!_program->BindMatrix4f("viewMatrix", &view_[0][0])) return false;
  if (!_program->BindMatrix4f("projectionMatrix", &proj_[0][0])) return false;
  return true;
}

void Raycaster::SetCubeFrontTexture(Texture2D *_cubeFrontTexture) {
  cubeFrontTex_ = _cubeFrontTexture;
}

void Raycaster::SetCubeBackTexture(Texture2D *_cubeBackTexture) {
  cubeBackTex_ = _cubeBackTexture;
}

void Raycaster::SetQuadTexture(Texture2D *_quadTexture) {
  quadTex_ = _quadTexture;
}

void Raycaster::SetCLManager(CLManager *_clManager) {
  clManager_ = _clManager;
}

void Raycaster::SetTSP(TSP *_tsp) {
  tsp_ = _tsp;
}

void Raycaster::SetCubeShaderProgram(ShaderProgram *_cubeShaderProgram) {
  cubeShaderProgram_ = _cubeShaderProgram;
}

void Raycaster::SetQuadShaderProgram(ShaderProgram *_quadShaderProgram) {
  quadShaderProgram_ = _quadShaderProgram;
}

bool Raycaster::Render(float _timestep) {

  if (animator_ != NULL) {
    animator_->Update(_timestep);
  } else {
    WARNING("Animator not set");
  }
  
  // Reset any errors
  glGetError();

  // TODO move init checks and only run them once
  if (!matricesInitialized_) {
    ERROR("Rendering failed, matrices not initialized");
    return false;
  }

  if (!cubeInitialized_) {
    ERROR("Rendering failed, cube not initialized");
    return false;
  }

  if (!quadInitialized_) {
    ERROR("Rendering failed, quad not initialized");
    return false;
  }

  if (!framebuffersInitialized_) {
    ERROR("Rendering failed, framebuffers not initialized");
    return false;
  }

  if (!cubeFrontTex_ || !cubeBackTex_ || !quadTex_) {
    ERROR("Rendering failed, one or more texures are not set");
    return false;
  }

  if (!cubeShaderProgram_ || !quadShaderProgram_) {
    ERROR("Rendering failed, one or more shaders are not set");
    return false;
  }

  if (!HandleKeyboard()) return false;
  if (!HandleMouse()) return false;
  if (!UpdateMatrices()) return false;
  if (!BindTransformationMatrices(cubeShaderProgram_)) return false;

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Render cube
  glUseProgram(cubeShaderProgram_->Handle());
  cubePositionAttrib_ = cubeShaderProgram_->GetAttribLocation("position");
  if (cubePositionAttrib_ == -1) {
    ERROR("Cube position attribute lookup failed");
    return false;
  }
  glFrontFace(GL_CW);
  glEnable(GL_CULL_FACE);

  // Front
  glBindFramebuffer(GL_FRAMEBUFFER, cubeFrontFBO_);
  glCullFace(GL_BACK);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindBuffer(GL_ARRAY_BUFFER, cubePosbufferObject_);
  glEnableVertexAttribArray(cubePositionAttrib_);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLES, 0, 144);
  glDisableVertexAttribArray(cubePositionAttrib_);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  CheckGLError("Cube front rendering");

  // Back
  glBindFramebuffer(GL_FRAMEBUFFER, cubeBackFBO_);
  glCullFace(GL_FRONT);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glBindBuffer(GL_ARRAY_BUFFER, cubePosbufferObject_);
  glEnableVertexAttribArray(cubePositionAttrib_);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLES, 0, 144);
  glDisableVertexAttribArray(cubePositionAttrib_);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  CheckGLError("Cube back rendering");

  glUseProgram(0);

  // TODO Set kernel constants that might have changed 

  unsigned int currentTimestep;
  unsigned int nextTimestep;
  if (animator_ != NULL) {
    currentTimestep = animator_->CurrentTimestep();
    nextTimestep = animator_->NextTimestep();
  } else {
    WARNING("Animator not set");
    currentTimestep = 0;
    nextTimestep = 1;
  }


  // TODO temp
  traversalConstants_.numTimesteps_ = (int)tsp_->NumTimesteps();
  traversalConstants_.numValuesPerNode_ = (int)tsp_->NumValuesPerNode();
  traversalConstants_.numOTNodes_ = (int)tsp_->NumOTNodes();
  traversalConstants_.timestep_ = currentTimestep;
  
  kernelConstants_.numTimesteps_ = (int)tsp_->NumTimesteps();
  kernelConstants_.numValuesPerNode_ = (int)tsp_->NumValuesPerNode();
  kernelConstants_.numOTNodes_ = (int)tsp_->NumOTNodes();
  kernelConstants_.numBoxesPerAxis_ = (int)tsp_->NumBricksPerAxis();
  kernelConstants_.timestep_ = currentTimestep;
  kernelConstants_.rootLevel_ = (int)tsp_->NumOTLevels() - 1;
  kernelConstants_.paddedBrickDim_ = (int)tsp_->PaddedBrickDim();

  // Add updated traversal constants
  if (!clManager_->AddBuffer("TSPTraversal", tspConstantsArg_,
                             reinterpret_cast<void*>(&traversalConstants_),
                             sizeof(TraversalConstants),
                             CLManager::COPY_HOST_PTR,
                             CLManager::READ_ONLY)) return false;
  // TODO test brick request list
  std::vector<int> brickRequest(tsp_->NumTotalNodes(), 0);
  if (!clManager_->AddBuffer("TSPTraversal", tspBrickListArg_,
                             reinterpret_cast<void*>(&brickRequest[0]), 
                             brickRequest.size()*sizeof(int),
                             CLManager::COPY_HOST_PTR,
                             CLManager::READ_WRITE)) return false;

  
  if (!clManager_->PrepareProgram("TSPTraversal")) return false;
  if (!clManager_->LaunchProgram("TSPTraversal", 
                                 512, 512, 16, 16)) return false;
  if (!clManager_->FinishProgram("TSPTraversal")) return false;

  if (!clManager_->ReadBuffer("TSPTraversal", tspBrickListArg_,
                              reinterpret_cast<void*>(&brickRequest[0]),
                              brickRequest.size()*sizeof(int),
                              true)) return false;

  //INFO(" ");
  //for (unsigned int i=0; i< brickRequest.size(); ++i) {
  //    if (brickRequest[i] > 0) {
  //        INFO("Req brick " << i);
  //    }
  //}

  if (!clManager_->ReleaseBuffer("TSPTraversal", tspBrickListArg_)) return false;
  if (!clManager_->ReleaseBuffer("TSPTraversal", tspConstantsArg_)) return false;
  
  // Build a brick list from the request list
  if (!brickManager_->BuildBrickList(brickRequest)) return false;
  // Apply the brick list, update the texture atlas
  if (!brickManager_->UpdateAtlas()) return false;

  // When the texture atlas contains the correct bricks, run second pass
 
  // Add kernel constants
  if (!clManager_->AddBuffer("RaycasterTSP", constantsArg_,
                             reinterpret_cast<void*>(&kernelConstants_),
                             sizeof(KernelConstants),
                             CLManager::COPY_HOST_PTR,
                             CLManager::READ_ONLY)) return false;

  // Add brick list
  if (!clManager_->
    AddBuffer("RaycasterTSP", brickListArg_,
              reinterpret_cast<void*>(&(brickManager_->BrickList()[0])),
              brickManager_->BrickList().size()*sizeof(int),
              CLManager::COPY_HOST_PTR,
              CLManager::READ_ONLY)) return false;
              

  if (!clManager_->PrepareProgram("RaycasterTSP")) return false;
  if (!clManager_->LaunchProgram("RaycasterTSP",
                                 512, 512, 16, 16)) return false;
  if (!clManager_->FinishProgram("RaycasterTSP")) return false;
  
  if (!clManager_->ReleaseBuffer("RaycasterTSP", constantsArg_)) return false;
  if (!clManager_->ReleaseBuffer("RaycasterTSP", brickListArg_)) return false;

  /*

  // Shut down after one loop - for profiling purposes
  if (nextTimestep == 0) return false;

  bool timeToUpdate = (currentTimestep != lastTimestep_);
  if (timeToUpdate) {
    lastTimestep_ = currentTimestep;
  }

  pingPongIndex_ = 1-pingPongIndex_;
  // Switch between pinned memory indices
  CLHandler::MemIndex index = 
    static_cast<CLHandler::MemIndex>(pingPongIndex_);
  CLHandler::MemIndex nextIndex = 
    static_cast<CLHandler::MemIndex>(1-pingPongIndex_);

  //unsigned int timestepOffset = voxelData_->TimestepOffset(currentTimestep);
  //float *frameData = voxelData_->DataPtr(timestepOffset);
  //unsigned int frameSize = voxelData_->NumVoxelsPerTimestep()*sizeof(float);

  reader_->ReadTimestep(currentTimestep);
  float *frameData = voxelDataFrame_->Data();
  unsigned int frameSize = 
    voxelDataHeader_->NumVoxelsPerTimestep()*sizeof(float);

  // Prepare and run kernel. The launch returns immediately.
  if (!clHandler_->PrepareRaycaster()) return false;
  if (!clHandler_->LaunchRaycaster()) return false;

  // Copy next frame data to host
  if (!clHandler_->UpdateHostMemory(nextIndex, voxelData_, nextTimestep)) {
    return false;
  }

  // Initiate transfer of next frame from pinned memory to GMEM
  if (!clHandler_->WriteToDevice(nextIndex)) return false;
  
  // Wait for kernel to finish if needed and release resources 
  if (!clHandler_->FinishRaycaster()) return false;
  */

  // Render to screen using quad
  if (!quadTex_->Bind(quadShaderProgram_, "quadTex", 0)) return false;

  glDisable(GL_CULL_FACE);

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram(quadShaderProgram_->Handle());
  quadPositionAttrib_ = quadShaderProgram_->GetAttribLocation("position");
  if (quadPositionAttrib_ == -1) {
    ERROR("Quad position attribute lookup failed");
    return false;
  }
  glCullFace(GL_BACK);
  glClear(GL_COLOR_BUFFER_BIT);
  glBindBuffer(GL_ARRAY_BUFFER, quadPosbufferObject_);
  glEnableVertexAttribArray(quadPositionAttrib_);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(quadPositionAttrib_);

  CheckGLError("Quad rendering");

  glUseProgram(0);
  
  /*
  
  // Wait for transfer to pinned mem to complete
  clHandler_->FinishQueue(CLHandler::TRANSFER);

  // Signal that the next frame is ready
  clHandler_->SetActiveIndex(nextIndex);

  */

  // Window manager takes care of swapping buffers
  return true;
}

bool Raycaster::ReloadShaders() {
  glGetError();
  INFO("Reloading shaders");
  if (!cubeShaderProgram_->DeleteShaders()) return false;
  if (!quadShaderProgram_->DeleteShaders()) return false;
  if (!cubeShaderProgram_->Reload()) return false;
  if (!quadShaderProgram_->Reload()) return false;
  CheckGLError("ReloadShaders()");
  return true;
}

bool Raycaster::HandleMouse() {
  if (leftMouseDown_) {
    pitch_ += 0.2f*(float)(currentMouseX_ - lastMouseX_);
    roll_ += 0.2f*(float)(currentMouseY_ - lastMouseY_);  
  }
  return true;
}

// Don't forget to add keys to look for in window manager
// TODO proper keyboard handling class
bool Raycaster::HandleKeyboard() {
  if (KeyPressedNoRepeat('R')) {
    if (!config_->Read()) return false; 
    INFO("Config file read");
    if (!UpdateConfig()) return false;
    INFO("Config updated");
    if (!ReloadShaders()) return false;
    INFO("Shaders reloaded");
    if (!ReloadTransferFunctions()) return false;
    INFO("Transfer functions reloaded");
    if (!animator_->UpdateConfig()) return false;
    INFO("Animator updated");
  }

  if (KeyPressed('W')) zoom_ -= 0.1f;
  if (KeyPressed('S')) zoom_ += 0.1f;
  if (KeyPressedNoRepeat(32)) animator_->TogglePause();
  if (KeyPressedNoRepeat('F')) animator_->ToggleFPSMode();
  if (KeyPressed('Z')) animator_->IncTimestep();
  if (KeyPressed('X')) animator_->DecTimestep();
  return true;
}

bool Raycaster::KeyPressedNoRepeat(int _key) {
  if (KeyPressed(_key) == true) {
    if (KeyLastState(_key) == false) {
      SetKeyLastState(_key, true);
      return true;
    }
  } else {
    SetKeyLastState(_key, false);
  }
  return false;
}

void Raycaster::SetKeyLastState(int _key, bool _pressed) {
  std::map<int, bool>::iterator it;
  it = keysLastState_.find(_key);
  if (it == keysLastState_.end()) {
    keysLastState_.insert(std::make_pair(_key, _pressed));
  } else {
    it->second = _pressed;
  }
}

bool Raycaster::KeyLastState(int _key) const {
  std::map<int, bool>::const_iterator it;
  it = keysLastState_.find(_key);
  if (it == keysLastState_.end()) {
    return false;
  } else {
    return it->second;
  }
}

bool Raycaster::InitCL() {

  INFO("Initializing OpenCL");

  if (!clManager_) {
    ERROR("InitCL() - No CL manager has been set");
    return false;
  }
  
  if (!clManager_->InitPlatform()) return false;
  if (!clManager_->InitDevices()) return false;
  if (!clManager_->CreateContext()) return false;
  if (!clManager_->CreateCommandQueue()) return false;

  // TSP traversal part or raycaster
  if (!clManager_->CreateProgram("TSPTraversal",
                                 "kernels/TSPTraversal.cl")) return false;
  if (!clManager_->BuildProgram("TSPTraversal")) return false;
  if (!clManager_->CreateKernel("TSPTraversal")) return false;

  cl_mem cubeFrontCLmem;
  if (!clManager_->AddTexture("TSPTraversal", tspCubeFrontArg_, 
                              cubeFrontTex_, CLManager::TEXTURE_2D,
                              CLManager::READ_ONLY, cubeFrontCLmem)) return false;

  cl_mem cubeBackCLmem;
  if (!clManager_->AddTexture("TSPTraversal", tspCubeBackArg_,
                              cubeBackTex_, CLManager::TEXTURE_2D,
                              CLManager::READ_ONLY, cubeBackCLmem)) return false;

  if (!clManager_->AddBuffer("TSPTraversal", tspTSPArg_,
                             reinterpret_cast<void*>(tsp_->Data()),
                             tsp_->Size()*sizeof(int),
                             CLManager::COPY_HOST_PTR,
                             CLManager::READ_ONLY)) return false;


  // TEST
  if (!clManager_->CreateProgram("RaycasterTSP",
                                "kernels/RaycasterTSP.cl")) return false;
  if (!clManager_->BuildProgram("RaycasterTSP")) return false;
  if (!clManager_->CreateKernel("RaycasterTSP")) return false;

  if (!clManager_->AddTexture("RaycasterTSP", cubeFrontArg_, cubeFrontCLmem,  
                              CLManager::READ_ONLY)) return false;

  if (!clManager_->AddTexture("RaycasterTSP", cubeBackArg_, cubeBackCLmem, 
                              CLManager::READ_ONLY)) return false;

  if (!clManager_->AddTexture("RaycasterTSP", quadArg_, quadTex_, 
                              CLManager::TEXTURE_2D, 
                              CLManager::WRITE_ONLY)) return false;

  if (!clManager_->AddTexture("RaycasterTSP", textureAtlasArg_, 
                              brickManager_->TextureAtlas(),
                              CLManager::TEXTURE_3D, 
                              CLManager::READ_ONLY)) return false;

  // Add transfer function
  float* tfData = transferFunctions_[0]->FloatData();
  unsigned int tfSize = sizeof(float)*transferFunctions_[0]->Width()*4;
  if (!clManager_->AddBuffer("RaycasterTSP", transferFunctionArg_,
                             reinterpret_cast<void*>(tfData),
                             tfSize,
                             CLManager::COPY_HOST_PTR,
                             CLManager::READ_ONLY)) return false;

  if (!clManager_->AddBuffer("RaycasterTSP", tspArg_,
                             reinterpret_cast<void*>(tsp_->Data()),
                             tsp_->Size()*sizeof(int),
                             CLManager::COPY_HOST_PTR,
                             CLManager::READ_ONLY)) return false;

  return true;
}

void Raycaster::AddTransferFunction(TransferFunction *_transferFunction) {
  transferFunctions_.push_back(_transferFunction);
}
