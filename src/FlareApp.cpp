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
  Config *config = Config::New("config/flareConfig.txt");
  if (!config) exit(1);

  // Init the singleton window manager and set up OpenGL context
  SGCTWinManager::Instance()->SetConfig(config);
  SGCTWinManager::Instance()->InitEngine(argc,  argv, 
    sgct::Engine::OpenGL_4_2_Core_Profile);

  // Get the viewport coordinates from OpenGL
  GLint currentViewPort[4];
  glGetIntegerv( GL_VIEWPORT, currentViewPort);
  
  // Make sure texture width/height and global kernel worksizes are 
  // multiples of the local worksize
 
  // Window dimensions
  unsigned int width = currentViewPort[2] - currentViewPort[0];
  unsigned int height = currentViewPort[3] - currentViewPort[1];
  unsigned int xFactor = width/config->LocalWorkSizeX();
  unsigned int yFactor = height/config->LocalWorkSizeY();
  width = xFactor * config->LocalWorkSizeX();
  height = yFactor * config->LocalWorkSizeY();
  width /= config->TextureDivisionFactor();
  height /= config->TextureDivisionFactor();

  // Create TSP structure from file
  TSP *tsp = TSP::New(config);  
  if (!tsp->ReadHeader()) exit(1);
  // Read cache if it exists, calculate otherwise
  if (tsp->ReadCache()) {
    INFO("\nUsing cached TSP file");
  } else {
    INFO("\nNo cached TSP file found");
    if (!tsp->Construct()) exit(1);
    if (!tsp->CalculateSpatialError()) exit(1);
    if (!tsp->CalculateTemporalError()) exit(1);
    if (!tsp->WriteCache()) exit(1);
  }

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
  if (!raycaster->InitCL()) exit(1);
  if (!raycaster->InitPipeline()) exit(1);

  // Tie stuff together
  SGCTWinManager::Instance()->SetAnimator(animator);
  SGCTWinManager::Instance()->SetRaycaster(raycaster);

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
