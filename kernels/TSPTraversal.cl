// Mirrors struct on host side
struct TraversalConstants {
  int numTimesteps_;
  int numValuesPerNode_;
  int numBSTNodesPerOT_;
  int timestep_;
  int temporalTolerance_;
  int spatialTolerance_;
};

// Intersect a ray specifiec by origin and direction
// with a box specified by opposing corners.
// Returns intersect/no intersect along with t values 
// for intersections points.
bool IntersectBox(float3 _boundsMin, float3 _boundsMax,
                   float3 _rayO, float3 _rayD,
                   float *_tMinOut, float *_tMaxOut) {
  float _tMin, _tMax, tYMin, tYMax, tZMin, tZMax;
  float divx = (_rayD.x == 0.0) ? 1e20 : 1.0/_rayD.x;
  if (divx >= 0.0) {
    _tMin = (_boundsMin.x - _rayO.x) * divx;
    _tMax = (_boundsMax.x - _rayO.x) * divx;
  } else {
    _tMin = (_boundsMax.x - _rayO.x) * divx;
    _tMax = (_boundsMin.x - _rayO.x) * divx;
  }
  float divy = (_rayD.y == 0.0) ? 1e20 : 1.0/_rayD.y;
  if (divy >= 0.0) {
    tYMin = (_boundsMin.y - _rayO.y) * divy;
    tYMax = (_boundsMax.y - _rayO.y) * divy;
  } else {
    tYMin = (_boundsMax.y - _rayO.y) * divy;
    tYMax = (_boundsMin.y - _rayO.y) * divy;
  }
  if ( (_tMin > tYMax || tYMin > _tMax) ) return false;
  if (tYMin > _tMin) _tMin = tYMin;
  if (tYMax < _tMax) _tMax = tYMax;
  float divz = (_rayD.z == 0.0) ? 1e20 : 1.0/_rayD.z;
  if (divz >= 0.0) {
    tZMin = (_boundsMin.z - _rayO.z) * divz;
    tZMax = (_boundsMax.z - _rayO.z) * divz;
  } else {
    tZMin = (_boundsMax.z - _rayO.z) * divz;
    tZMax = (_boundsMin.z - _rayO.z) * divz;
  }
  if ( (_tMin > tZMax || tZMin > _tMax) ) return false;
  if (tZMin > _tMin) _tMin = tZMin;
  if (tZMax < _tMax) _tMax = tZMax;
  *_tMinOut = _tMin;
  *_tMaxOut = _tMax;
  return ( (_tMin < 1e20 && _tMax > -1e20 ) );
}

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
                   int _timestep,
                   int _numValuesPerNode) {
  // Choose left or right child
  int middle = *_timespanStart + (*_timespanEnd - *_timespanStart)/2; 
  if (_timestep <= middle) {
    // Left subtree
    *_timespanEnd = middle;
    return LeftBST(_bstNodeIndex, _numValuesPerNode);
  } else {
    // Right subtree
    *_timespanStart = middle+1;
    return RightBST(_bstNodeIndex, _numValuesPerNode);
  }
}

// Return the brick index that a BST node represents
int BrickIndex(int _bstNodeIndex, int _numValuesPerNode, 
               __global __read_only int *_tsp) {
  return _tsp[_bstNodeIndex*_numValuesPerNode + 0];
}

// Checks if a BST node is a leaf ot not
bool IsBSTLeaf(int _bstNodeIndex, int _numValuesPerNode, 
               __global __read_only int *_tsp) {
  return (_tsp[_bstNodeIndex*_numValuesPerNode + 1] == -1);
}

// Checks if an OT node is a leaf or not
bool IsOctreeLeaf(int _otNodeIndex, int _numValuesPerNode, 
                  __global __read_only int *_tsp) {
  // CHILD_INDEX is at offset 1, and -1 represents leaf
  return (_tsp[_otNodeIndex*_numValuesPerNode + 1] == -1);
}

