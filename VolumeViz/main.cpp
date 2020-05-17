#include "VolumeViz.h"
#include <QtWidgets/QApplication>

#include "TIFFStackVolumeDataLoader.h"

int main(int argc, char *argv[])
{
	//////////////////////////////////////////////////////////////////////////
	QApplication a(argc, argv);
	VolumeViz w;
	w.show();
	return a.exec();
}
