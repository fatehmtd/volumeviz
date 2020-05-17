#include "VolumeDataLoader.h"
#include <QFile>
#include <QDir>
#include <QDebug>

void AbstractVolumeDataLoader::saveToBinFormat(const VolumeData* vdata, const QString& path)
{
	if (vdata == nullptr) return;
	QDir directory(path);
	directory.mkpath(path);
	QFile file(directory.absoluteFilePath(QString("%1.bin").arg(directory.dirName())));
	if (file.open(QIODevice::OpenModeFlag::WriteOnly))
	{
		file.write((char*)(&vdata->_nxyz), sizeof(vdata->_nxyz));
		file.write((char*)(&vdata->_sxyz), sizeof(vdata->_sxyz));
		file.write((char*)vdata->_data, sizeof(VolumeData::DataType) * vdata->_nxyz.x * vdata->_nxyz.y * vdata->_nxyz.z);
		file.write((char*)&vdata->_min, sizeof(float));
		file.write((char*)&vdata->_max, sizeof(float));
		file.write((char*)&vdata->_mean, sizeof(float));
		file.write((char*)&vdata->_std, sizeof(float));
		file.close();
	}
}
