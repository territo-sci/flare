/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */
#include <GL/glew.h>
#include <BrickManager.h>
#include <Texture3D.h>
#include <Config.h>
#include <Utils.h>
#include <cmath>
//#include <boost/timer/timer.hpp>

using namespace osp;

BrickManager * BrickManager::New(Config *_config) {
  return new BrickManager(_config);
}


BrickManager::BrickManager(Config *_config)
  : textureAtlas_(NULL), config_(_config), atlasInitialized_(false), 
   hasReadHeader_(false), xCoord_(0), yCoord_(0), zCoord_(0) {

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
  // Number of bricks per frame
  numBricksFrame_ = numBricks_*numBricks_*numBricks_;

  // Calculate number of bricks in tree
  unsigned int numOTLevels = (unsigned int)(log((int)numBricks_)/log(2)+1);
  unsigned int numOTNodes = (unsigned int)((pow(8, numOTLevels) - 1) / 7);
  unsigned int numBSTNodes = (unsigned int)numTimesteps_*2 - 1;
  numBricksTree_ = numOTNodes * numBSTNodes;

  brickSize_ = sizeof(float)*numBrickVals_;
  volumeSize_ = brickSize_*numBricksFrame_;
  numValsTot_ = numBrickVals_*numBricksFrame_;

  hasReadHeader_ = true;

  // Hold two brick lists
  brickLists_.resize(2);
  // Make sure the brick list can hold the maximum number of bricks
  // Each entry holds tree coordinates
  brickLists_[EVEN].resize(numBricksTree_*3, -1);
  brickLists_[ODD].resize(numBricksTree_*3, -1);

  bricksInPBO_.resize(2);
  bricksInPBO_[EVEN].resize(numBricksTree_, -1);
  bricksInPBO_[ODD].resize(numBricksTree_, -1);

  usedCoords_.resize(2);
  usedCoords_[EVEN].resize(numBricksFrame_, false);
  usedCoords_[ODD].resize(numBricksFrame_, false);

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

void BrickManager::IncCoord() {
  // Update atlas coordinate
  xCoord_++;
  if (xCoord_ == xNumBricks_) {
    xCoord_ = 0;
    yCoord_++;
    if (yCoord_ == yNumBricks_) {
      yCoord_ = 0;
      zCoord_++;
      if (zCoord_ == zNumBricks_) {
        zCoord_ = 0;
      }
    }
  }
}

unsigned int BrickManager::LinearCoord(int _x, int _y, int _z) {
 return _x + _y*xNumBricks_ + _z*xNumBricks_*yNumBricks_;
}

void BrickManager::CoordsFromLin(int _idx, int &_x, int &_y, int &_z) {
  _x = _idx % xNumBricks_;
  _idx /= xNumBricks_;
  _y = _idx % yNumBricks_;
  _idx /= yNumBricks_;
  _z = _idx;
}

bool BrickManager::BuildBrickList(BUFFER_INDEX _bufIdx,
                                  std::vector<int> &_brickRequest) {


  int numBricks = 0;
  int numCached = 0;

  // For every non-zero entry in the request list, assign a texture atlas
  // coordinate. For zero entries, signal "no brick" using -1.
  for (unsigned int i=0; i<_brickRequest.size(); ++i) {

    if (_brickRequest[i] > 0) {

      //INFO("Checking brick " << i);

      // If the brick is already in the atlas, keep the coordinate
      if (bricksInPBO_[_bufIdx][i] != -1) {

      numCached++;
          
       // INFO("Brick " << i << " already in atlas");
        
        int x, y, z;
        CoordsFromLin(bricksInPBO_[_bufIdx][i], x, y, z);
        brickLists_[_bufIdx][3*i + 0] = x;
        brickLists_[_bufIdx][3*i + 1] = y;
        brickLists_[_bufIdx][3*i + 2] = z;
        //INFO("At coord " << x << " " << y << " " << z);

        // Mark coordinate as used
        usedCoords_[_bufIdx][bricksInPBO_[_bufIdx][i]] = true;

      } else {

        // If coord is already used, skip it and try the next one
        while (usedCoords_[_bufIdx][LinearCoord(xCoord_, yCoord_, zCoord_)]) {
          IncCoord();
        }

        brickLists_[_bufIdx][3*i + 0] = xCoord_;
        brickLists_[_bufIdx][3*i + 1] = yCoord_;
        brickLists_[_bufIdx][3*i + 2] = zCoord_;
        usedCoords_[_bufIdx][LinearCoord(xCoord_, yCoord_, zCoord_)] = true;
        
        IncCoord();
      }

      
      numBricks++;
      
    } else {

      brickLists_[_bufIdx][3*i + 0] = -1;
      brickLists_[_bufIdx][3*i + 1] = -1;
      brickLists_[_bufIdx][3*i + 2] = -1;

    }

    // Reset brick list during iteration
    _brickRequest[i] = 0;

  }

  // Brick list is build, reset coordinate list
  for (auto it=usedCoords_[_bufIdx].begin(); 
      it!=usedCoords_[_bufIdx].end(); ++it) {
    *it = false;
  }

  INFO("Num bricks used: " << numBricks);
  INFO("Num bricks cached: " << numCached);

  return true;
}

bool BrickManager::FillVolume(float *_in, float *_out, 
                              unsigned int _x, 
                              unsigned int _y, 
                              unsigned int _z) {

  //timer_.start();

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

        unsigned int idx = 
          xValCoord + 
          yValCoord*atlasDim_ +
          zValCoord*atlasDim_*atlasDim_;
          
          _out[idx] = _in[from];
          from++;
      }
    }
  }

  //timer_.stop();
  //double time = timer_.elapsed().wall / 1.0e9;
  //INFO("FillVolume: " << time << " s");

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
      // If not used, remove from PBO cache list
      bricksInPBO_[_pboIndex][brickIndex] = -1;
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
    //INFO("Reading " << sequence << " bricks");

    // Read the sequence into a buffer
    float *seqBuffer = new float[sequence*numBrickVals_];
    std::ios::pos_type offset = static_cast<std::ios::pos_type>(brickIndex) *
                                static_cast<std::ios::pos_type>(brickSize_);
    timer_.start();

    in_.seekg(dataPos_+offset);
    in_.read(reinterpret_cast<char*>(seqBuffer), brickSize_*sequence);

    timer_.stop();
    double time = timer_.elapsed().wall / 1.0e9;
    double mb = (brickSize_*sequence) / 1048576.0;
    //INFO("Disk read "<<mb<<" MB in "<<time<<" s, "<< mb/time<<" MB/s");

    // For each brick in the buffer, put it the correct buffer spot
    for (unsigned int i=0; i<sequence; ++i) {

      // Only upload if needed
      if (bricksInPBO_[_pboIndex][brickIndex+i] == -1) {

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

        // Update the atlas list since the brick will be uploaded
        //INFO(brickIndex+i);
        bricksInPBO_[_pboIndex][brickIndex+i] = LinearCoord(x, y, z);

      }
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
