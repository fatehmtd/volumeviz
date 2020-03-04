#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_VolumeViz.h"
#include "RenderWidget.h"

class VolumeViz : public QMainWindow
{
	Q_OBJECT
public:
	VolumeViz(QWidget *parent = Q_NULLPTR);

private:
	Ui::VolumeVizClass ui;
	RenderWidget* _renderWidget = nullptr;
	AbstractVolumeRenderer* _volumeRenderer = nullptr;
};
