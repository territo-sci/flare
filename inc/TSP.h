/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 * Construct a traversable TSP structure with structure and error info
 * from a .tsp file
 *
 */

#ifndef TSP_H_
#define TSP_H_

#include <vector>
#include <list>
#include <string>
#include <iostream>

namespace osp {

class Config;

class TSP {
public:

  enum NodeData {
    BRICK_INDEX = 0,
    CHILD_INDEX,
    SPATIAL_ERR,
    TEMPORAL_ERR,
    NUM_DATA
  };

  static TSP * New(Config * _config);
  ~TSP();
  
  bool Construct();
  bool CalculateSpatialError();
  //bool CalculateTemporalError();

  int * Data() { return &data_[0]; }
  unsigned int Size() { return data_.size(); }

  unsigned int BrickDim() const { return brickDim_; }
  unsigned int PaddedBrickDim() const { return paddedBrickDim_; }
  unsigned int NumBricksPerAxis() const { return numBricksPerAxis_; }
  unsigned int NumTimesteps() const { return numTimesteps_; }
  unsigned int NumTotalNodes() const { return numTotalNodes_; }
  unsigned int NumValuesPerNode() const { return NUM_DATA; }
  unsigned int NumBSTNodes() const { return numBSTNodes_; }
  unsigned int NumOTNodes() const { return numOTNodes_; }
  unsigned int NumOTLevels() const { return numOTLevels_; }

private:
  TSP();
  TSP(Config *_config);
  TSP(const TSP&);
  
  Config *config_;

  // Holds the actual structure
  std::vector<int> data_;

  unsigned int structure_;
  unsigned int dataDimensionality_;
  unsigned int brickDim_;
  unsigned int paddedBrickDim_;
  unsigned int numBricksPerAxis_;
  unsigned int numTimesteps_;
  unsigned int paddingWidth_;
  unsigned int dataSize_;
  unsigned int numTotalNodes_;
  unsigned int numBSTLevels_;
  unsigned int numBSTNodes_;
  unsigned int numOTLevels_;
  unsigned int numOTNodes_;

  // Return a vector with the indices of the bricks that a given
  // input brick covers. If given a leaf brick, return leaf index back.
  std::list<unsigned int> CoveredLeafBricks(unsigned int _brickIndex);

  // Return a list of eight children brick incices given a brick index
  std::list<unsigned int> ChildBricks(unsigned int _brickIndex);

  std::ios::pos_type dataPos_;

};

}

#endif

