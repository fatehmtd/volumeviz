#pragma once

#include <QString>
#include "VolumeData.h"

class AbstractVolumeDataLoader
{
public:
	virtual VolumeData* load(const QString& path) = 0;

	virtual void saveToBinFormat(const VolumeData* vdata, const QString& path);
};

