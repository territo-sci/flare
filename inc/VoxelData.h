#ifndef VOXELDATA_H
#define VOXELDATA_H

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
	unsigned int XDim() const { return xDim_; }
	unsigned int YDim() const { return yDim_; }
	unsigned int ZDim() const { return zDim_; }
	unsigned int Size() const { return xDim_*yDim_*zDim_; }
	T Data(unsigned int _x, unsigned int _y, unsigned int _z) const {
		return data_.at(CoordsToIndex(_x, _y, _z));
	}
	T * DataPtr() { return &data_[0]; }
private:
	unsigned int CoordsToIndex(unsigned int _x,
														 unsigned int _y,
														 unsigned int _z) {
		return _x + _y*zDim_ + _z*yDim_*zDim_;
	}
	unsigned int xDim_;
	unsigned int yDim_;
	unsigned int zDim_;
	unsigned int numTimesteps_;
	unsigned int dataDimensionality_;
	std::vector<T> data_;
};

}

#endif
