// Ray related utilities

float4 mult(__constant const float4* matrix, const float4 vec)
{
	float4 output;
	output.x = matrix[0].x * vec.x + matrix[1].x * vec.y + matrix[2].x * vec.z + matrix[3].x * vec.w;
	output.y = matrix[0].y * vec.x + matrix[1].y * vec.y + matrix[2].y * vec.z + matrix[3].y * vec.w;
	output.z = matrix[0].z * vec.x + matrix[1].z * vec.y + matrix[2].z * vec.z + matrix[3].z * vec.w;
	output.w = matrix[0].w * vec.x + matrix[1].w * vec.y + matrix[2].w * vec.z + matrix[3].w * vec.w;
	return output;
}

typedef struct
{
	float3 origin, direction;
} Ray;

Ray makeRay(float3 position, int2 pixel, int4 viewport, __constant const float4* invModelViewProjMatrix)
{
	float4 tempPixel = (float4)(
		(float)(pixel.x - viewport.x) / (float)viewport.z,
		(float)(pixel.y - viewport.y) / (float)viewport.w,
		1.0f,
		1.0f) * 2.0f - 1.0f;
	float4 projectedPosition = mult(invModelViewProjMatrix, tempPixel);
	projectedPosition /= projectedPosition.w;

	Ray ray;
	ray.origin = position;
	ray.direction = normalize(projectedPosition.xyz - position);
	return ray;
}


