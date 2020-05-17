#pragma once

#include <QString>
#include <QObject>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>

class VolumeData 
{
public:
	using DataType = unsigned char;
	glm::int3 _nxyz;
	glm::float3 _sxyz;
	DataType* _data = nullptr;
	float _mean, _std, _min, _max;
	unsigned int* _histogram = nullptr;
	unsigned int _numBins;

	VolumeData();
	virtual ~VolumeData();

	virtual void init(int nx, int ny, int nz, float sx, float sy, float sz);
	virtual void computeHistogram(unsigned int numBins = 1024);
};