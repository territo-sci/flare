#include <GL/glew.h>
#include <GL/glfw.h>
#include <GL/glx.h>
#include <fstream>
#include <Raycaster.h>
#include <Texture2D.h>
#include <Utils.h>
#include <ShaderProgram.h>
#include <glm/gtc/matrix_transform.hpp>
#include <CLHandler.h>
#include <TransferFunction.h>

using namespace osp;

Raycaster::Raycaster() : Renderer(),
                         configFilename_("NotSet"),
												 kernelConfigFilename_("NotSet"),
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
                         clHandler_(CLHandler::New()),
												 timeElapsed_(0.f),
												 animationRate_(0.f),
												 currentTimestep_(0) {

  kernelConstants_.stepSize = 0.01f;
	kernelConstants_.intensity = 60.f;
	kernelConstants_.aDim = 128;
	kernelConstants_.bDim = 128;
	kernelConstants_.cDim = 128;
}

Raycaster::~Raycaster() {
}

Raycaster * Raycaster::New() {
  return new Raycaster();
}

bool Raycaster::InitMatrices() {
  float aspect = (float)winWidth_/(float)winHeight_;
  proj_ = glm::perspective(40.f, aspect, 0.1f, 100.f);
  matricesInitialized_ = true;
  return true;
}

void Raycaster::SetAnimationRate(float _animationRate) {
	animationRate_ = _animationRate;
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

bool Raycaster::ReadShaderConfig(const std::string &_filename) {
  shaderConstants_.clear();
  configFilename_ = _filename;
  std::ifstream in;
  in.open(_filename.c_str());

  if (!in.is_open()) {
    ERROR("Could not open config file " << _filename);
    return false;
  } else {
    std::string uniform;
    float value;
    while (!in.eof()) {
      in >> uniform;
      in >> value;
      shaderConstants_.insert(std::make_pair(uniform, value));
    }

    return true;
  }
}

bool Raycaster::ReloadTransferFunctions() {
	INFO("Reloading transfer functions");
	if (!transferFunctions_[0]->ReadFile()) return false;
	if (!transferFunctions_[0]->ConstructTexture()) return false;
	if (!clHandler_->BindTransferFunction(6, transferFunctions_[0])) 
		return false;
	return true;
}

bool Raycaster::UpdateKernelConfig() {
	std::ifstream in;
	in.open(kernelConfigFilename_.c_str());
	if (!in.is_open()) {
		ERROR("Could not open kernel config file " << kernelConfigFilename_);
		return false;
	} else {
		std::string variable;
		float value;
		while (!in.eof()) {
			in >> variable;
			in >> value;
			if (variable == "stepSize") {
				kernelConstants_.stepSize = value;
			} else if (variable == "intensity") {
				kernelConstants_.intensity = value;
			} else {
				ERROR("Invalid variable name: " << variable);
				return false;
			}
		}
	}
	kernelConstants_.aDim = voxelData_->ADim();
	kernelConstants_.bDim = voxelData_->BDim();
	kernelConstants_.cDim = voxelData_->CDim();
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

void Raycaster::SetCubeShaderProgram(ShaderProgram *_cubeShaderProgram) {
  cubeShaderProgram_ = _cubeShaderProgram;
}

void Raycaster::SetQuadShaderProgram(ShaderProgram *_quadShaderProgram) {
  quadShaderProgram_ = _quadShaderProgram;
}

bool Raycaster::Render(float _timestep) {

	timeElapsed_ += _timestep;
  
	// Reset any errors
  glGetError();

  // TODO move init checks maybe baby
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

  // Set kernel constants that might have changed 

	// Advance the animation if it's time
  if (timeElapsed_ > animationRate_) {
		timeElapsed_ = (timeElapsed_-animationRate_); 
		if (currentTimestep_ < voxelData_->NumTimesteps()-1) {
			currentTimestep_++;
		} else {
			currentTimestep_ = 0;
		}
  }

  // Run kernel
  unsigned int timestepOffset = voxelData_->TimestepOffset(currentTimestep_);
	if (!clHandler_->BindTimestepOffset(timestepOffsetArg_, timestepOffset))
		return false;
	if (!clHandler_->RunRaycaster()) return false;

  // Output 

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

bool Raycaster::ReloadConfig() {
  INFO("Reloading config");
  return true;
  //return ReadShaderConfig(configFilename_);
}

bool Raycaster::HandleMouse() {
  if (leftMouseDown_) {
    pitch_ += 0.2f*(float)(currentMouseX_ - lastMouseX_);
    roll_ += 0.2f*(float)(currentMouseY_ - lastMouseY_);
  }
  return true;
}

bool Raycaster::HandleKeyboard() {
  if (KeyPressedNoRepeat('R')) {
    if (!ReloadConfig()) 
			return false;
		INFO("Config reloaded");
    if (!ReloadShaders()) 
			return false;
		INFO("Shaders reloaded");
		if (!UpdateKernelConfig()) 
			return false;
		INFO("Kernel config reloaded");
    if (!clHandler_->BindConstants(constantsArg_, &kernelConstants_)) 
			return false;
		INFO("Kernel constants reloaded");
		if (!ReloadTransferFunctions()) 
			return false;
		INFO("Transfer functions reloaded");
  }

  if (KeyPressed('W')) zoom_ -= 0.1f;
  if (KeyPressed('S')) zoom_ += 0.1f;

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
  if (!clHandler_->Init()) 
		return false;
  if (!clHandler_->CreateContext()) 
		return false;
	if (!clHandler_->BindTexture2D(cubeFrontArg_, cubeFrontTex_, true)) 
		return false;
	if (!clHandler_->BindTexture2D(cubeBackArg_, cubeBackTex_, true)) 
		return false;
	if (!clHandler_->BindTexture2D(quadArg_, quadTex_, false)) 
		return false;
	if (!clHandler_->CreateProgram("kernels/Raycaster.cl")) 
		return false;
	if (!clHandler_->BuildProgram()) 
		return false;
	if (!clHandler_->CreateKernel()) 
		return false;
	if (!clHandler_->CreateCommandQueue()) 
		return false;
	if (!clHandler_->BindVoxelData(voxelDataArg_, voxelData_)) 
		return false;
  if (!clHandler_->BindConstants(constantsArg_, &kernelConstants_)) 
		return false;
	if (!clHandler_->BindTransferFunction(transferFunctionArg_, 
		                                    transferFunctions_[0]))
		return false;
	return true;
}

void Raycaster::SetVoxelData(VoxelData<float> *_voxelData) {
	voxelData_ = _voxelData;
}

void Raycaster::SetKernelConfigFilename(const std::string &_filename) {
	kernelConfigFilename_ = _filename;
}

void Raycaster::AddTransferFunction(TransferFunction *_transferFunction) {
	transferFunctions_.push_back(_transferFunction);
}
