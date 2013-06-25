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

class BrickManager {
public:
  static BrickManager * New();
  ~BrickManager();

  // Coordinates in texture atlas plus relative size of brick
  struct AtlasCoords {
    int x_;
    int y_;
    int z_;
    int size_;
  };

  enum BUFFER_INDEX { EVEN = 0, ODD };

  void SetInFilename(const std::string &_inFilename);
  // Read header data from file, should normally only be called once
  // unless header data changes
  bool ReadHeader();

  bool InitAtlas();

  // Update the brick list. This does not apply the changes.
  bool UpdateBrickList(unsigned int _brickIndex, AtlasCoords _atlasCoords);
  // See if a certain brick is present in the atlas. If it is, put its
  // coordnates in the referenced argument.
  bool InAtlas(unsigned int _brickIndex, AtlasCoords &_atlasCoords);
  // Apply changed upload bricks in brick list to texture
  bool UpdateAtlas();

  // The box list maps one brick index to each box in the volume
  bool UpdateBoxList(unsigned int _boxIndex, unsigned int _brickIndex);
  
  std::vector<int> BoxList() { return boxList_; }
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
  BrickManager(const BrickManager&);

  // Read a single brick into the brick buffer
  bool ReadBrick(unsigned int _brickIndex, BUFFER_INDEX _bufferIndex);

  std::string inFilename_;

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

  // Texture where the actual atlas is kept
  Texture3D *textureAtlas_;

  // Keeps track of which bricks are in the texture atlas, and at what coords
  std::map<unsigned int, AtlasCoords> brickList_;
  // Keeps track of which bricks belongs to which boxes
  std::vector<int> boxList_;

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
