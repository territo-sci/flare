/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#ifndef VOXELDATAHEADER_H_
#define VOXELDATAHEADER_H_

namespace osp {

class VoxelDataHeader {
public:
  static VoxelDataHeader * New();
  ~VoxelDataHeader();

  bool SetDataDimensionality(unsigned int _dataDimensionality);
  bool SetNumTimesteps(unsigned int _numTimesteps);
  bool SetDims(unsigned int _aDim, unsigned int _bDim, unsigned int _cDim);

  unsigned int DataDimensionality() const { return dataDimensionality_; }
  unsigned int NumTimesteps() const { return numTimesteps_; }
  unsigned int ADim() const { return aDim_; }
  unsigned int BDim() const { return bDim_; }
  unsigned int CDim() const { return cDim_; }
  unsigned int NumVoxelsPerTimestep() const { return numVoxelsPerTimestep_; } 

private:
  VoxelDataHeader();
  VoxelDataHeader(const VoxelDataHeader&);
  unsigned int dataDimensionality_;
  unsigned int numTimesteps_;
  unsigned int aDim_;
  unsigned int bDim_;
  unsigned int cDim_;
  unsigned int numVoxelsPerTimestep_;
};

}

#endif
