#ifndef JERKYMAINWINDOW_H

#define JERKYMAINWINDOW_H

#include "ui_jerky_main_window.h"
#include "jerky-rtmp-stream.h"
#include "jerky-h264-source.h"

#include <QtWidgets/QMainWindow>

namespace Ui
{
	class MainWindow;
}


class JerkyMainWindow :public QMainWindow, public Ui::MainWindow

{

	Q_OBJECT

public:

	explicit JerkyMainWindow(QWidget *parent = 0);

	~JerkyMainWindow();

private:
	jerky_h264_source* h264Source; 
	rtmp_stream *stream;

private slots:

	void startStream();

	void pushStream();

};

#endif