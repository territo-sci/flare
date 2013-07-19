/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <TSP.h>
#include <Config.h>
#include <fstream>
#include <Utils.h>
#include <cmath>

using namespace osp;

TSP::TSP(Config *_config) : config_(_config) {
}

TSP * TSP::New(Config *_config) {
  return new TSP(_config);
}

TSP::~TSP() {
}

bool TSP::Construct() {

  INFO("\nConstructing TSP tree, spatial ordering");

  std::fstream in;
  std::string inFilename = config_->TSPFilename();
  in.open(inFilename.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    ERROR("TSP Construct failed to open " << inFilename);
    return false;
  }
  
  // Read header
  size_t s = sizeof(unsigned int);
  in.read(reinterpret_cast<char*>(&structure_), s);
  in.read(reinterpret_cast<char*>(&dataDimensionality_), s);
  // TODO remove x y z dims
  in.read(reinterpret_cast<char*>(&brickDim_), s);
  in.read(reinterpret_cast<char*>(&brickDim_), s);
  in.read(reinterpret_cast<char*>(&brickDim_), s);
  in.read(reinterpret_cast<char*>(&numBricksPerAxis_), s);
  in.read(reinterpret_cast<char*>(&numBricksPerAxis_), s);
  in.read(reinterpret_cast<char*>(&numBricksPerAxis_), s);
  in.read(reinterpret_cast<char*>(&numTimesteps_), s);
  in.read(reinterpret_cast<char*>(&paddingWidth_), s);
  in.read(reinterpret_cast<char*>(&dataSize_), s);
  in.close();

  INFO("Brick dimensions: " << brickDim_);
  INFO("Num bricks per axis: " << numBricksPerAxis_);
  INFO("Num timesteps: " << numTimesteps_);

  if (paddingWidth_ > 1) {
    ERROR("Padding width > 1 unsupported");
    return false;
  }

  paddedBrickDim_ = brickDim_ + 2*paddingWidth_;

  numOTLevels_ = log((int)numBricksPerAxis_)/log(2) + 1;
  numOTNodes_ = (pow(8, numOTLevels_) - 1) / 7;
  numBSTLevels_ = log((int)numTimesteps_)/log(2) + 1;
  numBSTNodes_ = (int)numTimesteps_*2 - 1;
  numTotalNodes_ = numOTNodes_ * numBSTNodes_;

  INFO("Num OT levels: " << numOTLevels_);
  INFO("Num OT nodes: " << numOTNodes_);
  INFO("Num BST levels: " << numBSTLevels_);
  INFO("Num BST nodes: " << numBSTNodes_);

  // Allocate space for TSP structure
  data_.resize(numTotalNodes_*NUM_DATA);

  // Loop over the OTs (one per BST node)
  for (unsigned int OT=0; OT<numBSTNodes_; ++OT) {
 
    // Start at the root of each OT  
    unsigned int OTNode = OT*numOTNodes_;

    // Calculate BST level (first level is level 0)
    unsigned int BSTLevel = log(OT+1)/log(2);

    // Traverse OT
    unsigned int OTChild = 1;
    unsigned int OTLevel = 0;
    while (OTLevel < numOTLevels_) {

      unsigned int OTNodesInLevel = pow(8, OTLevel);
      for (unsigned int i=0; i<OTNodesInLevel; ++i) {      

        // Brick index
        data_[OTNode*NUM_DATA + BRICK_INDEX] = (int)OTNode;

        // Error metrics
        int localOTNode = (OTNode - OT*numOTNodes_);
        data_[OTNode*NUM_DATA + TEMPORAL_ERR] = (int)(numBSTLevels_-1-BSTLevel);
        data_[OTNode*NUM_DATA + SPATIAL_ERR] = (int)(numOTLevels_-1-OTLevel);
    
        if (BSTLevel == 0) {
          // Calculate OT child index (-1 if node is leaf)
          int OTChildIndex = 
            (OTChild < numOTNodes_) ? (int)(OT*numOTNodes_+OTChild) : -1;
            data_[OTNode*NUM_DATA + CHILD_INDEX] = OTChildIndex;
        } else {
          // Calculate BST child index (-1 if node is BST leaf)

          // First BST node of current level
          int firstNode = (2*pow(2, BSTLevel-1)-1)*numOTNodes_;
          // First BST node of next level
          int firstChild = (2*pow(2, BSTLevel)-1)*numOTNodes_;
          // Difference between first nodes between levels
          int levelGap = firstChild-firstNode;
          // How many nodes away from the first node are we?
          int offset = (OTNode-firstNode) / numOTNodes_;

          // Use level gap and offset to calculate child index
          int BSTChildIndex =
            (BSTLevel < numBSTLevels_-1) ? 
              (int)(OTNode+levelGap+(offset*numOTNodes_)) : -1;

          data_[OTNode*NUM_DATA + CHILD_INDEX] = BSTChildIndex;
        }

        OTNode++;
        OTChild += 8;

      }

      OTLevel++;
    }
  }

 
  /*
  for (unsigned int i=0; i<data_.size()/4; ++i) {
    INFO("------------------------");
    INFO("Brick index: " << data_[i*NUM_DATA + BRICK_INDEX]);
    INFO("Child index: " << data_[i*NUM_DATA + CHILD_INDEX]);
    INFO("Spat. error: " << data_[i*NUM_DATA + SPATIAL_ERR]);
    INFO("Temp. error: " << data_[i*NUM_DATA + TEMPORAL_ERR]);
  }
  */

  INFO("");
  return true;

}

