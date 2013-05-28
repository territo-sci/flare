#ifndef VDFREADER_H
#define VDFREADER_H

/*
 * Author: Victor Sand (victor.sand@gmail.com)
 * Reads a .vdf file and populate VoxelData.
 *
 */

#include <string>
#include <fstream>

namespace osp {

class VoxelDataHeader;

template <class T>
class VoxelDataFrame;

template <class T>
class VoxelData;

class VDFReader {
public:
  static VDFReader * New();
  ~VDFReader();
  void SetVoxelData(VoxelData<float> *_voxelData);
  bool Read(std::string _filename);

  // TODO work in progress
  bool Init(std::string _filename,
            VoxelDataHeader *_header,
            VoxelDataFrame<float> *_frame);
  bool ReadHeader();
  bool ReadTimestep(unsigned int _timestep);


private:
  VDFReader();
  VoxelData<float> *voxelData_;

  // TODO work in progress
  std::fstream fs_;
  VoxelDataHeader *header_;
  VoxelDataFrame<float> *frame_;
  std::string filename_;
  bool initialized_;
  bool hasReadHeader_;

  // Position of first byte of voxel data
  std::streampos headerOffset_;
  // Size in bytes of one frame
  unsigned int timestepSize_;
};

}

#endif
