#include "jerky_main_window.h"

JerkyMainWindow::JerkyMainWindow(QWidget *parent) : QMainWindow(parent),
													h264Source(NULL),
													stream(NULL){
	setupUi(this);
}

JerkyMainWindow::~JerkyMainWindow(){
}


void JerkyMainWindow::startStream() {
	jerky_h264_source* h264Source = jerky_h264_source_init(openUrl->text().toStdString().c_str());
}



void JerkyMainWindow::pushStream() {
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 2), &wsad);
	rtmp_stream *stream = (rtmp_stream *)rtmp_stream_create();
	h264Source->m_rtmpStream = stream;
	init_connect(stream, pushUrl->text().toStdString().c_str());
	try_connect(stream);
}
