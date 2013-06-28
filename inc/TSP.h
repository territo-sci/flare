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
private:
  TSP();
  TSP(const std::string &_inFilename);
  TSP(const TSP&);
  std::string inFilename_;
  // Holds the actual structure
  std::vector<int> data_;
};

}

#endif

