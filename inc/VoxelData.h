#ifndef VOXELDATA_H
#define VOXELDATA_H

/*
 * Auhtor: Victor Sand (victor.sand@gmail.com)
 * Template class to represent voxel data, its dimensions, 
 * data dimensionality, number of timesteps and so on.
 * Currently only used with float, has not been tested 
 * on other types of data.
 *
 */


#include <vector>

namespace osp {

class VDFReader;

template <class T>
class VoxelData {
public:
  static VoxelData * New() { return new VoxelData<T>; }
  VoxelData() {}
  ~VoxelData() {}
  friend class VDFReader;

  unsigned int NumTimesteps() const { return numTimesteps_; }
  unsigned int DataDimensionality() const { return dataDimensionality_; }
  unsigned int ADim() const { return aDim_; }
  unsigned int BDim() const { return bDim_; }
  unsigned int CDim() const { return cDim_; }
  unsigned int NumVoxelsPerTimestep() const { return aDim_*bDim_*cDim_; }
  unsigned int NumVoxelsTotal() const { 
    return aDim_*bDim_*cDim_*numTimesteps_;
  }
  unsigned int TimestepOffset(unsigned int _timestep) {
    return aDim_*bDim_*cDim_*_timestep;
  }
  T Data(unsigned int _a, unsigned int _b, unsigned int _c) const {
    return data_.at(CoordsToIndex(_a, _b, _c));
  }
  T * DataPtr() { return &data_[0]; }
  T * DataPtr(unsigned int _offset) { return &data_[_offset]; }

private:
  // Convert coordinates to a flattened index.
  // TODO support other orderings
  unsigned int CoordsToIndex(unsigned int _a, unsigned int _b,
                             unsigned int _c) {
    return _a + _b*aDim_ + _c*aDim_*bDim_;
  }
  unsigned int aDim_;
  unsigned int bDim_;
  unsigned int cDim_;
  unsigned int numTimesteps_;
  unsigned int dataDimensionality_;
  std::vector<T> data_;
};

}

#endif
