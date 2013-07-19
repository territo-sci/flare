/* 
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#ifndef BRICKMANAGER_H
#define BRICKMANAGER_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>

#define real float

namespace osp {

class Texture3D;
class Config;

class BrickManager {
public:
  static BrickManager * New(Config *_config);
  ~BrickManager();

  Config *config_;

  enum BUFFER_INDEX { EVEN = 0, ODD };

  // Read header data from file, should normally only be called once
  // unless header data changes
  bool ReadHeader();

  bool InitAtlas();

  bool BuildBrickList(std::vector<int> _brickRequest);
  bool UpdateAtlas();

  std::vector<int> BrickList() { return brickList_; }
  Texture3D * TextureAtlas() { return textureAtlas_; }

  // Header accessors
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

private:

  BrickManager();
  BrickManager(Config *_config);
  BrickManager(const BrickManager&);

  // Read a single brick into the brick buffer
  bool ReadBrick(unsigned int _brickIndex, BUFFER_INDEX _bufferIndex);

  // Header data
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

  unsigned int numBricks_;
  unsigned int brickDim_;
  unsigned int paddedBrickDim_;
  unsigned int atlasDim_;

  // Texture where the actual atlas is kept
  Texture3D *textureAtlas_;

  std::vector<int> brickList_;

  // Filestream to read from
  // Opened in the ReadHeader() function and closed in destructor
  std::ifstream in_;

  // Points to the first brick data (just after the header)
  // This is set when the ReadHeader() function is run
  std::ios::pos_type dataPos_;
  
  // Buffer for reading brick data
  // Allocated in ReadHeader(), deleted in destructor
  real *brickBuffer_[2];

  bool hasReadHeader_;
  bool atlasInitialized_;

  // PBO
  unsigned int pboHandle_[2];

};

}

#endif
