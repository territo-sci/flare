#include <GL/glew.h>
#include <WindowManager.h>
#include <Renderer.h>
#include <Texture.h>
#include <Texture2D.h>
#include <Raycaster.h>
#include <ShaderProgram.h>
#include <TransferFunction.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <Animator.h>
#include <BrickManager.h>
#include <TSP.h>
#include <CLManager.h>  
#include <Config.h>
#include <Utils.h>
#include <sgct.h>


using namespace osp;

// TEMP for SGCT testing
// TODO don't use global scope
// encapsulate in classes
Raycaster *raycaster;
sgct::Engine *gEngine;

void drawFunc();
void initNav();
void updateNav(); 

std::vector<int> keysToCheck;
float oldTime;
float currentTime;

int main(int argc, char **argv) {

  // Start with reading a config file
  Config *config = Config::New("../config/flareConfig.txt");
  if (!config) exit(1);

  // Window dimensions
  unsigned int width = config->WinWidth();
  unsigned int height = config->WinHeight();

  // Create a WindowManager, open window to init GLEW and GLFW.
  // This has to be done before other OpenGL/CL stuff can be initialized.
  //WindowManager *manager = WindowManager::New(width, height, "FlareApp");
  // if (!manager->OpenWindow()) exit(1);

  // SGCT

  gEngine = new sgct::Engine(argc, argv);

  gEngine->setDrawFunction(drawFunc);
  
  // TODO select core profile with config
  if (!gEngine->init(sgct::Engine::OpenGL_4_2_Core_Profile)) {
    delete gEngine;
    ERROR("Failed to init SGCT engine");
  }

  // Create TSP structure from file
  TSP *tsp = TSP::New(config);
  if (!tsp->Construct()) exit(1);
  //if (!tsp->CalculateSpatialError()) exit(1);
  //if (!tsp->CalculateTemporalError()) exit(1);

  // Create brick manager and init (has to be done after init OpenGL!)
  BrickManager *brickManager= BrickManager::New(config);
  if (!brickManager->ReadHeader()) exit(1);
  if (!brickManager->InitAtlas()) exit(1);

  // Create shaders for color cube and output textured quad
  ShaderProgram *cubeShaderProgram = ShaderProgram::New();
  cubeShaderProgram->CreateShader(ShaderProgram::VERTEX,
                                  config->CubeShaderVertFilename());
  cubeShaderProgram->CreateShader(ShaderProgram::FRAGMENT,
                                  config->CubeShaderFragFilename());
  cubeShaderProgram->CreateProgram();
  
  ShaderProgram *quadShaderProgram = ShaderProgram::New();
  quadShaderProgram->CreateShader(ShaderProgram::VERTEX,
                                  config->QuadShaderVertFilename());
  quadShaderProgram->CreateShader(ShaderProgram::FRAGMENT,
                                  config->QuadShaderFragFilename());
  quadShaderProgram->CreateProgram();

  // Create two textures to hold the color cube
  std::vector<unsigned int> dimensions(2);
  dimensions[0] = width;
  dimensions[1] = height;
  Texture2D *cubeFrontTex = Texture2D::New(dimensions);
  Texture2D *cubeBackTex = Texture2D::New(dimensions);
  cubeFrontTex->Init();
  cubeBackTex->Init();

  // Create an output texture to write to
  Texture2D *quadTex = Texture2D::New(dimensions);
  quadTex->Init();
  
  // Create transfer functions
  TransferFunction *transferFunction = TransferFunction::New();
  transferFunction->SetInFilename(config->TFFilename());
  if (!transferFunction->ReadFile()) exit(1);
  if (!transferFunction->ConstructTexture()) exit(1);

  // Create animator
  Animator *animator = Animator::New(config);
  animator->SetNumTimesteps(brickManager->NumTimesteps());

  // Create CL manager
  CLManager *clManager = CLManager::New();

  // Create a raycaster and set it up
  raycaster = Raycaster::New(config, gEngine);
  raycaster->SetWinWidth(width);
  raycaster->SetWinHeight(height);
  raycaster->InitMatrices();
  if (!raycaster->InitCube()) exit(1);
  if (!raycaster->InitQuad()) exit(1);
  raycaster->SetBrickManager(brickManager);
  raycaster->SetCubeFrontTexture(cubeFrontTex);
  raycaster->SetCubeBackTexture(cubeBackTex);
  raycaster->SetQuadTexture(quadTex);
  raycaster->SetCubeShaderProgram(cubeShaderProgram);
  raycaster->SetQuadShaderProgram(quadShaderProgram);
  if (!raycaster->InitFramebuffers()) exit(1);
  raycaster->SetAnimator(animator);
  raycaster->AddTransferFunction(transferFunction);
  // Tie CL manager to renderer
  raycaster->SetCLManager(clManager);
  raycaster->SetTSP(tsp);
  if (!raycaster->InitCL()) return false;

  initNav();
  gEngine->render();

  // Go!
  //manager->SetRenderer(raycaster);
  //manager->StartLoop();

  // Clean up like a good citizen

  delete clManager;
  delete tsp;
  delete brickManager;
  delete cubeFrontTex;
  delete cubeBackTex;
  delete quadTex;
  delete cubeShaderProgram;
  delete quadShaderProgram;
  delete transferFunction;
  //delete manager;
  delete animator;
  delete raycaster;
  delete config;
  delete gEngine;

  

  exit(0);
}


void drawFunc() {

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  updateNav();

  raycaster->Render(0.5);
}

void initNav() {

  keysToCheck.push_back('R');
  keysToCheck.push_back('W');
  keysToCheck.push_back('S');
  keysToCheck.push_back('F');
  keysToCheck.push_back(32);
  keysToCheck.push_back('Z');
  keysToCheck.push_back('X');
  keysToCheck.push_back('P');
  keysToCheck.push_back('T');
  keysToCheck.push_back('U');

}

void updateNav() {

  for (auto it = keysToCheck.begin(); it != keysToCheck.end(); it++) {
    raycaster->SetKeyPressed(*it, (glfwGetKey(*it) == GLFW_PRESS));
  }

  int xMouse, yMouse;
  glfwGetMousePos(&xMouse, &yMouse);
  raycaster->SetMousePosition((float)xMouse, (float)yMouse);

  bool leftButton = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
  bool rightButton = glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT)==GLFW_PRESS;
  raycaster->SetMousePressed(leftButton, rightButton);


}