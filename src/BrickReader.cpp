/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <BrickReader.h>
#include <fstream>
#include <iostream>
#include <Utils.h>

using namespace osp;

BrickReader * BrickReader::New() {
  return new BrickReader;
}

BrickReader::BrickReader() 
  : structure_(0), dataDimensionality_(0), xBrickDim_(0), yBrickDim_(0),
    zBrickDim_(0), xNumBricks_(0), yNumBricks_(0), zNumBricks_(0),
    numTimesteps_(0), paddingWidth_(0), dataSize_(0), brick_(NULL) {}

BrickReader::~BrickReader() {
  if (brick_ != NULL) {
    delete[] brick_;
  }
  if (in_.is_open()) {
    in_.close();
  }
}

void BrickReader::SetInFilename(std::string _inFilename) {
  inFilename_ = _inFilename;
}

bool BrickReader::ReadHeader() {

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

  // Prepare data storage
  brick_ = new real[xBrickDim_*yBrickDim_*zBrickDim_];
  
  // Keep filestream open for brick data

  // Keep track of where in file brick data starts
  dataPos_ = in_.tellg();

  return true;
}

bool BrickReader::ReadBrick(unsigned int _brickIndex, unsigned int _timestep) {
  
  if (brick_ == NULL) {
    ERROR("BrickReader::ReadTimestep() Pointer is NULL");
    return false;
  }

  if (!in_.is_open()) {
    ERROR("Filestream not open");
    return false;
  }

  size_t valSize = sizeof(real);
  unsigned int numVals = xBrickDim_*yBrickDim_*zBrickDim_;
  unsigned int bricksPerTimestep = xNumBricks_*yNumBricks_*zNumBricks_;
  size_t brickSize = valSize*numVals;
  size_t timestepSize = valSize*numVals*bricksPerTimestep;

  std::ios::pos_type offset = dataPos_ + static_cast<std::ios::pos_type>(
    _timestep*timestepSize + _brickIndex*brickSize);
  
  in_.seekg(offset);

  in_.read(reinterpret_cast<char*>(brick_), brickSize);

  return true;
}
