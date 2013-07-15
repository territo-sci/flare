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
#include <string>

namespace osp {

class TSP {
public:

  enum NodeData {
    BRICK_INDEX = 0,
    CHILD_INDEX,
    SPATIAL_ERR,
    TEMPORAL_ERR,
    NUM_DATA
  };

  static TSP * New(const std::string &_inFilename);
  ~TSP();
  
  bool Construct();

  int * Data() { return &data_[0]; }
  unsigned int Size() { return data_.size(); }

  unsigned int BrickDim() const { return brickDim_; }
  unsigned int PaddedBrickDim() const { return paddedBrickDim_; }
  unsigned int NumBricksPerAxis() const { return numBricksPerAxis_; }
  unsigned int NumTimesteps() const { return numTimesteps_; }
  unsigned int NumTotalNodes() const { return numTotalNodes_; }
  unsigned int NumValuesPerNode() const { return NUM_DATA; }
  unsigned int NumBSTNodesPerOT() const { return numBSTNodesPerOT_; }
  unsigned int NumOTLevels() const { return numOTLevels_; }

private:
  TSP();
  TSP(const std::string &_inFilename);
  TSP(const TSP&);
  std::string inFilename_;
  // Holds the actual structure
  std::vector<int> data_;

  unsigned int brickDim_;
  unsigned int paddedBrickDim_;
  unsigned int numBricksPerAxis_;
  unsigned int numTimesteps_;
  unsigned int numTotalNodes_;
  unsigned int numBSTNodesPerOT_;
  unsigned int numOTLevels_;

};

}

#endif

