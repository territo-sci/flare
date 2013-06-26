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

  unsigned int numLevels = log(xNumBricks)/log(2) + 1;
  unsigned int numOctreeNodes = (pow(8, numLevels) - 1) / 7;
  unsigned int numBSTNodes = numTimesteps * 2 - 1;
  unsigned int numTotalNodes = numOctreeNodes * numBSTNodes;

  INFO("TSP construction, num total nodes: " << numTotalNodes);

  // Allocate space for TSP structure
  data_.resize(numTotalNodes*NUM_DATA);

  // Loop over levels in octree skeleton
  for (unsigned int level=0; level<numLevels; ++level) {
    INFO("Visiting level " << level);
    
    // First index of this level
    // 0 for level 0
    // 1 for level 1
    // 9 for level 2
    // etc
    unsigned int firstLevelIndex = (pow(8, level) - 1) / 7;

    // Position of first child for this level
    // Equals number of nodes up to this point
    unsigned int firstChildOfLevel = (pow(8, level+1) - 1) / 7;
    // Offset between children
    // For level 0, the children at level 1 are 1 octree node apart
    // For level 1, the children at level 2 are 8 octree nodes apart 
    // etc
    unsigned int childrenOffset = pow(8, level);

    // For each level, loop over all octree nodes
    unsigned int numNodesInLevel = pow(8, level);
    for (unsigned int OTNode=0; OTNode<numNodesInLevel; ++OTNode) {
      //INFO("Visiting node " << OTNode << " at level " << level);

      // Octree node child index
      unsigned int OTChild;
      if (level == numLevels - 1) {
        OTChild = 0; // Leaf
      } else {
        OTChild = firstChildOfLevel+OTNode*childrenOffset;
      }
      //INFO("Child: " << OTChild);
        
      // At every octree node (root of BST), save index to child
      unsigned int OTNodeIndex = firstLevelIndex + OTNode;
      unsigned int OTChildIndex = numBSTNodes*OTChild*NUM_DATA;
      data_[numBSTNodes*NUM_DATA*OTNodeIndex+CHILD_INDEX] = OTChildIndex;

      // For each octree node, loop over BST nodes
      for (unsigned int BSTNode=0; BSTNode<numBSTNodes; ++BSTNode) {
        //INFO("Visiting BST node " << BSTNode);
         
        unsigned int BSTNodeIndex = numBSTNodes*OTNodeIndex + BSTNode;
        if (BSTNode != 0) { // If not root
          unsigned int BSTChildIndex = 0;
          if (BSTNode < (numTimesteps) - 1) {  // If not leaf
            BSTChildIndex = numBSTNodes*OTNodeIndex + BSTNode*2 - 1;
          }
          data_[NUM_DATA*BSTNodeIndex + CHILD_INDEX] = BSTChildIndex*NUM_DATA;
          //INFO("Child " << BSTChildIndex);
        } 

        data_[NUM_DATA*BSTNodeIndex + BRICK_INDEX] = BSTNodeIndex;
        data_[NUM_DATA*BSTNodeIndex + SPATIAL_ERR] = level;
        data_[NUM_DATA*BSTNodeIndex + TEMPORAL_ERR] = 1;
        
        /* 
        INFO("Visited BSTNodeIndex " << BSTNodeIndex);
        INFO("data[" << NUM_DATA*BSTNodeIndex + BRICK_INDEX << "] = " << BSTNodeIndex);
        INFO("data[" << NUM_DATA*BSTNodeIndex + SPATIAL_ERR << "] = " << level);
        INFO("data[" << NUM_DATA*BSTNodeIndex + TEMPORAL_ERR << "] = " << 1);
        */

      }
          
    }

  }


  in.close();

  return true;

}



