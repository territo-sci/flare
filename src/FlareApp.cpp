#include <WindowManager.h>
#include <Renderer.h>
#include <Texture.h>
#include <Texture2D.h>
#include <Raycaster.h>
#include <ShaderProgram.h>
#include <VDFReader.h>
#include <VoxelData.h>
#include <string>
#include <cstdlib>
#include <vector>

using namespace osp;

int main() {

	// Read data
	VoxelData<float> *floatData = new VoxelData<float>();
	VDFReader *reader_ = VDFReader::New();
	reader_->SetVoxelData(floatData);
	if (!reader_->Read("data/enlilTestData.vdf")) exit(1);

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

  // Create a raycaster and set it up
  Raycaster * raycaster = Raycaster::New();
  raycaster->SetWinWidth(width);
  raycaster->SetWinHeight(height);
  raycaster->InitMatrices();
  raycaster->InitCube();
  raycaster->InitQuad();
  raycaster->SetCubeFrontTexture(cubeFrontTex);
  raycaster->SetCubeBackTexture(cubeBackTex);
  raycaster->SetQuadTexture(quadTex);
  raycaster->SetCubeShaderProgram(cubeShaderProgram);
  raycaster->SetQuadShaderProgram(quadShaderProgram);
  raycaster->InitFramebuffers();
	raycaster->SetVoxelData(floatData);
  raycaster->InitCL();

  // Go!
  manager->SetRenderer(raycaster);
  manager->StartLoop();

  // Clean up, like a good citizen
  delete cubeFrontTex;
  delete cubeBackTex;
  delete cubeShaderProgram;
  delete quadShaderProgram;
  delete manager;

  exit(0);
}
