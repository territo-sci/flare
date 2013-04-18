__kernel void
Raycaster(__global __read_only image2d_t cubeFront,
				  __global __read_only image2d_t cubeBack,
					__global __write_only image2d_t output,
					__global __read_only float *voxelData) {
	
	// Kernel should be launched in 2D
	int idx = get_global_id(0);
	int idy = get_global_id(1);
	int2 intCoords = (int2)(idx, idy);

	float blah = voxelData[10];

	// Make a sampler to enable reading
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE;
	
	// Read from textures
	float4 cubeFrontColor = read_imagef(cubeFront, sampler, intCoords);
	float4 cubeBackColor = read_imagef(cubeBack, sampler, intCoords);
	
	// Output
	float4 color = cubeFrontColor - blah*cubeBackColor;

	// Write to image
	write_imagef(output, intCoords, color);
	
}

