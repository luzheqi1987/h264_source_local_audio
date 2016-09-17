#include "h264_source_local_audio.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	h264_source_local_audio w;
	w.show();
	return a.exec();
}
