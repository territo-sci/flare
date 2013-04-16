__kernel void
Raycaster(//global read_only image2d_t cubeFront,
					//global read_only image2d_t cubeBack,
					global write_only image2d_t output) {
	
	// Kernel should be launched in 2D
	int idx = get_global_id(0);
	int idy = get_global_id(1);
	int2 intCoords = (int2)(idx, idy);
	//float2 coords = (float2)(idx, idy);

	// Output
	float4 color = (float4)(0.0, 1.0, 0.0, 0.0);

	// Sample the front and back textures

	// Screw that, just make everything green
	write_imagef(output, intCoords, color);
	
}