/*

bool TSP::Construct() {

  std::fstream in;
  in.open(inFilename_.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    ERROR("TSP Construct failed to open " << inFilename_);
    return false;
  }
  
  // Read header
  size_t s = sizeof(unsigned int);
  in.read(reinterpret_cast<char*>(&structure_), s);
  in.read(reinterpret_cast<char*>(&dataDimensionality_), s);
  in.read(reinterpret_cast<char*>(&brickDim_), s);
  in.read(reinterpret_cast<char*>(&brickDim_), s);
  in.read(reinterpret_cast<char*>(&brickDim_), s);
  in.read(reinterpret_cast<char*>(&numBricksPerAxis_), s);
  in.read(reinterpret_cast<char*>(&numBricksPerAxis_), s);
  in.read(reinterpret_cast<char*>(&numBricksPerAxis_), s);
  in.read(reinterpret_cast<char*>(&numTimesteps_), s);
  in.read(reinterpret_cast<char*>(&paddingWidth_), s);
  in.read(reinterpret_cast<char*>(&dataSize_), s);

  numOTLevels_ = log(numBricksPerAxis_)/log(2) + 1;
  numOTNodes_ = (pow(8, numOTLevels_) - 1) / 7;
  numBSTNodes_ = numTimesteps_ * 2 - 1;
  numTotalNodes_ = numOTNodes_ * numBSTNodes_;

  INFO("numOTLevels_: " << numBSTLevels_);
  INFO("numOctreeNodes_: " << numOTNodes_);
  INFO("numTotalNodes_: " << numTotalNodes_);

  paddedBrickDim_ = brickDim_ + 2*paddingWidth_;

  //INFO("TSP construction, num total nodes: " << numTotalNodes_);

  // Allocate space for TSP structure
  data_.resize(numTotalNodes_*NUM_DATA);

  // Loop over levels in octree skeleton
  for (unsigned int level=0; level<numOTLevels_; ++level) {
    //INFO("Visiting level " << level);
    

    INFO("level " << level);
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
    unsigned int childrenOffset = 8; 

    // For each level, loop over all octree nodes
    unsigned int numNodesInLevel = pow(8, level);
    for (int OTNode=0; OTNode<numNodesInLevel; ++OTNode) {
      //INFO("Visiting node " << OTNode << " at level " << level);

      unsigned int OTNodeIndex = firstLevelIndex + OTNode;

      unsigned int OTChild;
      if (level == numOTLevels_ - 1) { // If leaf
        data_[numBSTNodes_*OTNodeIndex*NUM_DATA+CHILD_INDEX] = -1;
      } else {
        OTChild = firstChildOfLevel+OTNode*childrenOffset;
        data_[numBSTNodes_*NUM_DATA*OTNodeIndex+CHILD_INDEX] = 
          numBSTNodes_*OTChild;
      }

      // For each octree node, loop over BST nodes
      int BSTChild = 1;
      for (unsigned int BSTNode=0; BSTNode<numBSTNodes_; ++BSTNode) {
        //INFO("Visiting BST node " << BSTNode);
        

        unsigned int BSTNodeIndex = numBSTNodes_*OTNodeIndex + BSTNode;
        if (BSTNode != 0) { // If not root
          if (BSTNode < (numTimesteps_-1)) {  // If not leaf
            int BSTChildIndex = numBSTNodes_*OTNodeIndex + BSTChild;
            data_[NUM_DATA*BSTNodeIndex+CHILD_INDEX] = BSTChildIndex;
          } else {
            data_[NUM_DATA*BSTNodeIndex+CHILD_INDEX] = -1;
          }
        } 

        data_[NUM_DATA*BSTNodeIndex + BRICK_INDEX] = BSTNodeIndex;
        data_[NUM_DATA*BSTNodeIndex + SPATIAL_ERR] = numOTLevels_-1-level;
        
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
      }

          
    }

  }



  in.close();

  return true;

}

*/

