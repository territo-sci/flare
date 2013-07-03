// Mirrors struct on host side
struct TraversalConstants {
  int numTimesteps_;
  int numValuesPerNode_;
  int numBSTNodesPerOT_;
  int timestep_;
  int temporalTolerance_;
  int spatialTolerance_;
};

// Return index to the octree root (same index as BST root at that OT node)
int OctreeRootNodeIndex() {
  return 0;
}

// Return index to left BST child (low timespan)
int LeftBST(int _bstNodeIndex, int _numValuesPerNode, 
            bool _bstRoot, __global __read_only int *_tsp) {
  // If the BST node is a root, the child pointer is used for the OT. 
  // The child index is next to the root.
  // If not root, look up in TSP structure.
  if (_bstRoot) {
    return _bstNodeIndex + 1;
  } else {
    return _tsp[_bstNodeIndex*_numValuesPerNode + 1];
  }
}

// Return index to right BST child (high timespan)
int RightBST(int _bstNodeIndex, int _numValuesPerNode,
             bool _bstRoot, __global __read_only int *_tsp) {
  if (_bstRoot) {
    return _bstNodeIndex + 2;
  } else {
    return _tsp[_bstNodeIndex*_numValuesPerNode + 1] + 1;
  }
}

// Return child node index given a BST node, a time span and a timestep
// Updates timespan
int ChildNodeIndex(int _bstNodeIndex, 
                   int *_timespanStart,
                   int *_timespanEnd,
                   int _timestep,
                   int _numValuesPerNode,
                   bool _bstRoot,
                   __global __read_only int *_tsp) {
  // TODO just return left
  return LeftBST(_bstNodeIndex, _numValuesPerNode, _bstRoot, _tsp);
  
  // Choose left or right child
  int middle = *_timespanStart + (*_timespanEnd - *_timespanStart)/2; 
  if (_timestep <= middle) {
    // Left subtree
    *_timespanEnd = middle;
    return LeftBST(_bstNodeIndex, _numValuesPerNode, _bstRoot, _tsp);
  } else {
    // Right subtree
    *_timespanStart = middle+1;
    return RightBST(_bstNodeIndex, _numValuesPerNode, _bstRoot, _tsp);
  }
}

// Return the brick index that a BST node represents
int BrickIndex(int _bstNodeIndex, int _numValuesPerNode, 
               __global __read_only int *_tsp) {
  return _tsp[_bstNodeIndex*_numValuesPerNode + 0];
}

// Checks if a BST node is a leaf ot not
bool IsBSTLeaf(int _bstNodeIndex, int _numValuesPerNode, 
               bool _bstRoot, __global __read_only int *_tsp) {
  if (_bstRoot) return false;
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
  return firstChild + _numBSTNodes*_child;
}

// Increment the count for a brick
void AddToList(int _brickIndex, 
               __global volatile int *_brickList) {
  atomic_inc(&_brickList[_brickIndex]);
}

int TemporalError(int _bstNodeIndex, int _numValuesPerNode, 
                    __global __read_only int *_tsp) {
  return (int)(_tsp[_bstNodeIndex*_numValuesPerNode + 3]);
}

int SpatialError(int _bstNodeIndex, int _numValuesPerNode, 
                   __global __read_only int *_tsp) {
  return (int)(_tsp[_bstNodeIndex*_numValuesPerNode + 2]);
}


// Given an octree node index, traverse the corresponding BST tree and look
// for a useful brick. 
bool TraverseBST(int _otNodeIndex,
                 int *_brickIndex, 
                 int _timestep,
                 __constant struct TraversalConstants *_constants,
                 __global volatile int *_brickList,
                 __global __read_only int *_tsp) {
  
  // Start at the root of the current BST
  int bstNodeIndex = _otNodeIndex;
  bool bstRoot = true;
  int timespanStart = 0;
  int timespanEnd = _constants->numTimesteps_;

  //AddToList(_otNodeIndex, _brickList);

  // Rely on structure for termination
  while (true) {
  
    *_brickIndex = BrickIndex(bstNodeIndex, 
                              _constants->numValuesPerNode_,
                              _tsp);

    // If temporal error is ok
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
                           bstRoot, _tsp)) {
        return false;
      
      // Keep traversing BST
      } else {
        bstNodeIndex = ChildNodeIndex(bstNodeIndex,
                                      &timespanStart,
                                      &timespanEnd,
                                      _timestep,
                                      _constants->numValuesPerNode_,
                                      bstRoot,
                                      _tsp);
      }

    // If temporal error is too big and the node is a leaf
    // Return false to traverse OT
    } else if (IsBSTLeaf(bstNodeIndex, _constants->numValuesPerNode_, 
                         bstRoot, _tsp)) {
      return false;
    
    // If temporal error is too big and we can continue
    } else {
      bstNodeIndex = ChildNodeIndex(bstNodeIndex,
                                    &timespanStart,
                                    &timespanEnd,
                                    _timestep,
                                    _constants->numValuesPerNode_,
                                    bstRoot,
                                    _tsp);
    }

    bstRoot = false;
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
    *_offset += (float3)(_boxDim);
  }
}

// Traverse one ray through the volume, build brick list
void TraverseOctree(float3 _rayO, 
                    float3 _rayD,
                    float _maxDist,
                    __constant struct TraversalConstants *_constants,
                    __global volatile int *_brickList,
                    __global __read_only int *_tsp) {
 
  // Choose a stepsize that guarantees that we don't miss any bricks
  // TODO dynamic depending on brick dimensions
  float stepsize = 0.1;
  float3 P = _rayO;
  // Keep traversing until the sample point goes outside the unit cube
  float traversed = 0.0;
  while (traversed < _maxDist) {
    
    // Reset traversal variables
    float3 offset = (float3)(0.0);
    float boxDim = 1.0;
    int child;
    bool foundBrick = false;

    // Init the octree node index to the root
    int otNodeIndex = OctreeRootNodeIndex();

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
        foundBrick = true;
        break;
        
      // If the BST lookup failed but the octree node is a leaf, 
      // add the brick anyway (it is the BST leaf)
      } else if (IsOctreeLeaf(otNodeIndex, 
                              _constants->numValuesPerNode_, _tsp)) {
        
        AddToList(brickIndex, _brickList);
        // We are now done with this node, so go to next
        foundBrick = true;
        break;

      // If the BST lookup failed and we can traverse the octree,
      // visit the child that encloses the point
      } else {

        // Next box dimension
        boxDim = boxDim/2.0;

        // Current mid point
        float boxMid = boxDim;

        // Check which child encloses P
        child = EnclosingChild(P, boxMid, offset);

        // Update offset
        UpdateOffset(&offset, boxDim, child);

        // Update node index to new node
        otNodeIndex = OTChildIndex(otNodeIndex, _constants->numValuesPerNode_,
                                   _constants->numBSTNodesPerOT_, child, _tsp);
      } 

    } // while traversing

    // Update 
    traversed += stepsize;
    P += stepsize * _rayD;

  } // while (traversed < maxDist)
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
    if (length(cubeFrontColor.xyz) == 0.0) return;
    float4 cubeBackColor = read_imagef(_cubeBack, sampler, intCoords);

    // Figure out ray direction 
    float maxDist = length(cubeBackColor.xyz-cubeFrontColor.xyz);
    float3 direction = normalize(cubeBackColor.xyz-cubeFrontColor.xyz);

    TraverseOctree(cubeFrontColor.xyz, direction, maxDist,
                   _constants,  _brickList, _tsp);

    return;

}
