/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <GL/glew.h>
#include <BrickManager.h>
#include <Texture3D.h>
#include <Utils.h>

using namespace osp;

BrickManager * BrickManager::New() {
  return new BrickManager();
}

BrickManager::BrickManager()
 : textureAtlas_(NULL) {

  brickBuffer_[ODD] = NULL;
  brickBuffer_[EVEN] = NULL;

  // TODO move
  glGenBuffers(1, &pboHandle_[EVEN]);
  glGenBuffers(2, &pboHandle_[ODD]);

}

BrickManager::~BrickManager() {
  if (in_.is_open()) {
    in_.close();
  }
  if (brickBuffer_) {
    delete[] brickBuffer_;
  }
}

void BrickManager::SetInFilename(const std::string &_inFilename) {
  inFilename_ = _inFilename;
}

bool BrickManager::ReadHeader() {
  
  in_.open(inFilename_.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in_.is_open()) {
    ERROR("Failed to open " << inFilename_);
    return false;
  }

  in_.seekg(std::ios_base::beg);
  
  size_t s = sizeof(unsigned int);
  in_.read(reinterpret_cast<char*>(&structure_), s);
  in_.read(reinterpret_cast<char*>(&dataDimensionality_), s);
  in_.read(reinterpret_cast<char*>(&xBrickDim_), s);
  in_.read(reinterpret_cast<char*>(&yBrickDim_), s);
  in_.read(reinterpret_cast<char*>(&zBrickDim_), s);
  in_.read(reinterpret_cast<char*>(&xNumBricks_), s);
  in_.read(reinterpret_cast<char*>(&yNumBricks_), s);
  in_.read(reinterpret_cast<char*>(&zNumBricks_), s);
  in_.read(reinterpret_cast<char*>(&numTimesteps_), s);
  in_.read(reinterpret_cast<char*>(&paddingWidth_), s);
  in_.read(reinterpret_cast<char*>(&dataSize_), s);
  
  INFO("Header reading complete");
  INFO("Structure: " << structure_);
  INFO("Data dimensionality: " << dataDimensionality_);
  INFO("Brick dims: " << xBrickDim_ << " " << yBrickDim_ <<" "<< zBrickDim_);
  INFO("Num bricks: " << xNumBricks_ <<" "<< yNumBricks_ <<" "<< zNumBricks_);
  INFO("Num timesteps: " << numTimesteps_);
  INFO("Padding width: " << paddingWidth_);
  INFO("Data size: " << dataSize_);
  INFO("");

  // Keep track of where in file brick data starts
  dataPos_ = in_.tellg();

  // Allocate box list and brick buffer
  // The box list keeps track of 1 brick index, 3 coordinate and 1 size value
  boxList_.resize(xNumBricks_*yNumBricks_*zNumBricks_*5);
  brickBuffer_[EVEN] = new real[xBrickDim_*yBrickDim_*zBrickDim_];
  brickBuffer_[ODD] = new real[xBrickDim_*yBrickDim_*zBrickDim_];

  hasReadHeader_ = true;

  return true;
}

bool BrickManager::InitAtlas() {

  if (atlasInitialized_) {
    WARNING("InitAtlas() - already initialized");
  }

  if (!hasReadHeader_) {
    ERROR("InitAtlas() - Has not read header");
    return false;
  }

  // Prepare the 3D texture
  std::vector<unsigned int> dims;
  dims.push_back(xBrickDim_*xNumBricks_);
  dims.push_back(yBrickDim_*yNumBricks_);
  dims.push_back(zBrickDim_*zNumBricks_);
  textureAtlas_ = Texture3D::New(dims);

  if (!textureAtlas_->Init()) return false;
  
  atlasInitialized_ = true;

  return true;

}


