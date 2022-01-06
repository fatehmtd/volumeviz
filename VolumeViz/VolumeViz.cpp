#include "VolumeViz.h"
#include "OpenCLVolumeRenderer.h"
#include "VolumeData.h"
#include "BasicVolumeDataLoader.h"

VolumeViz::VolumeViz(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	_renderWidget = new RenderWidget(this);
	setCentralWidget(_renderWidget);

	connect(ui._tfEditorWidget->getCurveEditorWidget(), &CurveEditorWidget::colorsUpdated, this, [=](const TransferFunction& tfColors)
		{
			_renderWidget->setTransferFunction(tfColors);
		});

	_renderWidget->setTransferFunction(ui._tfEditorWidget->getCurveEditorWidget()->getTransferFunction());

	show();
}

VolumeViz::~VolumeViz()
{
	if (_volumeData != nullptr)
		delete _volumeData;
}

void VolumeViz::paintEvent(QPaintEvent* event)
{
	if (_volumeRenderer == nullptr)
	{
		initRenderingSystem();
		resize(width() + 1, height());
	}
}

void VolumeViz::initRenderingSystem()
{
	_volumeRenderer = new OpenCLVolumeRenderer();
	_volumeRenderer->init();

	_renderWidget->setVolumeRenderer(_volumeRenderer);

	loadVolume();
}

void VolumeViz::loadVolume()
{
	BasicVolumeDataLoader loader;
	//_volumeData = loader.load("sample-small");
	//_volumeData = loader.load("data/cat");
	_volumeData = loader.load("data/didel");

	_volumeRenderer->setVolumeData(_volumeData);
	_volumeRenderer->setTransferFunction(ui._tfEditorWidget->getCurveEditorWidget()->getTransferFunction());
	ui._tfEditorWidget->getCurveEditorWidget()->setHistogram(_volumeData->_numBins, _volumeData->_histogram);
	_volumeRenderer->setRenderingStatus(true);
}
