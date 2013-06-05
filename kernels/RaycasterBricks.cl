// Needs to mirror struct on host
struct KernelConstants {
  float stepSize;
  float intensity;
  int aDim;
  int bDim;
  int cDim;
};

struct BrickConfig {
  float numBricksPerAxis;
  float brickDim;
};

// Linearly interpolate between two values. Distance
// is assumed to be normalized.
float Lerp(float _v0, float _v1, float _d) {
  return _v0*(1.0 - _d) + _v1*_d;
}


// Turn normalized [0..1] cartesian coordinates 
// to normalized spherical [0..1] coordinates
float3 CartesianToSpherical(float3 _cartesian) {
  // Put cartesian in [-1..1] range first
  _cartesian = (float3)(-1.0) + 2.0* _cartesian;
  float r = length(_cartesian);
  float theta, phi;
  if (r == 0.0) {
    theta = phi = 0.0;
  } else {
    theta = acospi(_cartesian.z/r);
    phi = (M_PI + atan2(_cartesian.y, _cartesian.x)) / (2.0*M_PI);
  }
  r = r / native_sqrt(3.0);
  // Sampler ignores w component
  return (float3)(r, theta, phi);
}

float4 TransferFunction(__global __read_only float *_tf, float _i) {
  // TODO remove  hard-coded value and change to 1D texture
  int i0 = (int)floor(1023.0*_i);
  int i1 = (i0 < 1023) ? i0+1 : i0;
  float di = _i - floor(_i);
  
  float tfr0 = _tf[i0*4+0];
  float tfr1 = _tf[i1*4+0];
  float tfg0 = _tf[i0*4+1];
  float tfg1 = _tf[i1*4+1];
  float tfb0 = _tf[i0*4+2];
  float tfb1 = _tf[i1*4+2];
  float tfa0 = _tf[i0*4+3];
  float tfa1 = _tf[i1*4+3];

  float tfr = Lerp(tfr0, tfr1, di);
  float tfg = Lerp(tfg0, tfg1, di);
  float tfb = Lerp(tfb0, tfb1, di);
  float tfa = Lerp(tfa0, tfa1, di);

  return (float4)(tfr, tfg, tfb, tfa);
}

float4 FindBrickCoords(float4 _spherical, float _numBricksPerAxis) {
  return floor(_spherical*_numBricksPerAxis);
}

float4 TraverseBrick(float4 _globalCoords, // global, normalized, spherical
                     float4 &_traversed,
                     __constant struct BrickConfig _bc,
                     __global __read_only float *_tf,
                     float4 _globalStepSize, 

  float numBricksPerAxis = _bc->numBricksPerAxis;
  float brickDim = _bc->brickDim;

  // Find the brick coordinates [0..numBricksPerAxis-1]
  float4 brickCoords = floor(_globalCoords*numBricksPerAxis);

 
  // Find the linear index in brick atlas
  int brickIndex = (int)(brickCoords.x +
                         brickCoords.y * numBricksPerAxis +
                         brickCoords.z * pow(numBricksPerAxis, 2.0));
  
  // Find the local coordinates in brick (unnormalized) [0..brickDim-1]
  float4 localCoords = brickDim * 
                       (_globalCoords-(brickCoords/numBricksPerAxis)) * 
                       numBricksPerAxis;

  // Atlas coordinates (bricks laid out along X axis)
  float4 atlasCoords = localCoords + 
                       (float4)(brickDim*brickIndex, 0.0, 0.0, 0.0);

  // Traverse 
  float4 atlasSamplePos = atlasCoords;
  float4 originalSample = atlasSamplePos;
  float atlasStepSize = _globalStepSize * brickDim;
  float4 atlasDirection = _globalDirection * brickDim;
  bool inside = true;
  float4 color = (float4)(0.0, 0.0, 0.0, 0.0);

  while (inside) {

    float i = read_imagef(_textureAtlas, atlasSampler, atlasSamplePos);
    float4 tf = Transferfunction(_tf, i);
    color += (1.0 - color.w)*tf;
    atlasSamplePos += atlasDirection * atlasStepSize;

    inside = ( atlasSamplePos.z > 0.0 && atlasSamplePos.z < _brickSize &&
               atlasSamplePos.y > 0.0 && atlasSamplePos.y < _brickSize &&
               atlasSamplePos.x > brickIndex * brickDim &&
               atlasSamplePos.x < (brickIndex+1) * brickDim );
  }

  _traversed = length(atlasSamplePos - originalSample)/brickDim;
 
  return color;
}



__kernel void
Raycaster(__global __read_only image2d_t _cubeFront,
          __global __read_only image2d_t _cubeBack,
          __global __write_only image2d_t _output,
          __global __read_only image3d_t _textureAtlas,
          __constant struct KernelConstants *_constants,
          __constant struct BrickConfig *_brickConfig,
          __global __read_only float *_tf) {

  int3 dimensions = (int3)(_constants->aDim,
                           _constants->bDim,
                           _constants->cDim);
  
  // Kernel should be launched in 2D with one work item per pixel
  int idx = get_global_id(0);
  int idy = get_global_id(1);
  int2 intCoords = (int2)(idx, idy);

  // Sampler for color cube texture reading
  const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;
    
  // Read from textures
  float4 cubeFrontColor = read_imagef(_cubeFront, sampler, intCoords);
  float4 cubeBackColor = read_imagef(_cubeBack, sampler, intCoords);

  // Figure out the ray starting point and direction
  float stepSize = _constants->stepSize;
  float4 samplePoint = cubeFrontColor; // Fourth element will be unused
  float3 direction = (cubeBackColor-cubeFrontColor).xyz;
  float maxDistance = length(direction);
  direction = normalize(direction);

  // Keep track of traversed distance
  float traversed = 0.0;

  // Cumulative color
  float4 color = (float4)(0.0, 0.0, 0.0, 0.0);

  // Loop until we have covered the distance
  while (traversed < maxDistance) {
  
    float4 spherical = CartesianToSpherical(samplePoint);
    
    // Figure out which brick to traverse
    int3 brickCoords = FindBrickCoords(spherical, numBricksPerAxis);

    // Traverse brick, get resulting color and update distance
    float traversedBrick;
    color += TraverseBrick(samplePoint, 
                           traversedBrick,
                           _brickConfig,
                           _tf,
                           stepSize);

    // Update traversed distance
    traversed += traversedBrick;

  }

  // Output
  color *= _constants->intensity*stepSize;

  // Write to image
  write_imagef(_output, intCoords, color);

}

