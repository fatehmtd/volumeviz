// Ray related utilities
typedef struct
{
	float3 origin, direction;
} Ray;

// Volume related utilities
typedef struct
{
	float3 dims;
	int3 ncells;
}   VolumeInfo;

// Transfer function related utilities
typedef struct
{
	float value;
	float4 rgba;
} TFEntry;

///////////////////////////////////////////////////////////////////////////////////////////////////////

float4 mult(__constant const float4* matrix, const float4 vec)
{
	float4 output;
	output.x = matrix[0].x * vec.x + matrix[1].x * vec.y + matrix[2].x * vec.z + matrix[3].x * vec.w;
	output.y = matrix[0].y * vec.x + matrix[1].y * vec.y + matrix[2].y * vec.z + matrix[3].y * vec.w;
	output.z = matrix[0].z * vec.x + matrix[1].z * vec.y + matrix[2].z * vec.z + matrix[3].z * vec.w;
	output.w = matrix[0].w * vec.x + matrix[1].w * vec.y + matrix[2].w * vec.z + matrix[3].w * vec.w;
	return output;
}

Ray makeRay(float3 position, int2 pixel, int4 viewport, __constant const float4* invModelViewProjMatrix)
{
	/*
	float4 tempPixel;
	tempPixel.x = (float)(pixel.x - viewport.x) / (float)viewport.z;
	tempPixel.y = (float)(pixel.y - viewport.y) / (float)viewport.w;
	tempPixel.z = 1.0f;
	tempPixel.w = 1.0f;
	tempPixel = tempPixel * 2.0f - 1.0f;
	//*/
	
	float4 tempPixel = (float4)(
		(float)(pixel.x - viewport.x) / (float)viewport.z,
		(float)(pixel.y - viewport.y) / (float)viewport.w,
		1.0f,
		1.0f) * 2.0f - 1.0f;
	//*/
	float4 projectedPosition = mult(invModelViewProjMatrix, tempPixel);
	projectedPosition /= projectedPosition.w;

	Ray ray;
	ray.origin = position;
	ray.direction = normalize(projectedPosition.xyz - position);
	return ray;
}

int3 clampVolume(const int3 index, const int3 dims)
{
	return clamp(index, (int3) (0, 0, 0), (int3)(dims.x - 1, dims.y - 1, dims.z - 1));
}

float bilinear(float a, float b,
	float c, float d,
	float alpha, float beta)
{
	return mix(mix(a, b, alpha), mix(c, d, alpha), beta);
}

float trilinear(float a, float b, float c, float d,
	float e, float f, float g, float h,
	float alpha, float beta, float gamma)
{
	return mix(bilinear(a, b, c, d, alpha, beta), bilinear(e, f, g, h, alpha, beta), gamma);
}

float volumeValue(int3 index, __global float* volume, int3 dims)
{
	const int3 idx = clamp(index, 0, dims - 1);
	//return volume[idx.y + dims.y * (idx.x + idx.z * dims.x)];
	return volume[idx.z + dims.z * (idx.y + idx.x * dims.x)];
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
	const float3 f = 1.0f/(float3)(dot(ray.direction, xAxis), dot(ray.direction, yAxis), dot(ray.direction, zAxis));

	// lack of matrix for the OOBB
	//const float3 invDIr = 1.0f / (ray.direction + 0.000001f);
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

float getDensity(__global float* volumeData, int3 dimensions, float3 position)
{
	float3 temp;
	float3 fracPart = modf(position, &temp); // the fractional part, used for interpolation
	int3 index = convert_int3(temp); // the integer part, used as an index

	const int3 dx = (int3)(1, 0, 0);
	const int3 dy = (int3)(0, 1, 0);
	const int3 dz = (int3)(0, 0, 1);

	// compute neighborhood values
	const float density000 = volumeValue(index, volumeData, dimensions);
	//return density000;

	const float density100 = volumeValue(index + dx, volumeData, dimensions);
	const float density010 = volumeValue(index + dy, volumeData, dimensions);
	const float density110 = volumeValue(index + dx + dy, volumeData, dimensions);
	const float density001 = volumeValue(index + dz, volumeData, dimensions);
	const float density101 = volumeValue(index + dz + dx, volumeData, dimensions);
	const float density011 = volumeValue(index + dz + dy, volumeData, dimensions);
	const float density111 = volumeValue(index + dz + dx + dy, volumeData, dimensions);

	return (trilinear(
		density000, density100, density010, density110,
		density001, density101, density011, density111,
		fracPart.x, fracPart.y, fracPart.z));
}

// Volume rendering kernel
__kernel void volumeRenderingKernel(
	__write_only image2d_t output_texture,
	__constant const float4* invModelViewProjMatrix,
	float4 eyePosition,
	int4 viewPort
)
{
	const int2 pixelCoords = (int2)(get_global_id(0), get_global_id(1));

	const int width = get_image_width(output_texture);
	const int height = get_image_height(output_texture);

	if (pixelCoords.x >= width ||
		pixelCoords.y >= height)
		return;

	// Project the ray from the pixel towards the volume
	Ray ray = makeRay(eyePosition.xyz, pixelCoords, viewPort, invModelViewProjMatrix);

	// Check for the intersection
	float size = 100.0f;
	float3 pMin = (float3)(-1, -1, -1) * size;
	float3 pMax = (float3)(1, 1, 1) * size;
	float tNear = FLT_MAX, tFar = FLT_MIN;

	if (rayBoxIntersection(pMin, pMax, ray, &tNear, &tFar))
	{
		write_imagef(output_texture, pixelCoords, 1);
	}
	else
	{
		write_imagef(output_texture, pixelCoords, (float)(pixelCoords.y)/(float)(height));
	}
}