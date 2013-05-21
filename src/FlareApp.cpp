#include <WindowManager.h>
#include <Renderer.h>
#include <Texture.h>
#include <Texture2D.h>
#include <Raycaster.h>
#include <ShaderProgram.h>
#include <VDFReader.h>
#include <TransferFunction.h>
#include <VoxelData.h>
#include <string>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <Animator.h>

using namespace osp;

int main() {

  // Read data
  VoxelData<float> *floatData = new VoxelData<float>();
  VDFReader *reader_ = VDFReader::New();
  reader_->SetVoxelData(floatData);
  if (!reader_->Read("/home/vsand/OpenSpace/enlilTestData_256_256_256.vdf")) exit(1);

  unsigned int width = 512;
  unsigned int height = 512;

  // Create a WindowManager, open window to init GLEW and GLFW
  WindowManager *manager = WindowManager::New(width, height, "FlareApp");
  manager->OpenWindow();

  // Create shaders
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

  // Create two textures to hold the raycaster's cube
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
  transferFunction->ReadFile();
  std::cout << *transferFunction << std::endl;
  transferFunction->ConstructTexture();

  //Create animator
  Animator *animator = Animator::New();
  animator->SetNumTimesteps(floatData->NumTimesteps());
  animator->SetRefreshInterval(0.07f);

  // Create a raycaster and set it up
  Raycaster * raycaster = Raycaster::New();
  raycaster->SetWinWidth(width);
  raycaster->SetWinHeight(height);
  raycaster->InitMatrices();
  if (!raycaster->InitCube()) exit(1);
  if (!raycaster->InitQuad()) exit(1);
  raycaster->SetCubeFrontTexture(cubeFrontTex);
  raycaster->SetCubeBackTexture(cubeBackTex);
  raycaster->SetQuadTexture(quadTex);
  raycaster->SetCubeShaderProgram(cubeShaderProgram);
  raycaster->SetQuadShaderProgram(quadShaderProgram);
  raycaster->InitFramebuffers();
  raycaster->SetVoxelData(floatData);
  // if (!raycaster->InitPixelBuffers()) exit(1);
  raycaster->SetAnimator(animator);
  //raycaster->SetAnimationRate(0.08f);
  raycaster->AddTransferFunction(transferFunction);
  raycaster->SetKernelConfigFilename("config/kernelConstants.txt");
  raycaster->UpdateKernelConfig();
  if (!raycaster->PopulateVolumeTexture()) exit(1);
  if (!raycaster->InitCL()) exit(1);

  // Go!
  manager->SetRenderer(raycaster);
  // We expect a false return from the loop, so exit with normal code
  if (!manager->StartLoop()) exit(0);

  // Clean up, like a good citizen
  delete floatData;
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
