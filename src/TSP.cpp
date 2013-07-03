/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <TSP.h>
#include <fstream>
#include <Utils.h>
#include <cmath>

using namespace osp;

TSP::TSP(const std::string &_inFilename) : inFilename_(_inFilename) {
}

TSP * TSP::New(const std::string &_inFilename) {
  return new TSP(_inFilename);
}

TSP::~TSP() {
}

bool TSP::Construct() {

  std::fstream in;
  in.open(inFilename_.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    ERROR("TSP Construct failed to open " << inFilename_);
    return false;
  }
  
  // Read header
  unsigned int structure, dataDimensionality, xBrickDim, yBrickDim, zBrickDim,
               xNumBricks, yNumBricks, zNumBricks, numTimesteps,
               paddingWidth, dataSize;
  size_t s = sizeof(unsigned int);
  in.read(reinterpret_cast<char*>(&structure), s);
  in.read(reinterpret_cast<char*>(&dataDimensionality), s);
  in.read(reinterpret_cast<char*>(&xBrickDim), s);
  in.read(reinterpret_cast<char*>(&yBrickDim), s);
  in.read(reinterpret_cast<char*>(&zBrickDim), s);
  in.read(reinterpret_cast<char*>(&xNumBricks), s);
  in.read(reinterpret_cast<char*>(&yNumBricks), s);
  in.read(reinterpret_cast<char*>(&zNumBricks), s);
  in.read(reinterpret_cast<char*>(&numTimesteps), s);
  in.read(reinterpret_cast<char*>(&paddingWidth), s);
  in.read(reinterpret_cast<char*>(&dataSize), s);

  int numLevels = log((int)xNumBricks)/log(2) + 1;
  int numOctreeNodes = (pow(8, numLevels) - 1) / 7;
  int numBSTNodes = (int)numTimesteps * 2 - 1;
  int numTotalNodes = numOctreeNodes * numBSTNodes;

  brickDim_ = (unsigned int)xBrickDim;
  numBricksPerAxis_ = (unsigned int)xNumBricks;
  numTimesteps_ = (unsigned int)numTimesteps;
  numTotalNodes_ = (unsigned int)numTotalNodes;


  INFO("TSP construction, num total nodes: " << numTotalNodes);

  // Allocate space for TSP structure
  data_.resize(numTotalNodes*NUM_DATA);

  // Loop over levels in octree skeleton
  for (int level=0; level<numLevels; ++level) {
    INFO("Visiting level " << level);
    
    // First index of this level
    // 0 for level 0
    // 1 for level 1
    // 9 for level 2
    // etc
    int firstLevelIndex = (pow(8, level) - 1) / 7;

    // Position of first child for this level
    // Equals number of nodes up to this point
    int firstChildOfLevel = (pow(8, level+1) - 1) / 7;
    // Offset between children
    // For level 0, the children at level 1 are 1 octree node apart
    // For level 1, the children at level 2 are 8 octree nodes apart 
    // etc
    int childrenOffset = pow(8, level);

    // For each level, loop over all octree nodes
    int numNodesInLevel = pow(8, level);
    for (int OTNode=0; OTNode<numNodesInLevel; ++OTNode) {
      //INFO("Visiting node " << OTNode << " at level " << level);

      int OTNodeIndex = firstLevelIndex + OTNode;

      if (level == numLevels - 1) { // If leaf
        data_[numBSTNodes*OTNodeIndex*NUM_DATA+CHILD_INDEX] = -1;
      } else {
        int OTChild = firstChildOfLevel+OTNode*childrenOffset;
        data_[numBSTNodes*NUM_DATA*OTNodeIndex+CHILD_INDEX] = 
          numBSTNodes*OTChild;
      }

      // For each octree node, loop over BST nodes
      int BSTChild = 1;
      for (int BSTNode=0; BSTNode<numBSTNodes; ++BSTNode) {
        //INFO("Visiting BST node " << BSTNode);

        int BSTNodeIndex = numBSTNodes*OTNodeIndex + BSTNode;
        if (BSTNode != 0) { // If not root
          if (BSTNode < (numTimesteps-1)) {  // If not leaf
            int BSTChildIndex = numBSTNodes*OTNodeIndex + BSTChild;
            data_[NUM_DATA*BSTNodeIndex+CHILD_INDEX] = BSTChildIndex;
          } else {
            data_[NUM_DATA*BSTNodeIndex+CHILD_INDEX] = -1;
          }
          //INFO("Child " << BSTChildIndex);
        } 

        data_[NUM_DATA*BSTNodeIndex + BRICK_INDEX] = BSTNodeIndex;
        data_[NUM_DATA*BSTNodeIndex + SPATIAL_ERR] = numLevels-1-level;
        // TODO test
        if (BSTNode == (numTimesteps-1)) {
         data_[NUM_DATA*BSTNodeIndex + TEMPORAL_ERR] = 0;
        } else {
         data_[NUM_DATA*BSTNodeIndex + TEMPORAL_ERR] = 1;
        }
        
        BSTChild += 2;
        /* 
        INFO("Visited BSTNodeIndex " << BSTNodeIndex);
        INFO("data[" << NUM_DATA*BSTNodeIndex + BRICK_INDEX << "] = " << BSTNodeIndex);
        INFO("data[" << NUM_DATA*BSTNodeIndex + SPATIAL_ERR << "] = " << level);
        INFO("data[" << NUM_DATA*BSTNodeIndex + TEMPORAL_ERR << "] = " << 1);
        */

      }
          
    }

  }


  for (int i=0; i<data_.size()/NUM_DATA; ++i) {
    INFO(i);
    INFO("Brick index " << data_[NUM_DATA*i + BRICK_INDEX]);
    INFO("Child index " << data_[NUM_DATA*i + CHILD_INDEX]);
    INFO("Spatial err " << data_[NUM_DATA*i + SPATIAL_ERR]);
    INFO("Tempor. err " << data_[NUM_DATA*i + TEMPORAL_ERR]);
  }

  in.close();

  return true;

}



