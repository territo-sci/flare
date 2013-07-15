/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <GL/glew.h>
#include <BrickManager.h>
#include <Texture3D.h>
#include <Utils.h>
#include <boost/timer/timer.hpp>

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
  //boxList_.resize(xNumBricks_*yNumBricks_*zNumBricks_*5);

  brickDim_ = xBrickDim_;
  numBricks_ = xNumBricks_;
  paddedBrickDim_ = brickDim_ + paddingWidth_*2;
  atlasDim_ = paddedBrickDim_*numBricks_; 
  
  INFO("Padded brick dim: " << paddedBrickDim_); 
  INFO("Atlas dim: " << atlasDim_);

  unsigned int bufferSize = paddedBrickDim_*paddedBrickDim_*paddedBrickDim_;
  brickBuffer_[EVEN] = new real[bufferSize];
  brickBuffer_[ODD] = new real[bufferSize];

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
  dims.push_back(atlasDim_);
  dims.push_back(atlasDim_);
  dims.push_back(atlasDim_);
  textureAtlas_ = Texture3D::New(dims);

  if (!textureAtlas_->Init()) return false;
  
  atlasInitialized_ = true;

  return true;

}

bool BrickManager::BuildBrickList(std::vector<int> _brickRequest) {

  int numBricks = 0;

  // Assume the brick request list has the correct size
  // TODO init step
  brickList_.resize(_brickRequest.size()*3);

  // For every non-zero entry in the request list, assign a texture atlas
  // coordinate. For zero entries, signal "no brick" using -1.
  int xCoord = 0;
  int yCoord = 0;
  int zCoord = 0;
  for (unsigned int i=0; i<_brickRequest.size(); ++i) {
    if (_brickRequest[i] > 0) {
        
      if (xCoord >= xNumBricks_ || 
          yCoord >= yNumBricks_ || 
          zCoord >= zNumBricks_) {
        ERROR("atlas coord too large!");
        return false;
      }

      brickList_[3*i + 0] = xCoord;
      brickList_[3*i + 1] = yCoord;
      brickList_[3*i + 2] = zCoord;
      
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

      brickList_[3*i + 0] = -1;
      brickList_[3*i + 1] = -1;
      brickList_[3*i + 2] = -1;
    
    }
  }

  return true;
}


bool BrickManager::UpdateAtlas() {

  // TEST non-alternating
  unsigned int brickIndex = 0;
  while (brickIndex < brickList_.size()/3) {
    while (brickList_[3*brickIndex] == -1 &&
           brickIndex < brickList_.size()/3) {
      brickIndex++;
    }
    if (brickIndex == brickList_.size()/3) return true;
    ReadBrick(brickIndex, EVEN);
    unsigned int x = static_cast<unsigned int>(brickList_[3*brickIndex+0]);
    unsigned int y = static_cast<unsigned int>(brickList_[3*brickIndex+1]);
    unsigned int z = static_cast<unsigned int>(brickList_[3*brickIndex+2]);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboHandle_[EVEN]);
    if (!textureAtlas_->UpdateSubRegion(x*paddedBrickDim_,
                                        y*paddedBrickDim_,
                                        z*paddedBrickDim_,
                                        paddedBrickDim_,
                                        paddedBrickDim_,
                                        paddedBrickDim_,
                                        0)) return false;
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    brickIndex++;
  }
  // END TEST

  /*

  // Update all bricks in the brick list
  // TODO caching and size control structure

  // Read first brick into mapped buffer
  unsigned int brickIndex = 0;
  while (brickList_[3*brickIndex] == -1) {
    brickIndex++;
  }
  if (!ReadBrick(brickIndex, EVEN)) return false;

  bool even = true;

  // Loop over all bricks
  while (brickIndex < brickList_.size()/3) {
    
    BUFFER_INDEX bufIndex = (even) ? EVEN : ODD;
    BUFFER_INDEX nextIndex = (even) ? ODD : EVEN;
     
    // Lookup the target atlas coordinates
    if (brickList_[3*brickIndex+0] < 0 ||
        brickList_[3*brickIndex+1] < 0 ||
        brickList_[3*brickIndex+2] < 0) {
      ERROR("Trying to read invalid brick position (-1)");
      return false;
    }
    unsigned int x = static_cast<unsigned int>(brickList_[3*brickIndex+0]);
    unsigned int y = static_cast<unsigned int>(brickList_[3*brickIndex+1]);
    unsigned int z = static_cast<unsigned int>(brickList_[3*brickIndex+2]);

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
    even = !even;
    brickIndex++;
    while (brickList_[3*brickIndex] == -1 &&
           brickIndex < brickList_.size()/3) {
      brickIndex++;
    }
    if (brickIndex != brickList_.size()/3) {
      if (!ReadBrick(brickIndex, nextIndex)) return false;
    }
    
  }

  */

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
  unsigned int numVals = paddedBrickDim_*paddedBrickDim_*paddedBrickDim_;

  // Total size in bytes for one brick
  size_t brickSize = sizeof(real) * numVals;

  // Offset in filestream
  std::ios::pos_type offset = dataPos_ +
    static_cast<std::ios::pos_type>(_brickIndex * brickSize);

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboHandle_[_bufferIndex]);

  glBufferData(GL_PIXEL_UNPACK_BUFFER, brickSize, 0, GL_STREAM_DRAW);
  
  // Map PBO
  brickBuffer_[_bufferIndex] = reinterpret_cast<real*>(
    glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));  

  // Read

  real *buffer = new real[numVals];

  in_.seekg(offset);
  /*
  boost::timer::cpu_timer t;
  t.start();
  */
  in_.read(reinterpret_cast<char*>(brickBuffer_[_bufferIndex]), brickSize);
  /*
  t.stop();
  double time = t.elapsed().wall / 1.0e9;
  double mb = brickSize / 1048579.0;
  INFO(mb << std::fixed << " MBs in " << time << " s, " << mb/time << "MB/s");
  */

  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  return true;
}



/*
void BrickManager::Print() {
    std::cout << "\nBRICK LIST " << std::endl;
    std::cout << "Constructed from " << inFilename_ << std::endl;

    std::cout << "Structure: " << structure_ << std::endl;
    std::cout << "Data dimensionality: " << dataDimensionality_ << std::endl;
    std::cout << "Brick dimensions: [" << xBrickDim_ << " " <<
                                          yBrickDim_ << " " << 
                                          zBrickDim_ << "]" << std::endl;
    std::cout << "Num bricks: [" << xNumBricks_ << " " <<
                                    yNumBricks_ << " " <<
                                    zNumBricks_ << "]" << std::endl;
    std::cout << "Num timesteps: " << numTimesteps_ << std::endl;
    std::cout << "Padding width: " << paddingWidth_ << std::endl;
    std::cout << "Data size: " << dataSize_ << std::endl;

    std::cout << "Bricks: " << std::endl;
    unsigned int i = 0;
    for (auto it=boxList_.begin(); it!=boxList_.end(); it+=5) {
      std::cout << "Box number: " << i++ << std::endl;
      std::cout << "Brick index: " << *it << std::endl;
      AtlasCoords ac = brickList_[*it];
      std::cout << "Atlas coords: [" << ac.x_ << " " << ac.y_ << " " << 
                                        ac.z_ << " " << ac.size_ << "]" <<
                                        std::endl;
    }
}
    */
