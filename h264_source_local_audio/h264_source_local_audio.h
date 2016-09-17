#ifndef H264_SOURCE_LOCAL_AUDIO_H
#define H264_SOURCE_LOCAL_AUDIO_H

#include <QtWidgets/QMainWindow>
#include "ui_h264_source_local_audio.h"
#include "jerky-h264-source.h"
#include "jerky-rtmp-stream.h"
#include <thread>

class h264_source_local_audio : public QMainWindow
{
	Q_OBJECT

public:
	h264_source_local_audio(QWidget *parent = 0);
	~h264_source_local_audio();

private:
	Ui::h264_source_local_audioClass ui;

	jerky_h264_source* h264Source;
	rtmp_stream *stream;
	std::thread h264SourceThread;
	std::thread h264SendThread;

private:
	void paintEvent(QPaintEvent *);
	void showEvent(QShowEvent *);

private slots:
	void startStream();
	void pushStream();
};

#endif // H264_SOURCE_LOCAL_AUDIO_H
