/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <VoxelDataHeader.h>

using namespace osp;

VoxelDataHeader * VoxelDataHeader::New() {
  return new VoxelDataHeader();
}

VoxelDataHeader::VoxelDataHeader() 
  : dataDimensionality_(0), numTimesteps_(0),
    aDim_(0), bDim_(0), cDim_(0), numVoxelsPerTimestep_(0) {}

VoxelDataHeader::~VoxelDataHeader() {}

bool VoxelDataHeader::SetDataDimensionality(unsigned int _dataDimensionality) {
  dataDimensionality_ = _dataDimensionality;
  return true;
}

bool VoxelDataHeader::SetNumTimesteps(unsigned int _numTimesteps) {
  numTimesteps_ = _numTimesteps;
  return true;
}

bool VoxelDataHeader::SetDims(unsigned int _aDim,
                              unsigned int _bDim,
                              unsigned int _cDim) {
  aDim_ = _aDim;
  bDim_ = _bDim;
  cDim_ = _cDim;
  numVoxelsPerTimestep_ = aDim_*bDim_*cDim_;
  return true;
}


