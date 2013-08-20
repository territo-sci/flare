/*
 * Author: Victor Sand (victor.sand@gmail.com)
 *
 */

#include <TSP.h>
#include <Config.h>
#include <fstream>
#include <Utils.h>
#include <cmath>
#include <list>
#include <queue>
#include <TransferFunction.h>
#include <algorithm>

using namespace osp;

TSP::TSP(Config *_config) : config_(_config) {
}

TSP * TSP::New(Config *_config) {
  return new TSP(_config);
}

TSP::~TSP() {
}

bool TSP::ReadHeader() {

  std::fstream in;
  std::string inFilename = config_->TSPFilename();
  in.open(inFilename.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    ERROR("TSP Construct failed to open " << inFilename);
    return false;
  }
  
  // Read header
  size_t s = sizeof(unsigned int);
  in.read(reinterpret_cast<char*>(&gridType_), s);
  in.read(reinterpret_cast<char*>(&numTimesteps_), s);
  in.read(reinterpret_cast<char*>(&xBrickDim_), s);
  in.read(reinterpret_cast<char*>(&yBrickDim_), s);
  in.read(reinterpret_cast<char*>(&zBrickDim_), s);
  in.read(reinterpret_cast<char*>(&xNumBricks_), s);
  in.read(reinterpret_cast<char*>(&yNumBricks_), s);
  in.read(reinterpret_cast<char*>(&zNumBricks_), s);
  in.read(reinterpret_cast<char*>(&dataSize_), s);
  dataPos_ = in.tellg();

  in.close();

  paddedBrickDim_ = xBrickDim_ + 2*paddingWidth_;

  // TODO support dimensions of different size
  numOTLevels_ = (unsigned int)(log((int)xNumBricks_)/log(2) + 1);
  numOTNodes_ = (unsigned int)((pow(8, numOTLevels_) - 1) / 7);
  numBSTLevels_ = (unsigned int)(log((int)numTimesteps_)/log(2) + 1);
  numBSTNodes_ = (unsigned int)numTimesteps_*2 - 1;
  numTotalNodes_ = numOTNodes_ * numBSTNodes_;

  INFO("Num OT levels: " << numOTLevels_);
  INFO("Num OT nodes: " << numOTNodes_);
  INFO("Num BST levels: " << numBSTLevels_);
  INFO("Num BST nodes: " << numBSTNodes_);

  // Allocate space for TSP structure
  data_.resize(numTotalNodes_*NUM_DATA);

  return true;

}

