#ifndef VOXELDATAFRAME_H_
#define VOXELDATAFRAME_H_

#include <vector>

namespace osp {

template <class T>
class VoxelDataFrame {
public:
  static VoxelDataFrame * New() { return new VoxelDataFrame<T>(); }
  VoxelDataFrame() {}
  ~VoxelDataFrame() {}
  void Resize(unsigned int _size) { data_.resize(_size); }
  T* Data() { return &data_[0]; }
private:
  VoxelDataFrame(const VoxelDataFrame&);
  std::vector<T> data_;
};

}

#endif
