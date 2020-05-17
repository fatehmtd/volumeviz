#include "TIFFStackVolumeDataLoader.h"

#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <qdebug>
#include <QImage>
#include <QFile>

VolumeData* TIFFStackVolumeDataLoader::load(const QString& path)
{
	VolumeData* vdata = new VolumeData;

	QDir directory(path);

	if (directory.exists(QString("%1.bin").arg(directory.dirName())))
	{
		QFile file(directory.absoluteFilePath(QString("%1.bin").arg(directory.dirName())));
		if (file.open(QIODevice::OpenModeFlag::ReadOnly))
		{
			file.read((char*)(&vdata->_nxyz), sizeof(vdata->_nxyz));
			file.read((char*)(&vdata->_sxyz), sizeof(vdata->_sxyz));
			vdata->init(vdata->_nxyz.x, vdata->_nxyz.y, vdata->_nxyz.z,
				vdata->_sxyz.x, vdata->_sxyz.y, vdata->_sxyz.z);
			file.read((char*)vdata->_data, sizeof(VolumeData::DataType) * vdata->_nxyz.x * vdata->_nxyz.y * vdata->_nxyz.z);
			file.read((char*)&vdata->_min, sizeof(float));
			file.read((char*)&vdata->_max, sizeof(float));
			file.read((char*)&vdata->_mean, sizeof(float));
			file.read((char*)&vdata->_std, sizeof(float));
			//file.read((char*)&vdata->_numBins, sizeof(unsigned int));
			//vdata->_histogram = new unsigned int[vdata->_numBins];
			//file.read((char*)&vdata->_histogram, sizeof(unsigned int)*vdata->_numBins);
			vdata->computeHistogram();

			file.close();
		}
	}
	else
	{
		auto entries = directory.entryList(QDir::Filter::Files);

		int w, h;

		{
			QImage image(directory.absoluteFilePath(entries[0]));
			vdata->init(image.width(), image.height(), entries.length(), 1.0f, 1.0f, 1.0f);
			w = image.width(); h = image.height();
		}

		int index = 0;
		for (auto entry : entries)
		{
			QImage image(directory.absoluteFilePath(entry));
			if (image.isNull())
			{
				qDebug() << "niet : " << entry;
				delete vdata;
				return nullptr;
			}
			//*/

			const int planeSize = w * h;

			auto* plane = &vdata->_data[planeSize * index];

			for (int i = 0; i < h; i++)
			{
				for (int j = 0; j < w; j++)
				{
					plane[j + i * w] = image.pixelColor(j, i).red();
					//plane[j + i * w] = index % 256;
				}
			}

			index++;
		}

		vdata->computeHistogram();
		
		AbstractVolumeDataLoader::saveToBinFormat(vdata, path);
	}
	return vdata;
}
