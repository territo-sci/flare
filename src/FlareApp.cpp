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
#include <SGCTWinManager.h>
#include <sgct.h>

using namespace osp;

int main(int argc, char **argv) {

  // Start with reading a config file
  Config *config = Config::New("../config/flareConfig.txt");
  if (!config) exit(1);

  // Window dimensions
  unsigned int width = config->WinWidth();
  unsigned int height = config->WinHeight();

  // Create TSP structure from file
  TSP *tsp = TSP::New(config);
  if (!tsp->Construct()) exit(1);
  // Run error calculations
  //if (!tsp->CalculateSpatialError()) exit(1);
  //if (!tsp->CalculateTemporalError()) exit(1);

  // Init the singleton window manager and set up OpenGL context
  SGCTWinManager::Instance()->InitEngine(argc,  argv, 
    sgct::Engine::OpenGL_4_2_Core_Profile);

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

  // Set up the raycaster
  Raycaster *raycaster = Raycaster::New(config);
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

  // Tie stuff together
  SGCTWinManager::Instance()->SetConfig(config);
  SGCTWinManager::Instance()->SetRenderer(raycaster);

  // Go!
  SGCTWinManager::Instance()->Render();

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
  delete animator;
  delete raycaster;
  delete config;

  exit(0);
}
