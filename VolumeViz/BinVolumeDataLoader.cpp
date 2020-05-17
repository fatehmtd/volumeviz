#include "BinVolumeDataLoader.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QImage>

template <typename T = unsigned short>
T reverseBits(T value)
{
	int nbits = sizeof(T) * 8;
	int nbytes = sizeof(T);

	T output = 0;

	for (int i = 0; i < nbytes; i++)
	{
		char extractedByte = (value >> (i * 8)) & 0xff;
		char outputByte = 0;

		for (int j = 0; j < 8; j++)
		{
			outputByte |= (((extractedByte >> (8 - (j + 1))) & 1) << j);
		}
		//*/
		output |= (outputByte << (((nbytes - (i + 1)) * 8)));
		//output |= (outputByte << (8*i));
	}

	return output;
}

VolumeData* BinVolumeDataLoader::load(const QString& path)
{
	QDir dir(path);
	auto entries = dir.entryList(QDir::Filter::Files);

	int numEntries = entries.length();
	int size = 256;

	VolumeData* volumeData = new VolumeData;

	volumeData->_data = new VolumeData::DataType[size * size * numEntries];
	memset(volumeData->_data, 0, sizeof(float) * size * size * numEntries);

	volumeData->_nxyz = glm::int3(size, size, numEntries);
	volumeData->_sxyz = glm::float3(1.0f, 1.0f, 2.0f);

	for (int i = 1; i <= numEntries; i++)
	{
		QFile file(QString("%1.%2").arg(dir.absoluteFilePath(dir.dirName())).arg(i));
		if (file.open(QIODevice::ReadOnly))
		{
			auto* plane = &volumeData->_data[size * size * (i - 1)];
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

	volumeData->_mean = volumeData->_std = 0.0f;
	volumeData->_min = std::numeric_limits<float>::max();
	volumeData->_max = -volumeData->_min;

	for (int w = 0; w < numEntries; w++)
	{
		const auto* plane = &volumeData->_data[size * size * w];

		for (int u = 0; u < size; u++)
		{
			for (int v = 0; v < size; v++)
			{
				float value = plane[size * u + v];
				volumeData->_mean += value;

				if (value < volumeData->_min)
					volumeData->_min = value;

				if (value > volumeData->_max)
					volumeData->_max = value;
			}
		}
	}

	volumeData->_mean /= size * size * numEntries;

	volumeData->_numBins = 2048;
	volumeData->_histogram = new unsigned int[volumeData->_numBins];

	float deltaBin = (volumeData->_max - volumeData->_min) / (float)volumeData->_numBins;

	for (int w = 0; w < numEntries; w++)
	{
		const auto* plane = &volumeData->_data[size * size * w];

		for (int u = 0; u < size; u++)
		{
			for (int v = 0; v < size; v++)
			{
				float value = plane[size * u + v];
				++volumeData->_histogram[int((value - volumeData->_min) / deltaBin)];
				volumeData->_std += (value - volumeData->_mean) * (value - volumeData->_mean);
			}
		}
	}

	volumeData->_std = sqrtf(volumeData->_std / (size * size * numEntries));

	qDebug() << volumeData->_mean << volumeData->_std << volumeData->_min << volumeData->_max;
	
	return volumeData;
}
