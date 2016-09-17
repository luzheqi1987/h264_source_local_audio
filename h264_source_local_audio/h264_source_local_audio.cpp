#include "h264_source_local_audio.h"

h264_source_local_audio::h264_source_local_audio(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.startStreamBtn, SIGNAL(clicked()), this, SLOT(startStream()));
	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(pushStream()));
}

h264_source_local_audio::~h264_source_local_audio()
{

}


void h264_source_local_audio::startStream() {
	h264Source = jerky_h264_source_init((char *)ui.openUrl->text().toStdString().c_str());
	stream = (rtmp_stream *)rtmp_stream_create();
	h264Source->m_rtmpStream = stream;
	ui.startStreamBtn->setText(QStringLiteral("ÒÑ´ò¿ª"));
	ui.startStreamBtn->setEnabled(false);
	h264Source->sourceThread = new std::thread(jerky_h264_source_thread, h264Source);
}

void h264_source_local_audio::pushStream() {
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 2), &wsad);
	init_connect(stream, ui.openUrl_2->text().toStdString().c_str());
	try_connect(stream);
	h264Source->startSend = true;
	stream->startSend = true;
	stream->sendThread = new std::thread(send_thread, stream);
	//h264SendThread.detach();
}

void h264_source_local_audio::paintEvent(QPaintEvent *e)
{
	Q_UNUSED(e);
	QMainWindow::paintEvent(e);
}

void h264_source_local_audio::showEvent(QShowEvent *e)
{
	if (isVisible())
		repaint();
	QMainWindow::showEvent(e);
}