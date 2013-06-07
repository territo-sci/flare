// Needs to mirror struct on host
struct KernelConstants {
  float stepSize;
  float intensity;
  int aDim;
  int bDim;
  int cDim;
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

// Translates a global volume coordinate [0..1] to a box coordinate,
// given a number of boxes that fit along each axis
int3 BoxCoords(float3 _globalCoords, int _boxesPerAxis) {
  int3 boxCoords = convert_int3(floor(_globalCoords * (float)_boxesPerAxis));
  return clamp(boxCoords, (int3)(0, 0, 0), (int3)(_boxesPerAxis-1));
}

// Calculate global volume coordinates for the corners of a box, given
// the box's coordinates [0..NumBoxesPerAxis] and the number of boxes
// that fit along each axis
void BoxCorners(int3 _boxCoords, float3 *_minCorner, float3 *_maxCorner,
                int _boxesPerAxis) {
  *_minCorner = convert_float3(_boxCoords) / (float)_boxesPerAxis;
  *_maxCorner = convert_float3((_boxCoords+1)) / (float)_boxesPerAxis;
}


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

float4 TraverseBrick(__global __read_only image3d_t _textureAtlas,
                     __global __read_only float *_tf,
                     int3 _brickCoords, int _brickSize, 
                     float _globalStepSize, float3 _brickEntry, 
                     float3 _brickExit) {
  
  // Number of boxes of this size for each axix
  int numBoxesPerAxis = 8/_brickSize;
  // Find brick box coordinates [0 .. NumBricksPerAxis]
  int3 brickCoords = BoxCoords (_brickEntry, numBoxesPerAxis);

  // Find local sample coordinates [0 .. 1]
  float3 localEntryCoords = (float)numBoxesPerAxis *
    (_brickEntry - convert_float3(brickCoords/numBoxesPerAxis));
  float3 localExitCoords = (float)numBoxesPerAxis *
  (_brickExit - convert_float3(brickCoords/numBoxesPerAxis));

  // Translate local brick coordinates to texture atlas coordinates
  float3 offset = convert_float3(_brickCoords)/(float)8;
  float3 atlasEntry = localEntryCoords/(float)8 + offset;
  float3 atlasExit = localExitCoords/(float)8 + offset;

  float3 direction = atlasExit - atlasEntry;
  float maxDistance = length(direction);
  direction = normalize(direction);

  float localStepSize = _globalStepSize * (float)_brickSize;
  float4 color = (float4)(0.0, 0.0, 0.0, 0.0);
  float traversed = 0.0;
  float3 samplePoint = atlasEntry;
  float4 spherical;
  const sampler_t atlasSampler = CLK_FILTER_LINEAR | 
                                 CLK_NORMALIZED_COORDS_TRUE |
                                 CLK_ADDRESS_CLAMP_TO_EDGE;

  while (traversed < maxDistance) {
    spherical.xyz = CartesianToSpherical(samplePoint);
    spherical.w = 1.0;
    
    // Texture stores intensity in R channel
    float i = read_imagef(_textureAtlas, atlasSampler, spherical).x;

    // Front-to-back compositing
    float4 tf = TransferFunction(_tf, i);
    if (samplePoint.y < 0.5) {
      color += spherical.x*0.001+0.000001*tf.x;
    }
    //color += (1.0 - color.w)*tf;

    traversed += localStepSize;
    samplePoint += localStepSize * direction;

  }

  return color;
}


__kernel void
Raycaster(__global __read_only image2d_t _cubeFront,
          __global __read_only image2d_t _cubeBack,
          __global __write_only image2d_t _output,
          __global __read_only image3d_t _textureAtlas,
          __constant struct KernelConstants *_constants,
          __global __read_only float *_tf) {


  int3 dimensions = (int3)(_constants->aDim,
                           _constants->bDim,
                           _constants->cDim);
  
  // Kernel should be launched in 2D with one work item per pixel
  int idx = get_global_id(0);
  int idy = get_global_id(1);
  int2 intCoords = (int2)(idx, idy);

  // Sampler for texture reading
  const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;
    
  // Read from textures
  float4 cubeFrontColor = read_imagef(_cubeFront, sampler, intCoords);
  float4 cubeBackColor = read_imagef(_cubeBack, sampler, intCoords);

  // Figure out the direction
  float3 direction = (cubeBackColor-cubeFrontColor).xyz;
  float maxDistance = length(direction);
  direction = normalize(direction);
  float3 origin = cubeFrontColor.xyz - 0.001 * direction;

  // Keep track of distance traversed
  float traversed = 0.0;

  // Sum colors
  float stepSize = _constants->stepSize;
  float3 samplePoint = cubeFrontColor.xyz;
  float4 spherical;
  float4 color = (float4)(0.0, 0.0, 0.0, 0.0);
   
  while (traversed < maxDistance) {
    
    // Convert the sample point to coords
    int3 boxCoords = BoxCoords(samplePoint, 8);
    // Calculate BRICK coords (in atlas)
    int3 brickCoords = boxCoords;

    // TODO lookup brick and brick size
    int brickSize = 1;

    // Calculate the box's corners
    float3 minCorner, maxCorner;
    BoxCorners(boxCoords, &minCorner, &maxCorner, 8);

    // Intersect ray with box
    float tMin, tMax;
    IntersectBox(minCorner, maxCorner, origin, direction, &tMin, &tMax);
    float3 brickEntry = origin + tMin*direction;
    // TODO calculate BRICK exit
    float3 brickExit = origin + tMax*direction;
    float brickDist = length(brickExit - brickEntry);

    // Traverse brick
    float4 brickColor = TraverseBrick(_textureAtlas, _tf, 
                                       brickCoords, brickSize, stepSize,
                                       brickEntry, brickExit);

    // Compositing
    color += (1.0 - brickColor.w)*brickColor;
                  
    // Advance ray
    brickDist += 0.000000001;
    samplePoint = brickEntry + direction * brickDist;
    traversed += brickDist;         
    
    //  color += (float4)(i, i, i, 1.0);
    samplePoint += direction * stepSize;
    traversed += stepSize;
  }

  // Output
  float intensity = _constants->intensity;
  color *= intensity*stepSize;

  // Write to image
  write_imagef(_output, intCoords, color);
  
}