// Return OT child index given current node and child number (0-7)
int OTChildIndex(int _otNodeIndex, int _numValuesPerNode, int _numBSTNodes,
                 int _child, 
                 __global __read_only int *_tsp) {
  int firstChild = _tsp[_otNodeIndex*_numValuesPerNode + 1];
  return firstChild + _numValuesPerNode*_numBSTNodes*_child;
}

// Increment the count for a brick
void AddToList(int _brickIndex, 
               __global int *_brickList) {
  _brickList[_brickIndex]++;
}

float TemporalError(int _bstNodeIndex, int _numValuesPerNode, 
                    __global __read_only int *_tsp) {
  return (float)(_tsp[_bstNodeIndex*_numValuesPerNode + 3]);
}

float SpatialError(int _bstNodeIndex, int _numValuesPerNode, 
                   __global __read_only int *_tsp) {
  return (float)(_tsp[_bstNodeIndex*_numValuesPerNode + 2]);
}


// Given an octree node index, traverse the corresponding BST tree and look
// for a useful brick. 
bool TraverseBST(int _otNodeIndex,
                 int *_brickIndex, 
                 int _timestep,
                 __constant struct TraversalConstants *_constants,
                 __global int *_brickList,
                 __global __read_only int *_tsp) {

  // Start at the root of the current BST
  int bstNodeIndex = _otNodeIndex;
  int timespanStart = 0;
  int timespanEnd = _constants->numTimesteps_;

  // Rely on structure for termination
  while (true)
  {
  
    *_brickIndex = BrickIndex(bstNodeIndex, 
                              _constants->numValuesPerNode_,
                              _tsp);
    if (TemporalError(bstNodeIndex, _constants->numValuesPerNode_,
                      _tsp) == _constants->temporalTolerance_) {
      
      // If the ot node is a leaf, we can't do any better spatially so we 
      // return the current brick
      // TODO float and <= errors
      // TEST pick one specific level
      if (IsOctreeLeaf(_otNodeIndex, _constants->numValuesPerNode_, _tsp)) {
        return true;

      // All is well!
      } else if (SpatialError(bstNodeIndex, _constants->numValuesPerNode_,
                 _tsp) == _constants->spatialTolerance_) {
        return true;
         
      // If spatial failed and the BST node is a leaf
      // The traversal will continue in the octree (we know that
      // the octree node is not a leaf)
      } else if (IsBSTLeaf(bstNodeIndex, _constants->numValuesPerNode_, 
                           _tsp)) {
        return false;
      
      // Keep traversing BST
      } else {
        bstNodeIndex = ChildNodeIndex(bstNodeIndex,
                                      &timespanStart,
                                      &timespanEnd,
                                      _timestep,
                                      _constants->numValuesPerNode_);
      }

    // If temporal error is too big and the node is a leaf
    // Return false to traverse OT
    } else if (IsBSTLeaf(bstNodeIndex, _constants->numValuesPerNode_, _tsp)) {
      return false;
    
    // If temporal error is too big and we can continue
    } else {
      bstNodeIndex = ChildNodeIndex(bstNodeIndex,
                                    &timespanStart,
                                    &timespanEnd,
                                    _timestep,
                                    _constants->numValuesPerNode_);
    }
  } 
}

// Given a point, a box mid value and an offset, return enclosing child
int EnclosingChild(float3 _P, float _boxMid, float3 _offset) {
  if (_P.x < _boxMid+_offset.x) {
    if (_P.y < _boxMid+_offset.y) {
      if (_P.z < _boxMid+_offset.z) {
        return 0;
      } else {
        return 4;
      }
    } else {
      if (_P.z < _boxMid+_offset.z) {
        return 2;
      } else {
        return 6;
      }
    }
  } else {
    if (_P.y < _boxMid+_offset.y) {
      if (_P.z < _boxMid+_offset.z) {
        return 1;
      } else {
        return 5;
      }
    } else {
      if (_P.z < _boxMid+_offset.z) {
        return 3;
      } else {
        return 7;
      }
    }
  }
}

