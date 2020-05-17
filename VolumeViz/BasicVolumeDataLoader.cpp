#include "BasicVolumeDataLoader.h"
#include <QDir>
#include <QFileInfo>
#include <QRegExp>
#include <qdebug>
#include <QImage>
#include <QFile>

BasicVolumeDataLoader::BasicVolumeDataLoader()
{
}

VolumeData* BasicVolumeDataLoader::load(const QString& path)
{
	QDir directory(path);
	QFile file(directory.absoluteFilePath(QString("%1.bin").arg(directory.dirName())));
	if (file.open(QIODevice::OpenModeFlag::ReadOnly))
	{
		auto vdata = new VolumeData;

		file.read((char*)(&vdata->_nxyz), sizeof(vdata->_nxyz));
		file.read((char*)(&vdata->_sxyz), sizeof(vdata->_sxyz));
		vdata->init(vdata->_nxyz.x, vdata->_nxyz.y, vdata->_nxyz.z,
			vdata->_sxyz.x, vdata->_sxyz.y, vdata->_sxyz.z);
		file.read((char*)vdata->_data, sizeof(VolumeData::DataType) * vdata->_nxyz.x * vdata->_nxyz.y * vdata->_nxyz.z);
		file.read((char*)&vdata->_min, sizeof(float));
		file.read((char*)&vdata->_max, sizeof(float));
		file.read((char*)&vdata->_mean, sizeof(float));
		file.read((char*)&vdata->_std, sizeof(float));
		vdata->computeHistogram();

		file.close();
		/*
		// half size, just to be able to commit the file
		QFile hfFile(directory.absoluteFilePath(QString("%1-small.bin").arg(directory.dirName())));
		if (hfFile.open(QIODevice::WriteOnly))
		{
			int w = vdata->_nxyz.x / 2;
			int h = vdata->_nxyz.y / 2;
			int d = vdata->_nxyz.z / 2;
			auto tempVolume = new VolumeData;
			tempVolume->init(w, h, d,
				1.0f, 1.0f, 1.0f);
			
			for (int z = 0; z < d; z++) {
				for (int y = 0; y < h; y++) {
					for (int x = 0; x < w; x++) {
						int z2 = z * 2;
						int y2 = y * 2;
						int x2 = x * 2;
						int w2 = w * 2;
						int h2 = h * 2;
						int d2 = d * 2;
						tempVolume->_data[z + h * (y + x * h)] = vdata->_data[(z2 + h2 * (y2 + x2 * h2))];
					}
				}
			}
			tempVolume->computeHistogram();
			hfFile.close();
			saveToBinFormat(tempVolume, "./sample-small");
			return tempVolume;
		}
		//*/
		return vdata;
	}
	return nullptr;
}
