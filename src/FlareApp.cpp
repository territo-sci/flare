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

using namespace osp;

int main() {

  // File to create TSP structure and then read data from
  std::string tspFileName = "/home/vsand/OpenSpace/enlil_256_32_32.tsp";

  // Window dimensions
  unsigned int width = 512;
  unsigned int height = 512;

  // Create a WindowManager, open window to init GLEW and GLFW.
  // This has to be done before other OpenGL/CL stuff can be initialized.
  WindowManager *manager = WindowManager::New(width, height, "FlareApp");
  if (!manager->OpenWindow()) exit(1);

  // Create TSP structure from file
  TSP *tsp = TSP::New(tspFileName);
  tsp->Construct();

  // Create brick manager and init (has to be done after init OpenGL!)
  BrickManager *brickManager= BrickManager::New();
  brickManager->SetInFilename(tspFileName);
  if (!brickManager->ReadHeader()) exit(1);
  if (!brickManager->InitAtlas()) exit(1);

  // Create shaders for color cube and output textured quad
  ShaderProgram *cubeShaderProgram = ShaderProgram::New();
  cubeShaderProgram->CreateShader(ShaderProgram::VERTEX,
                                  "shaders/cubeVert.glsl");
  cubeShaderProgram->CreateShader(ShaderProgram::FRAGMENT,
                                  "shaders/cubeFrag.glsl");
  cubeShaderProgram->CreateProgram();
  
  ShaderProgram *quadShaderProgram = ShaderProgram::New();
  quadShaderProgram->CreateShader(ShaderProgram::VERTEX,
                                  "shaders/quadVert.glsl");
  quadShaderProgram->CreateShader(ShaderProgram::FRAGMENT,
                                  "shaders/quadFrag.glsl");
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
  transferFunction->SetInFilename("transferfunctions/test.txt");
  //transferFunction->SetInFilename("transferfunctions/plain.txt");
  transferFunction->ReadFile();
  transferFunction->ConstructTexture();

  //Create animator
  Animator *animator = Animator::New();
  animator->SetNumTimesteps(brickManager->NumTimesteps());
  animator->SetRefreshInterval(0.05f);

  // Create CL manager
  CLManager *clManager = CLManager::New();

  // Create a raycaster and set it up
  Raycaster * raycaster = Raycaster::New();
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
  raycaster->SetKernelConfigFilename("config/kernelConstants.txt");
  if (!raycaster->UpdateKernelConfig()) exit(1);
  // Tie CL manager to renderer
  raycaster->SetCLManager(clManager);
  raycaster->SetTSP(tsp);
  if (!raycaster->InitCL()) return false;


  // Go!
  manager->SetRenderer(raycaster);
  // We expect a false return from the loop, so exit with normal code
  if (!manager->StartLoop()) exit(0);

  // Clean up, like a good citizen
  delete clManager;
  delete tsp;
  delete brickManager;
  delete cubeFrontTex;
  delete cubeBackTex;
  delete quadTex;
  delete cubeShaderProgram;
  delete quadShaderProgram;
  delete transferFunction;
  delete manager;
  delete animator;
  delete raycaster;

  exit(0);
}
