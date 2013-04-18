#ifndef VDFREADER_H
#define VDFREADER_H

#include <string>

namespace osp {

template <class T>
class VoxelData;

class VDFReader {
public:
	static VDFReader * New();
	~VDFReader();
void SetVoxelData(VoxelData<float> *_voxelData);
	bool Read(std::string _filename);
private:
VDFReader();
	VoxelData<float> *voxelData_;
};

}

#endif