// Intersections
bool rayBoxIntersection(float3 pMin, float3 pMax, Ray ray, float* tNear, float* tFar)
{
	const float3 invDIr = 1.0f / (ray.direction + 0.01f);
	const float3 e = -ray.origin;
	const float3 f = invDIr;

	const float3 t1a = (e + pMin) * f;
	const float3 t2a = (e + pMax) * f;

	const float3 tmin = fmin(t1a, t2a);
	const float3 tmax = fmax(t1a, t2a);

	*tNear = (fmax(fmax(tmin.x, tmin.y), tmin.z));
	*tFar = (fmin(fmin(tmax.x, tmax.y), tmax.z));

	return *tFar > * tNear;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

// Volume sampling 
int3 clampVolume(const int3 index, const int3 dims)
{
	return clamp(index, (int3)(0, 0, 0), (int3)(dims.x - 1, dims.y - 1, dims.z - 1));
}

// Texture samplers
const sampler_t volume_image_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
const sampler_t map_image_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;
const sampler_t tf_image_sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

// Progressive volume rendering kernel
__kernel void volumeRenderingKernelProgressive(
	__constant const float4* invModelViewProjMatrix,
	float4 eyePosition,
	int4 viewPort,
	__read_only image3d_t volumeDataImage,
	int3 numCells,
	float3 cellDims,
	float2 min_max_values,
	__read_only image1d_t tf_image,
	__read_write image2d_t depthMap,
	__read_write image2d_t opacityMap,
	__read_write image2d_t colorMap,
	int updateRequested)
{
	const int2 pixelCoords = (int2)(get_global_id(0), get_global_id(1));
	const int width = get_image_width(colorMap);
	const int height = get_image_height(colorMap);

	if (pixelCoords.x >= width ||
		pixelCoords.y >= height)
		return;

	float4 bg_color = (float)(pixelCoords.y) / (float)(height);

	if (updateRequested == 1) // Clear all buffers
	{
		write_imagef(depthMap, pixelCoords, 0.0f);
		write_imagef(opacityMap, pixelCoords, 0.0f);
		write_imagef(colorMap, pixelCoords, 0.0f);
	}

	// Project the ray from the pixel towards the volume
	Ray ray = makeRay(eyePosition.xyz, pixelCoords, viewPort, invModelViewProjMatrix);

	// Check for the intersection
	float3 pMin = convert_float3(numCells) * (-0.5f);
	float3 pMax = convert_float3(numCells) * 0.5f;
	float tNear = FLT_MAX, tFar = FLT_MIN;

	if (rayBoxIntersection(pMin, pMax, ray, &tNear, &tFar))
	{
		if (tNear < 0.0f) tNear = 0.0f;

		const float maxDepth = (tFar - tNear); // the total depth to travel throughout the volume
		const float maxOpacity = 0.95f; // the opacity threshold

		float depth = read_imagef(depthMap, pixelCoords).x;
		float accumOpacity = read_imagef(opacityMap, pixelCoords).x;
		float4 accumColor = read_imagef(colorMap, pixelCoords);

		if (depth < maxDepth && accumOpacity < maxOpacity)
		{
			float deltaStep = 0.0f; // the step with which the ray advances

			float3 intersectionPoint = clamp(pMax + ray.origin + ray.direction * (tNear + depth),
				(float3)(0.0f, 0.0f, 0.0f),
				convert_float3(numCells - 1));

			const float minStepSize = 0.01f; // the minimal step size to advance the ray
			const float maxStepSize = 0.1210f; // the maximal step size to advance the ray
			const int maxIterations = 2500; // the maximal number of iterations to perform in one single frame
											// decrease the number of iterations to get more fps, volume rendering update will be more noticeable
											// incrse the number of iterations to get better visual quality but low fps


			int cIteration = 0; // the current iteration count

			while (true)
			{
				// read the density at the current intersectionPoint
				float density = read_imagef(volumeDataImage,
					volume_image_sampler,
					(float4)(intersectionPoint.xyz, 0.0f)).x;

				// compute the corresponding color in the transfer function for the current density
				float4 color = read_imagef(tf_image, tf_image_sampler, density);
				float opacity = color.w;

				// forward alpha-compositing function
				accumColor = (1.0f - opacity) * accumColor + opacity * color;
				accumOpacity = accumOpacity + (1.0f - accumOpacity) * opacity;

				// stop the raymarching if the pixel is fully opaque
				// this technique is called "Early ray termination"
				if (accumOpacity >= maxOpacity)
				{
					break;
				}

				// compute an adaptive deltaStep size, the ray advances with a larger step if 
				// 
				const float t = 1.0f / (0.1f + exp(-opacity));
				deltaStep = mix(minStepSize, maxStepSize, clamp(0.0f, 1.0f, t));

				// compute the next intersection point by advancing deltaStep in the ray direction
				intersectionPoint += ray.direction * deltaStep;

				// increase the traversal depth by deltaStep
				depth += deltaStep;

				// stop the raymarching once the ray exists the volume
				if (depth >= maxDepth)
				{
					break;
				}

				// advance the current iteration number
				cIteration += 1;

				// stop if the number if iterations exceeds the given threshold
				// in order to avoid slowing down the rendering
				if (cIteration > maxIterations)
					break;
			}

			// compute lighting only when necessary, once the pixel is opaque
			bool computeLighting = accumOpacity > 0.11f;

			// lighting
			if (computeLighting) // lighting
			{
				const float3 lightDir = normalize((float3)(0.3f, 1.5f, 1.0f)); // the direction of the light source

				const float eps = 5.10f;

				// compute the lighting		
				const float3 dx = (float3)(1.0f, 0.0f, 0.0f) * eps;
				const float3 dy = (float3)(0.0f, 1.0f, 0.0f) * eps;
				const float3 dz = (float3)(0.0f, 0.0f, 1.0f) * eps;

				const float value = read_imagef(volumeDataImage, volume_image_sampler, (float4)(intersectionPoint.xyz, 0.0f)).x;

				float gxNext = read_imagef(volumeDataImage, volume_image_sampler, (float4)(intersectionPoint.xyz + dx, 0.0f)).x;
				float gyNext = read_imagef(volumeDataImage, volume_image_sampler, (float4)(intersectionPoint.xyz + dy, 0.0f)).x;
				float gzNext = read_imagef(volumeDataImage, volume_image_sampler, (float4)(intersectionPoint.xyz + dz, 0.0f)).x;
				float gxPrev = read_imagef(volumeDataImage, volume_image_sampler, (float4)(intersectionPoint.xyz - dx, 0.0f)).x;
				float gyPrev = read_imagef(volumeDataImage, volume_image_sampler, (float4)(intersectionPoint.xyz - dy, 0.0f)).x;
				float gzPrev = read_imagef(volumeDataImage, volume_image_sampler, (float4)(intersectionPoint.xyz - dz, 0.0f)).x;

				float3 normal = (float3)(gxNext, gyNext, gzNext) - (float3)(gxPrev, gyPrev, gzPrev);
				normal /= 2.0f * eps;
				/*
				if (length(normal) < 0.13f)
				{
					normal = (float3)(1.0f, 1.0f, 1.0f);
					normal = lightDir;
				}
				//*/
				//float3 normal = (float3)(gxNext, gyNext, gzNext) - value;
				//float3 normal =-( (float3)(gxNext + gxPrev, gyNext + gyPrev, gzNext + gzPrev) - 2.0f * value);
				normal = normalize(normal);
				accumColor = accumColor * clamp((dot(lightDir, normal)), 0.35f, 1.0f); // perform a simplified shading operation
			}
			//*/

			write_imagef(depthMap, pixelCoords, depth); // write the current depth to the dempthMap
			write_imagef(opacityMap, pixelCoords, accumOpacity); // write the current opacity to the opacityMap
			write_imagef(colorMap, pixelCoords, (float4)(accumColor.xyz, accumOpacity)); // write the current color to the colorMap
		}
	}
	else // no intersection found
	{
		write_imagef(opacityMap, pixelCoords, 0.0f);
	}
}

__kernel void postProcessingKernel(
	__read_write image2d_t output_texture,
	__read_only image2d_t depthMap,
	__read_only image2d_t opacityMap,
	__read_only image2d_t colorMap)
{
	const int2 pixelCoords = (int2)(get_global_id(0), get_global_id(1));

	const int width = get_image_width(output_texture);
	const int height = get_image_height(output_texture);

	if (pixelCoords.x >= width ||
		pixelCoords.y >= height)
		return;

	float bg_gradient = 1.0f-(float)pixelCoords.y / (float)height;
	float4 bg_color = (float4)(0.2f, 0.4f, 0.6f, 1.0f);
	float4 finalColor = read_imagef(colorMap, pixelCoords);
	write_imagef(output_texture, pixelCoords, mix(bg_color*bg_gradient, finalColor, finalColor.w));
}