void UpdateOffset(float3 *_offset, float _boxDim, int _child) {
  if (_child == 0) {
    // do nothing
  } else if (_child == 1) {
    _offset->x += _boxDim;
  } else if (_child == 2) {
    _offset->y += _boxDim;
  } else if (_child == 3) {
    _offset->x += _boxDim;
    _offset->y += _boxDim;
  } else if (_child == 4) {
    _offset->z += _boxDim;
  } else if (_child == 5) {
    _offset->x += _boxDim;
    _offset->z += _boxDim;
  } else if (_child == 6) {
    _offset->y += _boxDim;
    _offset->z += _boxDim;
  } else if (_child == 7) {
    _offset->x += _boxDim;
    _offset->y += _boxDim;
    _offset->z += _boxDim;
  }
}

// Traverse one ray through the volume, build brick list
void TraverseOctree(float3 _rayO, 
                    float3 _rayD,
                    __constant struct TraversalConstants *_constants,
                    __global int *_brickList,
                    __global __read_only int *_tsp) {

  float boxDim, boxMid, boxMin;
  int otNode, level;
  float3 offset;

  // Find tMax and tMin for unit cube
  float tMin, tMax, tMinNode, tMaxNode;
  if (!IntersectBox((float3)(0.0), (float3)(1.0), _rayO, _rayD, &tMin, &tMax)){
    return;
  }
  tMinNode = tMin;
  tMaxNode = tMax;

  // Keep traversing until the sample point goes outside the unit cube
  while (tMin < tMax) {

    // Reset traversal variables
    offset = (float3)(0.0);
    boxDim = 1.0;
    level = 0;
    int child;

    // Init the octree node index to the root
    int otNodeIndex = OctreeRootNodeIndex();

    // Find the point P where the ray intersects the unit cube
    float3 P = _rayO + tMin*_rayD;

    // Start traversing octree
    // Rely on finding a leaf for loop termination 
    while (true) {
      
      // See if the BST tree is good enough
      int brickIndex;
      bool bstSuccess = TraverseBST(otNodeIndex, 
                                    &brickIndex,
                                    _constants->timestep_,
                                    _constants,
                                    _brickList,
                                    _tsp);

      if (bstSuccess) {
        
        // Add the found brick to brick list
        AddToList(brickIndex, _brickList);
        // We are now done with this node, so go to next
        tMin = tMaxNode + 0.0001;
        break;
        
      // If the BST lookup failed but the octree node is a leaf, 
      // add the brick anyway (it is the BST leaf)
      } else if (IsOctreeLeaf(otNode, _constants->numValuesPerNode_, _tsp)) {
        
        AddToList(brickIndex, _brickList);
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
        otNodeIndex = OTChildIndex(otNodeIndex, _constants->numValuesPerNode_,
                                   _constants->numBSTNodesPerOT_, child, _tsp);

        level++;  
      } 

    } // while traversing

    // Find tMax for the node to visit next
    if (!IntersectBox(offset, offset+(float3)(boxDim), _rayO, _rayD,
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
                           __constant struct TraversalConstants *_constants,
                           __global __read_only int *_tsp,
                           __global int *_brickList) {

    // Kernel should be launched in 2D with one work item per pixel
    int2 intCoords = (int2)(get_global_id(0), get_global_id(1));

    // Sampler for color cube reading
    const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;

    // Read from color cube textures
    float4 cubeFrontColor = read_imagef(_cubeFront, sampler, intCoords);
    float4 cubeBackColor = read_imagef(_cubeBack, sampler, intCoords);

    // Figure out ray direction 
    float3 direction = normalize(cubeBackColor-cubeFrontColor).xyz;

    TraverseOctree(cubeFrontColor.xyz, direction, 
                   _constants,  _brickList, _tsp);

}
