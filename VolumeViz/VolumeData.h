#pragma once

#include <QString>
#include <QObject>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>

class VolumeData : public QObject
{
protected:
	glm::int3 _nxyz;
	glm::float3 _sxyz;
	float* _data = nullptr;
	float _mean, _std, _min, _max;
public:
	bool load(const QString& path);
};