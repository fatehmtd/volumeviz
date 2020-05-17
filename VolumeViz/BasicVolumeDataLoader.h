#pragma once

#include "VolumeDataLoader.h"

class BasicVolumeDataLoader : public AbstractVolumeDataLoader
{
public:
	BasicVolumeDataLoader(); 
	// Inherited via VolumeDataLoader
	virtual VolumeData* load(const QString& path) override;
};