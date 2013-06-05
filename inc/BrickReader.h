/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#ifndef BRICKREADER_H_
#define BRICKREADER_H_

#define real float

#include <string>
#include <fstream>

namespace osp {

class BrickReader {
public:
  static BrickReader * New();
  ~BrickReader();

  void SetInFilename(std::string _inFilename);
  
  bool ReadHeader();

  // Read one brick at a certain timestep, but data in brick ptr 
  bool ReadBrick(unsigned int _brickIndex, unsigned int _timestep);

  unsigned int Structure() const { return structure_; }
  unsigned int DataDimensionality() const { return dataDimensionality_; }
  unsigned int XBrickDim() const { return xBrickDim_; }
  unsigned int YBrickDim() const { return yBrickDim_; }
  unsigned int ZBrickDim() const { return zBrickDim_; }
  unsigned int XNumBricks() const { return xNumBricks_; }
  unsigned int YNumBricks() const { return yNumBricks_; }
  unsigned int ZNumBricks() const { return zNumBricks_; }
  unsigned int NumTimesteps() const { return numTimesteps_; }
  unsigned int PaddingWidth() const { return paddingWidth_; }
  unsigned int DataSize() const { return dataSize_; }

  real * BrickPtr() { return brick_; }

private:
  BrickReader();
  BrickReader(const BrickReader&);
  std::string inFilename_;

  // Header data to read (keep ordered properly)
  unsigned int structure_;
  unsigned int dataDimensionality_;
  unsigned int xBrickDim_;
  unsigned int yBrickDim_;
  unsigned int zBrickDim_;
  unsigned int xNumBricks_;
  unsigned int yNumBricks_;
  unsigned int zNumBricks_;
  unsigned int numTimesteps_;
  unsigned int paddingWidth_;
  unsigned int dataSize_;

  // Location in which to store one brick at a time
  real *brick_;

  // The filestream to read from
  // The stream is opened in the ReadHeader function and closed in destructor
  std::ifstream in_;

  // Points to the first brick data (just after header)
  // This is set in ReadHeader function
  std::ios::pos_type dataPos_;

};

}

#endif
