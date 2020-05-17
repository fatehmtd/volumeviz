#pragma once

#include "VolumeDataLoader.h"

class TIFFStackVolumeDataLoader : public AbstractVolumeDataLoader
{
public:
	// Inherited via VolumeDataLoader
	virtual VolumeData* load(const QString& path) override;
};