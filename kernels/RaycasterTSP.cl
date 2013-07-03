struct KernelConstants {
  float stepsize_;
  float intensity_;
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
  // TODO just return left until we know it works (timestep 0)
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

int TemporalError(int _bstNodeIndex, int _numValuesPerNode, 
                    __global __read_only int *_tsp) {
  return (int)(_tsp[_bstNodeIndex*_numValuesPerNode + 3]);
}

int SpatialError(int _bstNodeIndex, int _numValuesPerNode, 
                   __global __read_only int *_tsp) {
  return (int)(_tsp[_bstNodeIndex*_numValuesPerNode + 2]);
}

// Converts a global coordinate [0..1] to a box coordinate [0..1]
int3 BoxCoords(float3 _globalCoords, int _boxesPerAxis) {
  int3 boxCoords = convert_int3(floor(_globalCoords * (float)(_boxesPerAxis)));
  return clamp (boxCoords, (int3)(0, 0, 0), (int3)(_boxesPerAxis-1));
}

// Fetch atlas box coordinates from brick list
int3 AtlasBoxCoords(int _brickIndex, 
                    __global __read_only int *_brickList) {
  int x = _brickList[3*_brickIndex+0];
  int y = _brickList[3*_brickIndex+1];
  int z = _brickList[3*_brickIndex+2];
  return (int3)(x, y, z);
}

// Sample atlas
float4 SampleAtlas(float3 _cartesianCoords,
                   const sampler_t _atlasSampler,
                   __global __read_only float *_transferFunction,

                  ) {
  // Go to spherical coordinates
  float3 spherical = CartesianToSpherical(_cartesianCoords);
  


// Convert a global coordinate [0..1] to a texture atlas coordinate [0..1]
float3 AtlasCoords(float3 _globalCoords, int _boxesPerAxis,
                   __global __read_only int *_brickList) {
  // Start with finding the coordinates for the "box" the point sits in
  int3 boxCoords = BoxCoords(_globalCoords, _boxesPerAxis);
  // Fetch the brick's texture atlas (box) coordinates
  int3 atlasBoxCoords = AtlasBoxCoords(_brickIndex, _brickList);
  // Convert and return texture atlas coordinates
  int3 diff = boxCoords-atlasBoxCoords;
  return _globalCoords - convert_float3(diff)/(float)(_boxesPerAxis));
}



bool TraverseBST(int _otNodeIndex, int *_brickIndex,
                 __global __read_only struct KernelConstants *_constants,
                 __global __read_only int *_tsp) {
  // Start att the root of the current BST
  int bstNodeIndex = _otNodeIndex;
  bool bstRoot = true;
  int timespanStart = 0;
  int timespanEnd = _constants->numTimesteps_;

  while (true) {
    *_brickIndex = BrickIndex(bstNodeIndex, _constants->NumValuesPerNode_,
                              _tsp);

    // Check temporal error
    if (TemporalError(bstNodeIndex, _constants->numValuesPerNode_, _tsp) ==
        _constants->temporalTolerance_) {

      // If the OT node is a leaf, we cannot do any better spatially
      if (IsOctreeLeaf(_otNodeIndex, _constants->numValuesPerNode_, _tsp)) {
        return true;

      } else if (SpatialError(bstNodeIndex, _constants->numValuesPerNode_,
                              _tsp) == _constants->spatialTolerance_) {
        return true;

      } else if (IsBSTLeaf(bstNodeIndex, _constants->numValuesPerNode_,
                           bstRoot, _tsp)) {
        return false;

      } else {

        // Keep traversing
        bstNodeIndex = ChildNodeIndex(bstNodeIndex, &timespanStart,
                                      &timespanEnd, 
                                      _constants->timestep_,
                                      _constants->numValuesPerNode_,
                                      bstRoot, _tsp);
      }

    } else if (IsBstLeaf(bstNodeIndex, _constants->numValuesPerNode_,
                         bstRoot, _tsp)) {
      return false;

    } else {

      // Keep traversing
      bstNodeIndex = ChildNodeIndex(bstNodeIndex, &timespanStart,
                                    &timespanEnd,
                                    _constants->timestep_,
                                    _constants->numValuesPerNode_,
                                    bstRoot, _tsp);
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

float4 TraverseOctree(float3 _rayO, float3 _rayD, float _maxDist,
                      __constant struct KernelConstants *_constants,
                      __global __read_only float *_transferFunction,
                      __global __read_only int *_tsp,
                      __global __read_only int *_brickList) {

  float stepsize = _constants->stepsize_;
  // Sample point
  float3 cartesianP = _rayO;
  // Keep track of distance traveled
  float traversed = 0;
  mulative color to return
  float4 color = (float4)(0.0);

  // Traverse until sample point is outside of volume
  while (traversed < _maxDist) {
  
    // Reset octree traversal variables
    float3 offset = (float3)(0.0);
    float boxDim = 1.0;
    bool foundBrick = false;
    int child;

    int otNodeIndex = OctreeRootNodeIndex();

    // Rely on finding a leaf for loop termination
    while (true) {

      // Traverse BST to get a brick index, and see if the found brick
      // is good enough
      int brickIndex;
      bool bstSuccess = TraverseBST(otNodeIndex,
                                    &brickIndex,
                                    _constants,
                                    _tsp);

      if (bstSuccess || 
        IsOctreeLeaf(otNodeIndex, _constants->numValuesPerNode_, _tsp)) {

        // Sample the brick
        float i = SampleAtlas(cartesianP, brickIndex_, _brickList);
        float tf = TransferFunction(i, _transferFunction);
        color += (1.0 - color.w)*tf;
        break;

      } else {
           
        // Keep traversing the octree
        
        // Next box dimension
        boxDim /= 2.0;
        
        // Current mid point
        float boxMid = boxDim;

        // Check which child encloses the sample point
        child = EnclosingChild(cartesianP, boxMid, offset);

        // Update offset for next level
        UpdateOffset(&offset, boxDim, child);

        // Update index to new node
        otNodeIndex = OTChildIndex(otNodeIndex, _constants->numValuesPerNode_,
                                   _constants->numBSTNodesPerOT_, child, _tsp);

      }

    } // while traversing

    // Update sample point
    traversed += stepsize;
    cartesianP += stepsize * _rayD;

  } // while (traversed < maxDist)

  return color;

}


__kernel void RaycasterTSP(__global __read_only image2d_t _cubeFront,
                           __global __read_only image2d_t, _cubeBack,
                           __global __write_only image2d_t, _output,
                           __global __read_only image3d_t, _textureAtlas,
                           __constant struct KernelConstants *_constants,
                           __global __read_only float *_transferFunction,
                           __global __read_only int *_tsp,
                           __global __read_only int *_brickList) {

  // Kernel should be launched in 2D with one work item per pixel
  int2 intCoords = (int2)(idx, idy);

  // Sampler for color cube reading
  const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;

  // Read from color cube textures
  float4 cubeFrontColor = read_imagef(_cubeFront, sampler, intCoords);
  float4 cubeBackColor = read_imagef(_cubeBack, sampler, intCoords);
  // Abort if both colors are black (missed the cube)
  if (length(cubeFrontColor.xyz) == 0.0 && length(cubeBackColor.xyz == 0.0)) {
    return;
  }

  // Figure out ray direction and distance to traverse
  float3 direction = cubeBackColor.xyz - cubeFrontColor.xyz;
  float maxDist = length(direction);
  direction = normalize(direction);

  float4 color = TraverseOctree(cubeFrontColor.xyz, // ray origin
                                direction,          // direction
                                maxDist,            // distance to traverse
                                _constants,         // kernel constants
                                _transferFunction,  // transfer function
                                _tsp,               // TSP tree struct
                                _brickList);        // brick list

  write_imagef(_output, intCoords, color*_constants->intensity_;

}
                                  




                              
