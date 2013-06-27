struct TraversalConstants {
  int numTimesteps_;
  int numValuesPerNode_;
};

// Return index to the octree root (same index as BST root at that OT node)
int OctreeRootNodeIndex() {
  return 0;
}

// Return index to left BST child (low timespan)
int LeftBST(int _bstNodeIndex, int _numValuesPerNode) {
  // Left BST child is next to the current one
  return _bstNodeIndex + _numValuesPerNode;
}

// Return index to right BST child (high timespan)
int RightBST(int _bstNodeIndex, int _numValuesPerNode) {
  // Right BST child is two steps away from the current one
  return _bstNodeIndex + 2*_numValuesPerNode;
}

// Return child node index given a BST node, a time span and a timestep
// Updates timespan
int ChildNodeIndex(int _bstNodeIndex, 
                   int *_timespanStart,
                   int *_timespanEnd,
                   int _timestep) {
  // Choose left or right child
  int middle = *_timespanStart + (*_timeSpanEnd - *_timeSpanStart)/2; 
  if (_timestep <= middle) {
    // Left subtree
    *_timespanEnd = middle;
    return LeftBST(_bstNodeIndex);
  } else {
    // Right subtree
    *_timespanStart = middle+1;
    return RightBST(bstNodeIndex);
  }
}

// Return the brick index that a BST node represents
int BrickIndex(int _bstNodeIndex, int _numValuesPerNode, int *_tsp) {
  return _tsp[_bstNodeIndex+0];
}

// Given an octree node index, traverse the corresponding BST tree and look
// for a useful brick. 
bool TraverseBST(int _otNodeIndex, 
                 int *_brickIndex, 
                 int _timestep,
                 TraversalConstants _constants,
                 float _temporalTolerance,
                 float _spatialTolerance) {

  // Start at the root of the current BST
  int bstNodeIndex = _otNodeIndex;
  int timespanStart = 0;
  int timespanEnd = _constants->numTimesteps_;

  // Rely on structure for termination
  while (true)
  {
  
    *_brickIndex = BrickIndex(bstNodeIndex);
    if (TemporalError(bstNodeIndex) <= _temporalTolerance) {
      
      // If the ot node is a leaf, we can't do any better spatially so we 
      // return the current brick
      if (IsOctreeLeaf(_otNodeIndex)) {
        return true;

      // All is well!
      } else if (SpatialError(bstNodeIndex) <= _spatialTolerance) {
        return true;
         
      // If spatial failed and the BST node is a leaf
      // The traversal will continue in the octree (we know that
      // the octree node is not a leaf)
      } else if (IsBSTLeaf(bstNodeIndex)) {
        return false;
      
      // Keep traversing BST
      } else {
        bstNodeIndex = ChildNodeIndex(bstNodeIndex,
                                      &timespanStart,
                                      &timespanEnd,
                                      _timestep);
      }

    // If temporal error is too big and the node is a leaf
    // Return false to traverse OT
    } else if (IsBSTLeaf(bstNodeIndex)) {
      return false;
    
    // If temporal error is too big and we can continue
    } else {
      bstNodeIndex = ChildNodeIndex(bstNodeIndex,
                                    &timespanStart,
                                    &timespanEnd,
                                    _timestep);
    }
  } 
}


// Traverse one ray through the volume, build brick list
void TraverseOctree(float3 _rayO, float3 _rayD) {

  float boxDim, boxMid, boxMin;
  int otNode, level;
  float3 offset;

  // Find tMax and tMin for unit cube
  float tMin, tMax, tMinNode, tMaxNode;
  if (!IntersectCube(float3(0.0), float3(1.0), _rayO, _rayD, &tMin, &tMax)) {
    return;
  }
  tMinNode = tMin;
  tMaxNode = tMax;

  // Keep traversing until the sample point goes outside the unit cube
  while (tMin < tMax) {

    // Reset traversal variables
    offset = float3(0.0);
    boxDim = 1.0;
    level = 0;
    int child;

    // Init the octree node index to the root
    int otNodeIndex = OctreeRootIndex();

    // Find the point P where the ray intersects the unit cube
    float3 P = _rayO + tMin*_rayD;

    // Start traversing octree
    // Rely on finding a leaf for loop termination 
    while (true) {
      
      // See if the BST tree is good enough
      int brickIndex;
      bool bstSuccess = TraverseBST(otNodeIndex, &brickIndex);

      if (bstSuccess) {
        
        // Add the found brick to brick list
        AddToList(brickIndex);
        // We are now done with this node, so go to next
        tMin = tMaxNode + 0.0001;
        break;
        
      // If the BST lookup failed but the octree node is a leaf, 
      // add the brick anyway (it is the BST leaf)
      } else if (IsOctreeLeaf(otNode)) {
        
        AddToList(brickIndex);
        // We are now done with this node, so go to next
        tMin = tMaxNode + 0.0001;
        break;

      // If the BST lookup failed and we can traverse the octree,
      // visit the child that encloses the point
      } else {

        // Next box dimension
        boxDim /= 2;

        // Current mid point
        boxMid = boxDim;

        // Check which child encloses P
        child = EnclosingChild(P, boxMid, offset);

        // Update offset
        UpdateOffset(&offset, child, boxDim);

        // Update node index to new node
        otNodeIndex = OTChildIndex(otNodeIndex, child);

        level++;  
      } 

    } // while traversing

    // Find tMax for the node to visit next
    float tMinNode, tMaxNode;
    if (!IntersectCube(offset, offset+float3(boxDim), _rayO, _rayD,
        &tMinNode, &tMaxNode)) {
      // This should never happen
      return;
    }

    // Update (add small offset)
    tMin = tMaxNode + 0.0001;

  } // while (tMin < tMax)

}




__kernel void TSPTraversal(__global __read_only image2d_t _cubeFront,
                           __global __read_only image2d_t _cubeBack,
                           __global __read_only float *_tf,
                           __global __read_only int *_tsp,
                           __global __write_only int *_brickList) {

    // Kernel should be launched in 2D with one work item per pixel
    int2 intCoords = (int2)(get_global_id(0), get_global_id(1));

    // Sampler for color cube reading
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;

    // Read from color cube textures
    float4 cubeFrontColor = read_imagef(_cubeFront, sampler, intCoords);
    float4 cubeBackColor = read_imagef(_cubeBack, sampler, intCoords);

    // Figure out ray direction 
    float3 direction = normalize(cubeBackColor-cubeFrontColor).xyz;

    // Traverse structure, write to brick list
    Traverse(cubeFrontColor.xyz, direction, _tf, _tsp, _brickList);

}
