#include <VDFReader.h>
#include <fstream>
#include <Utils.h>
#include <VoxelData.h>

#include <VoxelDataHeader.h>
#include <VoxelDataFrame.h>

using namespace osp;

VDFReader::VDFReader() {
}

VDFReader * VDFReader::New() {
  return new VDFReader();
}

VDFReader::~VDFReader() {
  if (fs_.is_open()) {
    fs_.close();
  }
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


bool VDFReader::Init(std::string _filename, 
                     VoxelDataHeader *_header,
                     VoxelDataFrame<float> *_frame) { 
  
  if (initialized_) {
    ERROR("VDFReader already initialized");
    return false;
  }

  frame_ = _frame;
  header_ = _header;
  
  filename_ = _filename;
  fs_.open(filename_.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!fs_.is_open()) {
    ERROR("Init() failed to open " << filename_);
    return false;
  }

  initialized_ = true;
  hasReadHeader_ = false;
}

bool VDFReader::ReadHeader() {
  
  if (!initialized_) {
    ERROR("ReaderHeader(): VDFReader not initialized");
    return false;
  }
  
  if (hasReadHeader_) {
    ERROR("VDFReader has already read header");
    return false;
  }

  unsigned int dataDimensionality = 0;
  unsigned int numTimesteps = 0;
  unsigned int aDim = 0, bDim = 0, cDim = 0;

  fs_.read(reinterpret_cast<char*>(&dataDimensionality),
           sizeof(unsigned int));
  fs_.read(reinterpret_cast<char*>(&numTimesteps),
           sizeof(unsigned int));
  fs_.read(reinterpret_cast<char*>(&aDim),
           sizeof(unsigned int));
  fs_.read(reinterpret_cast<char*>(&bDim),
           sizeof(unsigned int));
  fs_.read(reinterpret_cast<char*>(&cDim),
           sizeof(unsigned int));
  
  header_->SetDataDimensionality(dataDimensionality);
  header_->SetNumTimesteps(numTimesteps);
  
  // Set dimensions (this also sets number of voxels per timestep)
  header_->SetDims(aDim, bDim, cDim);
  timestepSize_ = header_->NumVoxelsPerTimestep()*sizeof(float);

  headerOffset_ = fs_.tellg();
  if (headerOffset_ == -1) {
    ERROR("VDFReader() failed to get header offset");
  }

  hasReadHeader_ = true;
  return true;
}


bool VDFReader::ReadTimestep(unsigned int _timestep) {
  // Get the place to read to
  float *data = frame_->Data();
  std::streampos pos = headerOffset_ + 
                       static_cast<std::streampos>(timestepSize_*_timestep);
  // Read one frame into the chosen location
  fs_.seekg(pos);
  fs_.read(reinterpret_cast<char*>(data), timestepSize_);
  return true;
}
