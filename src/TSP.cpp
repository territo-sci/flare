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

  INFO("numLevels: " << numLevels);
  INFO("numOctreeNodes: " << numOctreeNodes);
  INFO("numTotalNodes: " << numTotalNodes);

  brickDim_ = (unsigned int)xBrickDim;
  paddedBrickDim_ = brickDim_ + 2*paddingWidth;
  numBricksPerAxis_ = (unsigned int)xNumBricks;
  numTimesteps_ = (unsigned int)numTimesteps;
  numTotalNodes_ = (unsigned int)numTotalNodes;
  numOTLevels_ = (unsigned int)numLevels;
  numBSTNodesPerOT_ = (unsigned int)numBSTNodes;


  //INFO("TSP construction, num total nodes: " << numTotalNodes);

  // Allocate space for TSP structure
  data_.resize(numTotalNodes*NUM_DATA);

  // Loop over levels in octree skeleton
  for (int level=0; level<numLevels; ++level) {
    //INFO("Visiting level " << level);
    

    INFO("level " << level);
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
    int childrenOffset = 8; 

    // For each level, loop over all octree nodes
    int numNodesInLevel = pow(8, level);
    for (int OTNode=0; OTNode<numNodesInLevel; ++OTNode) {
      //INFO("Visiting node " << OTNode << " at level " << level);

      int OTNodeIndex = firstLevelIndex + OTNode;

      int OTChild;
      if (level == numLevels - 1) { // If leaf
        data_[numBSTNodes*OTNodeIndex*NUM_DATA+CHILD_INDEX] = -1;
      } else {
        OTChild = firstChildOfLevel+OTNode*childrenOffset;
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
        } 

        data_[NUM_DATA*BSTNodeIndex + BRICK_INDEX] = BSTNodeIndex;
        data_[NUM_DATA*BSTNodeIndex + SPATIAL_ERR] = numLevels-1-level;
        // TODO test
        int tempErr;
        if (BSTNode == 0) {
          tempErr = 5;
        } else if (BSTNode < 3) {
          tempErr = 4;
        } else if (BSTNode < 7) {
          tempErr = 3;
        } else if (BSTNode < 15) {
          tempErr = 2;
        } else if (BSTNode < 31) {
          tempErr = 1;
        } else {
          tempErr = 0;
        }
        data_[NUM_DATA*BSTNodeIndex + TEMPORAL_ERR] = tempErr;
        
        BSTChild += 2;
        /*
        INFO("Visited BSTNodeIndex " << BSTNodeIndex);
        INFO("data[" << NUM_DATA*BSTNodeIndex + BRICK_INDEX << "] = " << BSTNodeIndex);
        INFO("data[" << NUM_DATA*BSTNodeIndex + CHILD_INDEX << "] = " << data_[NUM_DATA*BSTNodeIndex+CHILD_INDEX]);
        INFO("data[" << NUM_DATA*BSTNodeIndex + SPATIAL_ERR << "] = " << data_[NUM_DATA*BSTNodeIndex+SPATIAL_ERR]);
        INFO("data[" << NUM_DATA*BSTNodeIndex + TEMPORAL_ERR << "] = " << data_[NUM_DATA*BSTNodeIndex+TEMPORAL_ERR]);
       */
      }

          
    }

  }



  in.close();

  return true;

}



