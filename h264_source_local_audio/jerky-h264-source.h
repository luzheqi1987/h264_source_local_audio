#pragma once

#include "jerky-rtmp-stream.h"

extern "C"{
#include "util/array-serializer.h"
}

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include <libavformat/avformat.h>
#include "libavformat/avio.h"
#include "libswscale/swscale.h"
#include "libavutil/mathematics.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/samplefmt.h"
#include "libavdevice/avdevice.h"  //摄像头所用
#include "libavfilter/avfilter.h"
#include "libavutil/error.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
}

#include <iostream>
#include <stdio.h>


//定义包头长度，RTMP_MAX_HEADER_SIZE=18
#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)
//存储Nal单元数据的buffer大小
#define BUFFER_SIZE 32768
//搜寻Nal单元时的一些标志
#define GOT_A_NAL_CROSS_BUFFER BUFFER_SIZE+1
#define GOT_A_NAL_INCLUDE_A_BUFFER BUFFER_SIZE+2
#define NO_MORE_BUFFER_TO_READ BUFFER_SIZE+3

typedef struct _NaluUnit
{
	int type;
	int size;
	unsigned char *data;
}NaluUnit;

typedef struct _RTMPMetadata
{
	// video, must be h264 type
	unsigned int    nWidth;
	unsigned int    nHeight;
	unsigned int    nFrameRate;
	unsigned int    nSpsLen;
	unsigned char   *Sps;
	unsigned int    nPpsLen;
	unsigned char   *Pps;
} RTMPMetadata, *LPRTMPMetadata;

struct jerky_h264_source
{
	unsigned int		m_videoIndex;
	int					nalhead_pos;
	bool				m_stop;
	char *				m_streamUrl;
	RTMPMetadata *		m_metaData;
	AVFormatContext *	m_formatCtx;
	AVCodecContext	*	m_videoCodecCtx;
	AVCodec			*	m_videoCodec;
	rtmp_stream *		m_rtmpStream;
	bool				startSend;
	bool				firstPush;
	bool				got_sps_pps;
	std::thread *       sourceThread;
};



struct jerky_h264_source* jerky_h264_source_init(const char* url);
int find_video_index(jerky_h264_source* h264Source);
int video_codec_init(jerky_h264_source* h264Source);
void * jerky_h264_source_thread(void *args);
int ReadFirstNaluFromBuf(NaluUnit &nalu, int &nalhead_pos, int packetSize, unsigned char *packetData);
int ReadOneNaluFromBuf(NaluUnit &nalu, int &nalhead_pos, int packetSize, unsigned char *packetData);
int find_video_index(jerky_h264_source* h264Source);
int video_codec_init(jerky_h264_source* h264Source);
int fetchSpsPps(jerky_h264_source* h264Source, AVPacket *packet);
void startCapture(void *args);