bool BrickManager::UpdateAtlas() {

  // Update all bricks in the brick list
  // TODO caching and size control structure
  // TODO possibly repoint instead of reuploading

  // Read first brick into mapped buffer
  if (!ReadBrick(boxList_[0], EVEN)) return false;

  // Loop over all bricks in box list
  unsigned int numBoxes = xNumBricks_*yNumBricks_*zNumBricks_;
  for (unsigned int i=0; i<numBoxes; ++i) {
 
    BUFFER_INDEX bufIndex = (i % 2 == 0) ? EVEN : ODD;
    BUFFER_INDEX nextIndex = (bufIndex == EVEN) ? ODD : EVEN;
     
    // Lookup the target atlas coordinates
    unsigned int brickIndex = boxList_[5*i];
    unsigned int x = boxList_[5*i+1];
    unsigned int y = boxList_[5*i+2];
    unsigned int z = boxList_[5*i+3];

    // Upload the brick to the right place
    // The 0 argument enables upload from bound buffer
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboHandle_[bufIndex]);
    if (!textureAtlas_->UpdateSubRegion(x*xBrickDim_,
                                        y*yBrickDim_,
                                        z*zBrickDim_,
                                        xBrickDim_,
                                        yBrickDim_,
                                        zBrickDim_,
                                        0)) return false;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    // Read the next brick from file to the brick buffer
    if (i < numBoxes-1) {
      unsigned int nextBrickIndex = boxList_[5*(i+1)];
      if (!ReadBrick(nextBrickIndex, nextIndex)) return false;
    }
    
  }

  return true;

}

bool BrickManager::UpdateBrickList(unsigned int _brickIndex, 
                                   AtlasCoords _atlasCoords) {
  /*
  if (!ReadBrick(_brickIndex)) return false;

  unsigned int xOffset = _atlasCoords.x_ * xBrickDim_;
  unsigned int yOffset = _atlasCoords.y_ * yBrickDim_;
  unsigned int zOffset = _atlasCoords.z_ * zBrickDim_;

  if (!textureAtlas_->UpdateSubRegion(xOffset, yOffset, zOffset,
                                      xBrickDim_, yBrickDim_, zBrickDim_,
                                      brickBuffer_)) return false;
  */

  brickList_[_brickIndex] = _atlasCoords;

  return true;
}

bool BrickManager::InAtlas(unsigned int _brickIndex, 
                           AtlasCoords &_atlasCoords) {
  if (brickList_.find(_brickIndex) != brickList_.end()) {
    _atlasCoords = brickList_[_brickIndex];
    return true;
  } else {
    return false;
  }
}


bool BrickManager::UpdateBoxList(unsigned int _boxIndex, 
                                 unsigned int _brickIndex) {
  // Update the box list with the corresponding brick informatioon
  boxList_[5*_boxIndex] = _brickIndex;
  boxList_[5*_boxIndex+1] = brickList_[_brickIndex].x_;
  boxList_[5*_boxIndex+2] = brickList_[_brickIndex].y_;
  boxList_[5*_boxIndex+3] = brickList_[_brickIndex].z_;
  boxList_[5*_boxIndex+4] = brickList_[_brickIndex].size_;  
  return true;
}


bool BrickManager::ReadBrick(unsigned int _brickIndex, 
                             BUFFER_INDEX _bufferIndex) {
  
  if (!hasReadHeader_) {
    ERROR("ReadBrick() - Has not read header");
    return false;
  }

  if (!atlasInitialized_) {
    ERROR("ReadBrick() - Has not initialized atlas");
    return false;
  }

  // Number of values to be read per brick
  unsigned int numVals = xBrickDim_*yBrickDim_*zBrickDim_;

  // Total size in bytes for one brick
  size_t brickSize = sizeof(real) * numVals;

  // Offset in filestream
  std::ios::pos_type offset = dataPos_ +
    static_cast<std::ios::pos_type>(_brickIndex * brickSize);

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboHandle_[_bufferIndex]);

  unsigned int size = xBrickDim_*yBrickDim_*zBrickDim_*sizeof(real);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, size, 0, GL_STREAM_DRAW);
  
  // Map PBO
  brickBuffer_[_bufferIndex] = reinterpret_cast<real*>(
    glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));  

  // Read
  in_.seekg(offset);
  in_.read(reinterpret_cast<char*>(brickBuffer_[_bufferIndex]), brickSize);

  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
   
  return true;
}
