#include "VolumeViz.h"
#include "OpenCLVolumeRenderer.h"
#include "VolumeData.h"

VolumeViz::VolumeViz(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	_renderWidget = new RenderWidget(this);
	setCentralWidget(_renderWidget);

	//connect(this, &QWidget::show)

	show();

	_volumeRenderer = new OpenCLVolumeRenderer();
	_volumeRenderer->init();

	VolumeData* vdata = new VolumeData;
	//vdata->load("./data/mrbrain/");
	vdata->load("./data/cthead/");

	_renderWidget->setVolumeRenderer(_volumeRenderer);
}
