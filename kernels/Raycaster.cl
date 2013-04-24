// Turn normalized [0..1] coordinates into array index 
int CoordsToIndex(float3 _coordinates, 
								  int3 _dimensions) {
	// Put coords in [0 .. dim-1] range
	int x = (float)(_dimensions.x-1) * _coordinates.x;
	int y = (float)(_dimensions.y-1) * _coordinates.y;
	int z = (float)(_dimensions.z-1) * _coordinates.z;
  // Return index
	return x + y*_dimensions.x + z*_dimensions.x*_dimensions.y;
}

// Turn normalized [0..1] cartesian coordinates 
// to normalized spherical [0..1] coordinates
float3 CartesianToSpherical(float3 _cartesian) {
	// Put cartesian in [-1..1] range
	// r from [0..sqrt3] to [0..1] range
	// theta from [0..PI] to [0..1] range
	// phi from [-PI..PI] to [0..1] range
	_cartesian = (float3)(-1.0) + 2.0* _cartesian;
	float r = length(_cartesian);
	float theta, phi;
	if (r == 0.0) {
		theta = phi = 0.0;
	} else {
		theta = acos(_cartesian.z/r) / M_PI;
		phi = (M_PI + atan2(_cartesian.y, _cartesian.x)) / (2.0*M_PI);
	}
	r = r / sqrt(3.0);
	return (float3)(r, theta, phi);
}


__kernel void
Raycaster(__global __read_only image2d_t cubeFront,
				  __global __read_only image2d_t cubeBack,
					__global __write_only image2d_t output,
					__global __read_only float *voxelData,
					int timestepOffset) {

	int3 dimensions = (int3)(128, 128, 128);
	
	// Kernel should be launched in 2D with one work item per pixel
	int idx = get_global_id(0);
	int idy = get_global_id(1);
	int2 intCoords = (int2)(idx, idy);

	// Make a sampler to enable reading
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;
	
	// Read from textures
	float4 cubeFrontColor = read_imagef(cubeFront, sampler, intCoords);
	float4 cubeBackColor = read_imagef(cubeBack, sampler, intCoords);

	// Figure out the direction
	float3 direction = (cubeBackColor-cubeFrontColor).xyz;
	float maxDistance = length(direction);
	direction = normalize(direction);

	// Keep track of distance traversed
	float traversed = 0.0;

	// Sum colors
	float stepSize = 0.01;
	float3 samplePoint = cubeFrontColor.xyz;
	float3 spherical;
	float intensity = 0.0;
	while (traversed < maxDistance) 
	{
		spherical = CartesianToSpherical(samplePoint);
		int index = timestepOffset + CoordsToIndex(spherical, dimensions);
		intensity += voxelData[index];
		samplePoint += direction * stepSize;
		traversed += stepSize;
	}

	// Output
	float4 color = 50.0  * stepSize * (float4)(intensity, intensity, intensity, 1.0);

	// Write to image
	write_imagef(output, intCoords, color);
	
}

