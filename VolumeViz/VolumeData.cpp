#include "VolumeData.h"
#include <QDir>
#include <QFile>
#include <QDebug>

template <typename T = unsigned short>
T reverseBits(T value)
{
	//return value;
	int nbits = sizeof(T) * 8;
	T output = 0;

	for (int i = 0; i < nbits; i++)
	{
		//output |= ((value & (1 << i)) >> i) << (nbits - (i + 1));
		output |= ((value & (1 << i)) >> i) << (nbits - (i + 1));
	}

	return output;
}


#include <QImage>

bool VolumeData::load(const QString& path)
{
	QDir dir(path);
	auto entries = dir.entryList(QDir::Filter::Files);

	int numEntries = entries.length();
	int size = 256;

	_data = new float[size * size * numEntries];
	memset(_data, 0, sizeof(float) * size * size * numEntries);

	_nxyz = glm::int3(size, size, numEntries);
	_sxyz = glm::float3(1.0f, 1.0f, 2.0f);

	for (int i = 1; i <= numEntries; i++)
	{
		QFile file(QString("%1.%2").arg(dir.absoluteFilePath(dir.dirName())).arg(i));
		if (file.open(QIODevice::ReadOnly))
		{
			float* plane = &_data[size * size * (i - 1)];
			auto rawBytes = file.readAll();
			int offset = 0;
			for (int u = 0; u < size; u++)
			{
				for (int v = 0; v < size; v++)
				{
					unsigned short temp = 0;
					memcpy(&temp, rawBytes.constData() + offset, sizeof(temp));
					offset += 2;
					temp = reverseBits(temp);
					plane[size * u + v] = temp;
				}
			}
		}
	}

	_mean = _std = 0.0f;
	_min = std::numeric_limits<float>::max();
	_max = -_min;

	for (int w = 0; w < numEntries; w++)
	{
		const float* plane = &_data[size * size * w];

		for (int u = 0; u < size; u++)
		{
			for (int v = 0; v < size; v++)
			{
				float value = plane[size * u + v];
				_mean += value;
				if (value < _min)
				{
					_min = value;
				}

				if (value > _max)
					_max = value;
			}
		}
	}
	_mean /= size * size * numEntries;
	for (int w = 0; w < numEntries; w++)
	{
		const float* plane = &_data[size * size * w];

		for (int u = 0; u < size; u++)
		{
			for (int v = 0; v < size; v++)
			{
				_std += powf(plane[size * u + v] - _mean, 2.0f);
			}
		}
	}

	_std = sqrtf(_std / (size * size * numEntries));

	qDebug() << _mean << _std << _min << _max;
	return true;
}
