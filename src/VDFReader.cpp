#include <VDFReader.h>
#include <fstream>
#include <Utils.h>
#include <VoxelData.h>

using namespace osp;

VDFReader::VDFReader() {
}

VDFReader * VDFReader::New() {
  return new VDFReader();
}

VDFReader::~VDFReader() {
}

void VDFReader::SetVoxelData(VoxelData<float> *_voxelData) {
  voxelData_ = _voxelData;
}

bool VDFReader::Read(std::string _filename) {
  std::fstream in;
  in.open(_filename.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    ERROR("Could not open " << _filename);
    return false;
  }

  in.read(reinterpret_cast<char*>(&(voxelData_->dataDimensionality_)),
          sizeof(unsigned int));
  in.read(reinterpret_cast<char*>(&(voxelData_->numTimesteps_)),
          sizeof(unsigned int));
  in.read(reinterpret_cast<char*>(&(voxelData_->aDim_)),
          sizeof(unsigned int));
  in.read(reinterpret_cast<char*>(&(voxelData_->bDim_)),
          sizeof(unsigned int));
  in.read(reinterpret_cast<char*>(&(voxelData_->cDim_)),
          sizeof(unsigned int));
  unsigned int size = voxelData_->NumVoxelsTotal();
  voxelData_->data_.resize(size);
  in.read(reinterpret_cast<char*>(&(voxelData_->data_[0])),
          size*sizeof(float));

  INFO("Read data dimensionality: " << voxelData_->dataDimensionality_);
  INFO("Read data num timesteps: " << voxelData_->numTimesteps_);
  INFO("Read x dimension: " << voxelData_->aDim_);
  INFO("Read y dimension: " << voxelData_->bDim_);
  INFO("Read z dimension: " << voxelData_->cDim_);

  in.close();

  return true;
}

