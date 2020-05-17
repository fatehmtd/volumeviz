#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_VolumeViz.h"
#include "RenderWidget.h"
#include "VolumeData.h"
#include "TransferFunctionEditorWidget.h"
#include <QFocusEvent>

class VolumeViz : public QMainWindow
{
	Q_OBJECT
public:
	VolumeViz(QWidget *parent = Q_NULLPTR);
	~VolumeViz();
protected:
	virtual void paintEvent(QPaintEvent* event) override;
	void initRenderingSystem();
	void loadVolume();
private:
	Ui::VolumeVizClass ui;
	RenderWidget* _renderWidget = nullptr;
	AbstractVolumeRenderer* _volumeRenderer = nullptr;
	VolumeData* _volumeData = nullptr;
};
