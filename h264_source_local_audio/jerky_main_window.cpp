#include "jerky_main_window.h"
#include "winnt.h"
#include <thread>

JerkyMainWindow::JerkyMainWindow(QWidget *parent) : QMainWindow(parent),
													h264Source(NULL),
													stream(NULL){
	setupUi(this);
	connect(startStreamBtn, SIGNAL(clicked()), this, SLOT(startStream()));
}

JerkyMainWindow::~JerkyMainWindow(){
}


void JerkyMainWindow::startStream() {
	h264Source = jerky_h264_source_init(openUrl->text().toStdString().c_str());
	startCapture(h264Source);
	startStreamBtn->setText(QStringLiteral("ÒÑ´ò¿ª"));
	startStreamBtn->setEnabled(false);
}



void JerkyMainWindow::pushStream() {
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 2), &wsad);
	stream = (rtmp_stream *)rtmp_stream_create();
	h264Source->m_rtmpStream = stream;
	init_connect(stream, pushUrl->text().toStdString().c_str());
	try_connect(stream);
	h264Source->startSend = true;
}
