/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */
#include <GL/glew.h>
#include <BrickManager.h>
#include <Texture3D.h>
#include <Config.h>
#include <Utils.h>
//#include <boost/timer/timer.hpp>

using namespace osp;

BrickManager * BrickManager::New(Config *_config) {
  return new BrickManager(_config);
}


BrickManager::BrickManager(Config *_config)
  : textureAtlas_(NULL), config_(_config), atlasInitialized_(false), 
   hasReadHeader_(false)
{

  // TODO move
  glGenBuffers(1, &pboHandle_[EVEN]);
  glGenBuffers(2, &pboHandle_[ODD]);

}

BrickManager::~BrickManager() {
  if (in_.is_open()) {
    in_.close();
  }
}


bool BrickManager::ReadHeader() {
  
  if (!config_) {
    ERROR("No config set");
    return false;
  }

  std::string inFilename = config_->TSPFilename();
  in_.open(inFilename.c_str(), std::ios_base::in | std::ios_base::binary);

  if (!in_.is_open()) {
    ERROR("Failed to open " << inFilename);
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
  //boxList_.resize(xNumBricks_*yNumBricks_*zNumBricks_*5);

  brickDim_ = xBrickDim_;
  numBricks_ = xNumBricks_;
  paddedBrickDim_ = brickDim_ + paddingWidth_*2;
  atlasDim_ = paddedBrickDim_*numBricks_; 
  
  INFO("Padded brick dim: " << paddedBrickDim_); 
  INFO("Atlas dim: " << atlasDim_);

  numBrickVals_ = paddedBrickDim_*paddedBrickDim_*paddedBrickDim_;
  numBricksTot_ = numBricks_*numBricks_*numBricks_;
  brickSize_ = sizeof(float)*numBrickVals_;
  volumeSize_ = brickSize_*numBricksTot_;
  numValsTot_ = numBrickVals_*numBricksTot_;

  hasReadHeader_ = true;

  // Hold two brick lists
  brickLists_.resize(2);

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
  dims.push_back(atlasDim_);
  dims.push_back(atlasDim_);
  dims.push_back(atlasDim_);
  textureAtlas_ = Texture3D::New(dims);

  if (!textureAtlas_->Init()) return false;
  
  atlasInitialized_ = true;

  return true;

}

bool BrickManager::BuildBrickList(BUFFER_INDEX _bufIdx,
                                  std::vector<int> &_brickRequest) {

  int numBricks = 0;

  // Assume the brick request list has the correct size
  // TODO init step
  brickLists_[_bufIdx].resize(_brickRequest.size()*3);

  // For every non-zero entry in the request list, assign a texture atlas
  // coordinate. For zero entries, signal "no brick" using -1.
  int xCoord = 0;
  int yCoord = 0;
  int zCoord = 0;
  for (unsigned int i=0; i<_brickRequest.size(); ++i) {
    if (_brickRequest[i] > 0) {
        
      if (xCoord >= static_cast<int>(xNumBricks_) || 
          yCoord >= static_cast<int>(yNumBricks_) || 
          zCoord >= static_cast<int>(zNumBricks_)) {
        ERROR("atlas coord too large!");
        return false;
      }

      brickLists_[_bufIdx][3*i + 0] = xCoord;
      brickLists_[_bufIdx][3*i + 1] = yCoord;
      brickLists_[_bufIdx][3*i + 2] = zCoord;
      
      numBricks++;

      // Update atlas coordinate
      xCoord++;
      if (xCoord == xNumBricks_) {
        xCoord = 0;
        yCoord++;
        if (yCoord == yNumBricks_) {
          yCoord = 0;
          zCoord++;
        }
      }

    } else {

      brickLists_[_bufIdx][3*i + 0] = -1;
      brickLists_[_bufIdx][3*i + 1] = -1;
      brickLists_[_bufIdx][3*i + 2] = -1;
    
    }

    // Reset brick list during iteration
    _brickRequest[i] = 0;

  }

  //INFO("Num bricks used: " << numBricks);

  return true;
}

bool BrickManager::FillVolume(float *_in, float *_out, 
                              unsigned int _x, 
                              unsigned int _y, 
                              unsigned int _z) {

  unsigned int xMin = _x*paddedBrickDim_;
  unsigned int yMin = _y*paddedBrickDim_;
  unsigned int zMin = _z*paddedBrickDim_;
  unsigned int xMax = xMin+paddedBrickDim_;
  unsigned int yMax = yMin+paddedBrickDim_;
  unsigned int zMax = zMin+paddedBrickDim_;

  // Loop over the brick using three loops
  unsigned int from = 0;
  for (unsigned int zValCoord=zMin; zValCoord<zMax; ++zValCoord) {
    for (unsigned int yValCoord=yMin; yValCoord<yMax; ++yValCoord) {
      for (unsigned int xValCoord=xMin; xValCoord<xMax; ++xValCoord) {
        //INFO(xValCoord << " " << yValCoord << " " << zValCoord);
        unsigned int idx = 
          xValCoord + 
          yValCoord*atlasDim_ +
          zValCoord*atlasDim_*atlasDim_;

        _out[idx] = _in[from];
        from++;
      }
    }
  }

  return true;
}

// TODO find buffer size
bool BrickManager::DiskToPBO(BUFFER_INDEX _pboIndex) {

  // Map PBO
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboHandle_[_pboIndex]);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, volumeSize_, 0, GL_STREAM_DRAW);
  float *mappedBuffer = reinterpret_cast<float*>(
    glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));

  if (!mappedBuffer) {
    ERROR("Failed to map PBO");
    return false;
  }

  // Loop over brick request list
  unsigned int brickIndex = 0;
  while (brickIndex < brickLists_[_pboIndex].size()/3) {

    // Find first active brick index in list
    while (brickIndex<brickLists_[_pboIndex].size()/3 && 
          brickLists_[_pboIndex][3*brickIndex]== -1) {
      brickIndex++;
    }

    // If we are at the end of the list, exit
    if (brickIndex == brickLists_[_pboIndex].size()/3) {
      break;
    }

    // Find a sequence of consecutive bricks in list
    unsigned int sequence = 0;
    unsigned int brickIndexProbe = brickIndex;
    while (brickIndexProbe < brickLists_[_pboIndex].size()/3 &&
           brickLists_[_pboIndex][3*brickIndexProbe] != -1) {
      sequence++;
      brickIndexProbe++;
    }

    // Read the sequence into a buffer
    float *seqBuffer = new float[sequence*numBrickVals_];
    std::ios::pos_type offset = static_cast<std::ios::pos_type>(brickIndex) *
                                static_cast<std::ios::pos_type>(brickSize_);
    in_.seekg(dataPos_+offset);
    in_.read(reinterpret_cast<char*>(seqBuffer), brickSize_*sequence);

    // For each brick in the buffer, put it the correct buffer spot
    for (unsigned int i=0; i<sequence; ++i) {

      unsigned int x=static_cast<unsigned int>(
        brickLists_[_pboIndex][3*(brickIndex+i)+0]);
      unsigned int y=static_cast<unsigned int>(
        brickLists_[_pboIndex][3*(brickIndex+i)+1]);
      unsigned int z=static_cast<unsigned int>(
        brickLists_[_pboIndex][3*(brickIndex+i)+2]);

      // Put each brick in the correct buffer place.
      // This needs to be done because the values are in brick order, and
      // the volume needs to be filled with one big float array.
      FillVolume(&seqBuffer[numBrickVals_*i], mappedBuffer, x, y, z);  
    }

    delete[] seqBuffer;

    // Update the brick index
    brickIndex += sequence;

  }

  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  return true;
}

bool BrickManager::PBOToAtlas(BUFFER_INDEX _pboIndex) {
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboHandle_[_pboIndex]);
  if (!textureAtlas_->UpdateSubRegion(0, 0, 0,
                                      textureAtlas_->Dim(0),
                                      textureAtlas_->Dim(1),
                                      textureAtlas_->Dim(2),
                                      0)) return false;
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
   return true;
}
