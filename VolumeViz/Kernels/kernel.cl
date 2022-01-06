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
	float4 tempPixel;
	tempPixel.x = (float)(pixel.x - viewport.x) / (float)viewport.z;
	//tempPixel.y = (float)(((float)viewport.w - pixel.y) - viewport.y) / (float)viewport.w;
	tempPixel.y = (float)(pixel.y - viewport.y) / (float)viewport.w;
	tempPixel.z = 1.0f;
	tempPixel.w = 1.0f;
	//tempPixel.x = (tempPixel.x * 2.0f) - 1.0f;
	//tempPixel.y = (tempPixel.y * 2.0f) - 1.0f;

	tempPixel = tempPixel * 2.0f - 1.0f;

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
	// Uncomment in the case of a dn existing world to object matrix for the OOBB
	const float3 xAxis = (float3)(1.0f, 0.0f, 0.0f);
	const float3 yAxis = (float3)(0.0f, 1.0f, 0.0f);
	const float3 zAxis = (float3)(0.0f, 0.0f, 1.0f);
	const float3 delta = -ray.origin;
	const float3 e = (float3)(dot(xAxis, delta), dot(yAxis, delta), dot(zAxis, delta));
	const float3 f = 1.0f / (float3)(dot(ray.direction, xAxis), dot(ray.direction, yAxis), dot(ray.direction, zAxis));

	//const float3 invDIr = 1.0f / (ray.direction + 0.00000001f);
	//const float3 e = -ray.origin;
	//const float3 f = invDIr;

	const float3 t1a = (e + pMin) * f;
	const float3 t2a = (e + pMax) * f;

	const float3 tmin = fmin(t1a, t2a);
	const float3 tmax = fmax(t1a, t2a);

	*tNear = (fmax(fmax(tmin.x, tmin.y), tmin.z));
	*tFar = (fmin(fmin(tmax.x, tmax.y), tmax.z));

	return *tFar > *tNear;
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
const sampler_t tf_image_sampler = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;


__kernel void ssaoKernel(
	__write_only image2d_t occlusionMap,
	__read_only image2d_t depthMap,
	__read_only image2d_t opacityMap
) {
	const int2 pixelCoords = (int2)(get_global_id(0), get_global_id(1));

	const int width = get_image_width(occlusionMap);
	const int height = get_image_height(occlusionMap);

	if (pixelCoords.x >= width ||
		pixelCoords.y >= height)
		return;

	float depthValue = read_imagef(depthMap, map_image_sampler, pixelCoords).x;
	float opacityValue = read_imagef(opacityMap, map_image_sampler, pixelCoords).x;

	float occlusionCoeff = 1.0f;
	{
		// screen space ambient occlusion
		const int gap = 3;
		float larger = 0.0f, smaller = 0.0f;

		const float thresh = 0.4f;

		float sum = 0.0f;
		float weights = 1.0f;

		for (int i = -gap; i < gap; i++) {
			for (int j = -gap; j < gap; j++) {
				float2 sampleCoords = convert_float2(pixelCoords) + (float2)(i, j) * 1.25f;

				float depth = read_imagef(depthMap, map_image_sampler, sampleCoords).x;
				float opacity = read_imagef(opacityMap, map_image_sampler, sampleCoords).x;

				sum += depth * opacity;
				//weights += ;
			}
		}

		//float occlusionCoeff = pow((larger / smaller), 1.0f);
		float occlusionCoeff = pow(clamp((sum / (depthValue * opacityValue * gap * gap * 4.0f)), 0.0f, 1.0f), 4.0f);
		occlusionCoeff = clamp(occlusionCoeff, 0.0f, 1.0f);

		//write_imagef(output_texture, pixelCoords, opacityValue * (float4)(correctedNormalValue.xyz * occlusionCoeff, opacityValue));
		write_imagef(occlusionMap, pixelCoords, opacityValue * occlusionCoeff);
	}
}