bool TSP::Construct() {

  INFO("\nConstructing TSP tree, spatial ordering");

  // Loop over the OTs (one per BST node)
  for (unsigned int OT=0; OT<numBSTNodes_; ++OT) {
 
    // Start at the root of each OT  
    unsigned int OTNode = OT*numOTNodes_;

    // Calculate BST level (first level is level 0)
    unsigned int BSTLevel = (unsigned int)(log(OT+1)/log(2));

    // Traverse OT
    unsigned int OTChild = 1;
    unsigned int OTLevel = 0;
    while (OTLevel < numOTLevels_) {

      unsigned int OTNodesInLevel = static_cast<unsigned int>(pow(8, OTLevel));
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
          int firstNode = 
            static_cast<unsigned int>((2*pow(2, BSTLevel-1)-1)*numOTNodes_);
          // First BST node of next level
          int firstChild = 
            static_cast<unsigned int>((2*pow(2, BSTLevel)-1)*numOTNodes_);
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

  INFO("");
  return true;

}

bool TSP::CalculateSpatialError() {

  unsigned int numBrickVals = paddedBrickDim_*paddedBrickDim_*paddedBrickDim_;

  std::fstream in;
  std::string inFilename = config_->TSPFilename();
  in.open(inFilename.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    ERROR("TSP CalculateSpatialError failed to open " << inFilename);
    return false;
  }

  std::vector<float> buffer(numBrickVals);
  std::vector<float> averages(numBrickVals);
  std::vector<float> stdDevs(numTotalNodes_);

  // First pass: Calculate average color for each brick
  INFO("\nCalculating spatial error, first pass");
  for (unsigned int brick=0; brick<numTotalNodes_; ++brick) {

    // Offset in file
    std::ios::pos_type offset = dataPos_ +
      static_cast<std::ios::pos_type>(brick*numBrickVals*sizeof(float));

    in.seekp(offset);
    in.read(reinterpret_cast<char*>(&buffer[0]), sizeof(float)*numBrickVals);

    // TODO parallelize
    float average = 0.f;
    for (auto it=buffer.begin(); it!=buffer.end(); ++it) {
      average += *it;
    }
    
    averages[brick] = average/static_cast<float>(numBrickVals);
  }
    
  // Spatial SNR stats
  float minError = 1e20f;
  float maxError = 0.f;
  std::vector<float> medianArray(numTotalNodes_);

  // Second pass: For each brick, compare the covered leaf voxels with
  // the brick average
  INFO("Calculating spatial error, second pass");
  for (unsigned int brick=0; brick<numTotalNodes_; ++brick) {
  
    // Fetch mean color and compute scalar analouge
    float brickAvg = averages[brick];

    // Sum  for std dev computation
    float stdDev = 0.f;

    // Get a list of leaf bricks that the current brick covers
    std::list<unsigned int> coveredLeafBricks =
      CoveredLeafBricks(brick);

    // Calculate "standard deviation" corresponding to leaves
    for (auto lb=coveredLeafBricks.begin(); 
         lb!=coveredLeafBricks.end(); ++lb) {

      // Read brick
      std::ios::pos_type offset = dataPos_ +
        static_cast<std::ios::pos_type>((*lb)*numBrickVals*sizeof(float));
      in.seekp(offset);
      in.read(reinterpret_cast<char*>(&buffer[0]), sizeof(float)*numBrickVals);

      // Add to sum
      for (auto v=buffer.begin(); v!=buffer.end(); ++v) {
        stdDev += pow(*v-brickAvg, 2.f);
      }

    }

    // Finish calculation
    if (sizeof(float) != sizeof(int)) {
      ERROR("Float and int sizes don't match, can't reintepret");
      return false;
    }

    stdDev /= static_cast<float>(coveredLeafBricks.size()*numBrickVals);
    stdDev = sqrt(stdDev);
    //stdDev = pow(stdDev, 0.1f);
    //INFO(brick << " " << " std dev: " << stdDev);

    if (stdDev < minError) {
      minError = stdDev;
    } else if (stdDev > maxError) {
      maxError = stdDev;
    }

    //data_[brick*NUM_DATA+SPATIAL_ERR] = *reinterpret_cast<int*>(&stdDev);
    stdDevs[brick] = stdDev;
    medianArray[brick] = stdDev;
    
  }

  
  std::sort(medianArray.begin(), medianArray.end());
  float medError = medianArray[medianArray.size()/2];

  INFO("Min spatial std dev: " << minError);
  INFO("Max spatial std dev: " << maxError);
  INFO("Median spatial std dev: " << medError);
  INFO("");

  // Normalize errors
  float minNorm = 1e20f;
  float maxNorm = 0.f;
  for (unsigned int i=0; i<numTotalNodes_; ++i) {
    //float normalized = (stdDevs[i]-minError)/(maxError-minError);
    stdDevs[i] = pow(stdDevs[i], 0.5f);
    data_[i*NUM_DATA+SPATIAL_ERR] = *reinterpret_cast<int*>(&stdDevs[i]);
    if (stdDevs[i] < minNorm) {
      minNorm = stdDevs[i];
    } else if (stdDevs[i] > maxNorm) {
      maxNorm = stdDevs[i];
    }
  }
  
  std::sort(stdDevs.begin(), stdDevs.end());
  float medNorm = stdDevs[stdDevs.size()/2];
  
  minSpatialError_ = minNorm;
  maxSpatialError_ = maxNorm;
  medianSpatialError_ = medNorm;

  INFO("Min normalized spatial std dev: " << minNorm);
  INFO("Max normalized spatial std dev: " << maxNorm);
  INFO("Median normalized spatial std dev: " << medNorm);
  INFO("");

  return true;
}  


bool TSP::CalculateTemporalError() {

  std::fstream in;
  std::string inFilename = config_->TSPFilename();
  in.open(inFilename.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    ERROR("TSP CalculateTemporalError failed to open " << inFilename);
    return false;
  }

  INFO("Calculating temporal error");

  // Statistics
  float minErr = 1e20f;
  float maxErr = 0.f;
  std::vector<float> meanArray(numTotalNodes_);

  // Save errors
  std::vector<float> errors(numTotalNodes_);

  // Calculate temporal error for one brick at a time
  for (unsigned int brick=0; brick<numTotalNodes_; ++brick) {

    unsigned int numBrickVals = 
      paddedBrickDim_*paddedBrickDim_*paddedBrickDim_;

    // Save the individual voxel's average over timesteps. Because the
    // BSTs are built by averaging leaf nodes, we only need to sample
    // the brick at the correct coordinate.
    std::vector<float> voxelAverages(numBrickVals);
    std::vector<float> voxelStdDevs(numBrickVals);

    // Read the whole brick to fill the averages
    std::ios::pos_type offset = dataPos_ +
      static_cast<std::ios::pos_type>(brick*numBrickVals*sizeof(float));

    in.seekp(offset);
    in.read(reinterpret_cast<char*>(&voxelAverages[0]), 
            sizeof(float)*numBrickVals);

    // Build a list of the BST leaf bricks (within the same octree level) that
    // this brick covers
    std::list<unsigned int> coveredBricks = CoveredBSTLeafBricks(brick);

    // Calculate standard deviation per voxel, average over brick
    float avgStdDev = 0.f;
    for (unsigned int voxel=0; voxel<numBrickVals; ++voxel) {

      float stdDev = 0.f;
      for (auto leaf = coveredBricks.begin(); 
           leaf != coveredBricks.end(); ++leaf) {

        // Sample the leaves at the corresponding voxel position
        std::ios::pos_type sampleOffset = dataPos_ +
          static_cast<std::ios::pos_type>(
            (*leaf*numBrickVals+voxel)*sizeof(float));

        float sample;
        in.seekp(sampleOffset);
        in.read(reinterpret_cast<char*>(&sample), sizeof(float));

        stdDev += pow(sample-voxelAverages[voxel], 2.f);
      }
      stdDev /= static_cast<float>(coveredBricks.size());
      stdDev = sqrt(stdDev);

      avgStdDev += stdDev;
    } // for voxel

    avgStdDev /= static_cast<float>(numBrickVals);
    meanArray[brick] = avgStdDev;
    errors[brick] = avgStdDev;

    if (avgStdDev < minErr) {
      minErr = avgStdDev;
    } else if (avgStdDev > maxErr) {
      maxErr = avgStdDev;
    }

  } // for all bricks
  
  in.close();

  std::sort(meanArray.begin(), meanArray.end());
  float medErr = meanArray[meanArray.size()/2];

  INFO("");
  INFO("Min temporal error: " << minErr);
  INFO("Max temporal error: " << maxErr);
  INFO("Median temporal error: " << medErr); 

  // Normalize errors
  float minNorm = 1e20f;
  float maxNorm = 0.f;
  for (unsigned int i=0; i<numTotalNodes_; ++i) {
    //float normalized = (stdDevs[i]-minError)/(maxError-minError);
    errors[i] = pow(errors[i], 0.25f);
    data_[i*NUM_DATA+TEMPORAL_ERR] = *reinterpret_cast<int*>(&errors[i]);
    if (errors[i] < minNorm) {
      minNorm = errors[i];
    } else if (errors[i] > maxNorm) {
      maxNorm = errors[i];
    }
  }
  
  std::sort(errors.begin(), errors.end());
  float medNorm = errors[errors.size()/2];

  minTemporalError_ = minNorm;
  maxTemporalError_ = maxNorm;
  medianTemporalError_ = medNorm;

  INFO("Min normalized spatial std dev: " << minNorm);
  INFO("Max normalized spatial std dev: " << maxNorm);
  INFO("Median normalized spatial std dev: " << medNorm);
  INFO("");


  return true;
}

std::list<unsigned int> TSP::CoveredBSTLeafBricks(unsigned int _brickIndex) {
  std::list<unsigned int> out;

  // Traverse the BST children until we are at root
  std::queue<unsigned int> queue;
  queue.push(_brickIndex);
  do {

    unsigned int toVisit = queue.front();
    queue.pop();

    bool BSTRoot = toVisit < numOTNodes_;
    if (BSTRoot) {
      if (numBSTLevels_ == 1) {
        out.push_back(toVisit);
      } else {
        queue.push(toVisit + numOTNodes_);
        queue.push(toVisit + numOTNodes_*2);
      }
    } else {
      int child = data_[toVisit*NUM_DATA + CHILD_INDEX];
      if (child == -1) {
        // Save leaf brick to list
        out.push_back(toVisit);
      } else {
        // Queue children
        queue.push(child);
        queue.push(child+numOTNodes_);
      }
    }

    } while (!queue.empty());

  return out;
}


std::list<unsigned int> TSP::CoveredLeafBricks(unsigned int _brickIndex) {
  std::list<unsigned int> out;

  // Find what octree skeleton node the index belongs to
  unsigned int OTNode = _brickIndex % numOTNodes_;

  // Find what BST node the index corresponds to using int division
  unsigned int BSTNode = _brickIndex / numOTNodes_;

  // Calculate BST offset (to translate to root octree)
  unsigned int BSTOffset = BSTNode * numOTNodes_;

  // Traverse root octree structure to leaves
  // When visiting the leaves, translate back to correct BST level and save
  std::queue<unsigned int> queue;
  queue.push(OTNode);
  do {

    // Get front of queue and pop it
    unsigned int toVisit = queue.front();
    queue.pop();

    // See if the node has children
    int child = data_[toVisit*NUM_DATA + CHILD_INDEX];
    if (child == -1) {
      // Translate back and save
      out.push_back(toVisit+BSTOffset);
    } else {
      // Queue the eight children
      for (int i=0; i<8; ++i) {
        queue.push(child+i);
      }
    }
  
  } while (!queue.empty());

  return out;
}


float TSP::SquaredDist(Color _c1, Color _c2) {
  // Weighted by the c1 opacity, because transparent voxels have
  // less contribution to the final image
  return _c1.a*( pow(_c1.r-_c2.r, 2.f) + pow(_c1.g-_c2.g, 2.f) + 
    pow(_c1.b-_c2.b, 2.f) ) + pow(_c1.a-_c2.a, 2.f);
}


bool TSP::ReadCache() {

  std::string cacheFilename = config_->TSPFilename() + ".cache";
  INFO("Looking for cache file " << cacheFilename);
  
  std::fstream in;
  in.open(cacheFilename.c_str(), std::ios_base::in | std::ios_base::binary);
  if (!in.is_open()) {
    INFO("Failed to open cache file " << cacheFilename);
    return false;
  }

  size_t dataSize = numTotalNodes_*NUM_DATA*sizeof(int);
  
  in.read(reinterpret_cast<char*>(&minSpatialError_), sizeof(float));
  in.read(reinterpret_cast<char*>(&maxSpatialError_), sizeof(float));
  in.read(reinterpret_cast<char*>(&medianSpatialError_), sizeof(float));
  in.read(reinterpret_cast<char*>(&minTemporalError_), sizeof(float));
  in.read(reinterpret_cast<char*>(&maxTemporalError_), sizeof(float));
  in.read(reinterpret_cast<char*>(&medianTemporalError_), sizeof(float));
  in.read(reinterpret_cast<char*>(&data_[0]), dataSize);
  in.close();

  INFO("");
  INFO("Min spatial error: " << minSpatialError_);
  INFO("Max spatial error: " << maxSpatialError_);
  INFO("Median spatial error: " << medianSpatialError_);
  INFO("Min temporal error: " << minTemporalError_);
  INFO("Max temporal error: " << maxTemporalError_);
  INFO("Median temporal error: " << medianTemporalError_);

  return true;
}

bool TSP::WriteCache() {
  
  std::string cacheFilename = config_->TSPFilename() + ".cache";
  INFO("Writing cache to " << cacheFilename);

  std::fstream out;
  out.open(cacheFilename.c_str(),
    std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
  if (!out.is_open()) {
    ERROR("Failed to open " << cacheFilename);
    return false;
  }

  out.seekp(std::ios_base::beg);
  out.write(reinterpret_cast<char*>(&minSpatialError_), sizeof(float));
  out.write(reinterpret_cast<char*>(&maxSpatialError_), sizeof(float));
  out.write(reinterpret_cast<char*>(&medianSpatialError_), sizeof(float));
  out.write(reinterpret_cast<char*>(&minTemporalError_), sizeof(float));
  out.write(reinterpret_cast<char*>(&maxTemporalError_), sizeof(float));
  out.write(reinterpret_cast<char*>(&medianTemporalError_), sizeof(float));
  out.write(reinterpret_cast<char*>(&data_[0]), data_.size()*sizeof(int));
  out.close();

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

