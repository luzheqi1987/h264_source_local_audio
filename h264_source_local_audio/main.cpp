#include "jerky-rtmp-stream.h"
#include "jerky-h264-source.h"

int main(int argc, char* argv[]){
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 2), &wsad);
	rtmp_stream *stream = (rtmp_stream *)rtmp_stream_create();
	init_connect(stream);
	try_connect(stream);
	system("pause");
	return 0;
}