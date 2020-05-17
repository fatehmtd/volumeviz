#include "VolumeData.h"
#include <QDebug>

VolumeData::VolumeData()
{
	_data = nullptr;
	_histogram = nullptr;
}

VolumeData::~VolumeData()
{
	if (_data != nullptr)
		delete[] _data;

	if (_histogram != nullptr)
		delete[] _histogram;
}

void VolumeData::init(int nx, int ny, int nz, float sx, float sy, float sz)
{
	if (_data != nullptr)
		delete[] _data;

	_nxyz = glm::int3(nx, ny, nz);
	_sxyz = glm::float3(sx, sy, sz);

	_data = new DataType[nx * ny * nz];
	memset(_data, 0, sizeof(VolumeData::DataType) * nx * ny * nz);
}

void VolumeData::computeHistogram(unsigned int numBins)
{
	if (_histogram != nullptr)
		delete[] _histogram;

	_mean = _std = 0.0f;
	_min = std::numeric_limits<float>::max();
	_max = -_min;

	for (int w = 0; w < _nxyz.z; w++)
	{
		const auto* plane = &_data[_nxyz.x*_nxyz.y * w];

		for (int u = 0; u < _nxyz.y; u++)
		{
			for (int v = 0; v < _nxyz.x; v++)
			{
				auto value = plane[_nxyz.x * u + v];
				_mean += value;

				if (value < _min)
					_min = value;

				if (value > _max)
					_max = value;
			}
		}
	}

	_mean /= (_nxyz.x * _nxyz.y * _nxyz.z);

	_numBins = numBins;
	_histogram = new unsigned int[_numBins];
	memset(_histogram, 0, sizeof(unsigned int) * _numBins);

	float deltaBin = (_max - _min);

	for (int w = 0; w < _nxyz.z; w++)
	{
		const auto* plane = &_data[_nxyz.x * _nxyz.y * w];

		for (int u = 0; u < _nxyz.y; u++)
		{
			for (int v = 0; v < _nxyz.x; v++)
			{
				float value = plane[_nxyz.x * u + v];
				++_histogram[unsigned int((float)_numBins * (value - _min) / deltaBin)];
				_std += (value - _mean) * (value - _mean);
			}
		}
	}

	_std = sqrtf(_std / (_nxyz.x * _nxyz.y * _nxyz.z));

	qDebug() << _mean << _std << _min << _max;
}