__kernel void postProcessingKernel(
	__read_write image2d_t output_texture,
	__read_only image2d_t depthMap,
	__read_only image2d_t opacityMap,
	__read_only image2d_t colorMap,
	__read_only image2d_t normalMap,
	__read_only image2d_t densityMap,
	__read_only image2d_t positionMap,
	__read_only image2d_t occlusionMap,
	__read_only image1d_t tf_image
)
{
	const int2 pixelCoords = (int2)(get_global_id(0), get_global_id(1));

	const int width = get_image_width(output_texture);
	const int height = get_image_height(output_texture);

	if (pixelCoords.x >= width ||
		pixelCoords.y >= height)
		return;

	float bg_gradient = 1.0f - (float)pixelCoords.y / (float)height;
	float4 bg_color = (float4)(0.2f, 0.4f, 0.6f, 1.0f);

	float4 colorValue = read_imagef(colorMap, map_image_sampler, pixelCoords);
	float3 normalValue = read_imagef(normalMap, map_image_sampler, pixelCoords).xyz;

	float3 correctedNormalValue = normalValue * 0.5f + 0.5f;

	float depthValue = read_imagef(depthMap, map_image_sampler, pixelCoords).x;
	float opacityValue = read_imagef(opacityMap, map_image_sampler, pixelCoords).x;
	float densityValue = read_imagef(densityMap, map_image_sampler, pixelCoords).x;

	float4 colorAlt = read_imagef(tf_image, tf_image_sampler, densityValue);

	const float3 lightDir = normalize((float3)(0.3f, -1.5f, -10.0f)); // the direction of the light source
	//const float3 lightPosition = (float3)(0, 0, 0);
	//float3 lightDir = -normalize(lightPosition - intersectionPoint);
	float lighting = clamp((dot(lightDir, normalValue)), 0.135f, 1.0f); // perform a simplified shading operation

	float occlusionCoeff = 1.0f;
	{
		// screen space ambient occlusion
		const int gap = 1;
		float larger = 0.0f, smaller = 0.0f;

		float sum = 0.0f;
		float count = 0.0f;

		for (int i = -gap; i <= gap; i++) {
			for (int j = -gap; j <= gap; j++) {

				float2 sampleGap = (float2)(i, j) * 3.25f;
				float2 sampleCoords = convert_float2(pixelCoords) + sampleGap;

				float dist = 1.0f / (0.01f + pow(length(sampleGap), 2.0f));
				float occlusion = read_imagef(occlusionMap, map_image_sampler, sampleCoords).x;
				float opacity = read_imagef(opacityMap, map_image_sampler, sampleCoords).x;

				sum += occlusion * dist * opacity;
				count += 1.0f * dist * opacity;
			}
		}

		occlusionCoeff = sum / count;
		//write_imagef(output_texture, pixelCoords, opacityValue * (float4)(correctedNormalValue.xyz * occlusionCoeff, opacityValue));
		//write_imagef(output_texture, pixelCoords, opacityValue * occlusionCoeff);
	}

	//write_imagef(output_texture, pixelCoords, mix(bg_color*bg_gradient,  (float4)(occlusionCoeff * opacityValue * lighting * colorValue.xyz, opacityValue), opacityValue) );
	//write_imagef(output_texture, pixelCoords, mix(bg_color * bg_gradient, (float4)(correctedNormalValue, opacityValue), opacityValue));
	//write_imagef(output_texture, pixelCoords, mix(bg_color * bg_gradient, occlusionCoeff * (float4)(correctedNormalValue, opacityValue), opacityValue));
	write_imagef(output_texture, pixelCoords, opacityValue * occlusionCoeff);
	//write_imagef(output_texture, pixelCoords, (float4)(colorAlt.xyz, opacityValue));
}


void clearMaps(int2 pixelCoords,
	__write_only image2d_t depthMap,
	__write_only image2d_t opacityMap,
	__write_only image2d_t colorMap,
	__write_only image2d_t normalMap,
	__write_only image2d_t densityMap,
	__write_only image2d_t positionMap) {
	write_imagef(depthMap, pixelCoords, 0.0f);
	write_imagef(opacityMap, pixelCoords, 0.0f);
	write_imagef(colorMap, pixelCoords, 0.0f);
	write_imagef(normalMap, pixelCoords, 0.0f);
	write_imagef(densityMap, pixelCoords, 0.0f);
	write_imagef(positionMap, pixelCoords, 0.0f);
}

float3 computeNormal(float3 intersectionPoint,
	int2 pixelCoords,
	__read_only image1d_t tf_image,
	__read_only image3d_t volumeDataImage) {
	const float eps = 1.25f;

	// pseudo sobel operator
	float operatorValues[3][3] = {
		{1, 2, 1},
		{2, 4, 2},
		{1, 2, 1}
	};

	float values[3][3][3];

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				float3 offset = (float3)(i - 1, j - 1, k - 1);
				float4 samplePos = (float4)(intersectionPoint.xyz + offset * eps, 0.0f);
				float density = read_imagef(volumeDataImage, volume_image_sampler, samplePos).x;
				//float opacity = read_imagef(tf_image, tf_image_sampler, density).w;
				values[i][j][k] = density;
			}
		}
	}

	float gx = 0.0f, gy = 0.0f, gz = 0.0f;

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			gx += operatorValues[i][j] * (values[0][i][j] - values[2][i][j]);
			gy += operatorValues[i][j] * (values[i][0][j] - values[i][2][j]);
			gz += operatorValues[i][j] * (values[i][j][0] - values[i][j][2]);
		}
	}

	return normalize((float3)(gx, gy, gz));
}


