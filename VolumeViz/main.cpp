#include "VolumeViz.h"
#include <QtWidgets/QApplication>

#include <qdir.h>
#include <qfile.h>
#include <qdebug.h>

int main(int argc, char *argv[])
{

	//////////////////////////////////////////////////////////////////////////
	QApplication a(argc, argv);
	VolumeViz w;
	w.show();
	return a.exec();
}