// Progressive volume rendering kernel
__kernel void volumeRenderingKernelProgressiveAlt(
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
	__read_write image2d_t normalMap,
	__read_write image2d_t densityMap,
	__read_write image2d_t positionMap,
	int updateRequested)
{
	const int2 pixelCoords = (int2)(get_global_id(0), get_global_id(1));
	const int width = get_image_width(colorMap);
	const int height = get_image_height(colorMap);

	if (pixelCoords.x >= width ||
		pixelCoords.y >= height)
		return;

	float4 bg_color = (float)(pixelCoords.y) / (float)(height);

	if (updateRequested == 1) { // Clear all buffers
		clearMaps(pixelCoords, depthMap, opacityMap, colorMap, normalMap, densityMap, positionMap);
	}

	// Project the ray from the pixel towards the volume
	Ray ray = makeRay(eyePosition.xyz, pixelCoords, viewPort, invModelViewProjMatrix);

	// Check for the intersection
	float3 pMin = convert_float3(numCells) * (-0.5f);
	float3 pMax = convert_float3(numCells) * 0.5f;
	float tNear = FLT_MAX, tFar = FLT_MIN;

	if (rayBoxIntersection(pMin, pMax, ray, &tNear, &tFar)) {
		if (tNear < 0.0f) tNear = 0.0f;

		const float maxDepth = (tFar - tNear); // the total depth to travel throughout the volume
		const float maxOpacity = 0.95f; // the opacity threshold

		float depth = read_imagef(depthMap, pixelCoords).x;
		float accumDensity = read_imagef(densityMap, pixelCoords).x;

		float accumOpacity = read_imagef(opacityMap, pixelCoords).x;
		float4 accumColor = read_imagef(colorMap, pixelCoords);

		if (depth < maxDepth && accumOpacity < maxOpacity) {

			float3 intersectionPoint = clamp(pMax + ray.origin + ray.direction * (tNear + depth),
				(float3)(0.0f, 0.0f, 0.0f),
				convert_float3(numCells - 1));

			float3 deltaDist = (pMax - pMin);

			float largeDist = max(max(fabs(deltaDist.x), fabs(deltaDist.y)), fabs(deltaDist.z));

			float globalStepAcceleration = clamp((tNear / tFar), 0.25f, 1.0f);

			const float3 deltaStep3f = ((pMax - pMin) / largeDist) * ray.direction;
			const float deltaStep1f = length(deltaStep3f);

			while (true) {
				// read the density at the current intersectionPoint
				float density = read_imagef(volumeDataImage,
					volume_image_sampler,
					(float4)(intersectionPoint.xyz, 0.0f)).x;

				//density = (density - min_max_values.x) / (min_max_values.y - min_max_values.x);

				// compute the corresponding color in the transfer function for the current density
				float4 color = read_imagef(tf_image, tf_image_sampler, density);
				float opacity = color.w;

				float stepAcceleration = 2.25f;

				if (opacity > 0.001f) {

					float stepsCount = 1.0f / (0.000001f + pow(opacity, 128.0f));

					float3 nextIntersectionPoint = ceil(intersectionPoint + ray.direction * stepAcceleration);

					float segmentLength = pow(length(nextIntersectionPoint - intersectionPoint), 2.0f);

					stepAcceleration = segmentLength;

					float oneMinusOpacity = 1.0f - opacity;
					float oneMinusOpacityPowerN = pow(oneMinusOpacity, stepsCount);

					accumDensity = accumDensity * oneMinusOpacityPowerN + density * (1.0f - oneMinusOpacityPowerN);

					accumColor = accumColor * oneMinusOpacityPowerN + color * (1.0f - oneMinusOpacityPowerN);
					accumOpacity = accumOpacity * oneMinusOpacityPowerN + (1.0f - oneMinusOpacityPowerN);

					// stop the raymarching if the pixel is fully opaque
					// this technique is called "Early ray termination"
					if (accumOpacity >= maxOpacity) {
						break;
					}
				}

				depth += deltaStep1f * (stepAcceleration)*globalStepAcceleration;

				if (depth >= maxDepth) {
					break;
				}

				intersectionPoint += deltaStep3f * (stepAcceleration)*globalStepAcceleration;
			}

			write_imagef(positionMap, pixelCoords, (float4)(intersectionPoint, 0.0f)); // write the current position to the positionMap			
			write_imagef(depthMap, pixelCoords, depth / maxDepth); // write the current depth to the depthMap
			write_imagef(densityMap, pixelCoords, accumDensity); // write the current density to the densityMap
			write_imagef(opacityMap, pixelCoords, accumOpacity); // write the current opacity to the opacityMap
			write_imagef(colorMap, pixelCoords, (float4)(accumColor.xyz, accumOpacity)); // write the current color to the colorMap

			float3 normal = computeNormal(intersectionPoint, pixelCoords, tf_image, volumeDataImage);
			write_imagef(normalMap, pixelCoords, (float4)(normal, 0.0f)); // write the computed normal vector to the normalMap
		}
	}
	else { // no intersection found
		clearMaps(pixelCoords, depthMap, opacityMap, colorMap, normalMap, densityMap, positionMap);
	}
}