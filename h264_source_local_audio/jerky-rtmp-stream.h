#pragma once

#include <iostream>
#include <inttypes.h>
#include "jerky-circlebuf.h"
#include "w32-pthreads/pthread.h"
#include <thread>
extern "C"{
#include "util/bmem.h"
#include "util/dstr.h"
#include "util/array-serializer.h"
#include "rtmp/rtmp.h"
#include "rtmp/log.h"
}
#include <mutex> 

#include "mstream.h"


#define Packet_Type_Audio 0x08
#define Packet_Type_Video 0x09

#define Audio_Type_AAC 0x01
#define Audio_Type_MP3 0x02

#define Video_Type_H264 0x17

struct jerky_av_packet
{
	jerky_av_packet(char type)
		: pktType(type)
		, pts(0)
		, dts(0)
		, is_key(false)
		, has_encoded(false)
		, has_captured(false)
	{

	}
	jerky_av_packet(){}
	serializer data;
	char pktType;
	uint64_t pts;
	uint64_t dts;
	bool has_encoded;
	bool has_captured;
	bool is_key;
};

struct jerky_audio_packet : jerky_av_packet
{
	jerky_audio_packet(char at)
		: jerky_av_packet(Packet_Type_Audio)
		, audioType(at)
	{
	}
	~jerky_audio_packet() {}
	char audioType;
};

struct jerky_video_packet : jerky_av_packet
{
	jerky_video_packet(char vt)
		: jerky_av_packet(Packet_Type_Video)
		, videoType(vt)
	{

	}
	~jerky_video_packet() {}
	char videoType;
};

struct rtmp_stream {
	struct jerky_circlebuf	packets;
	bool					sent_headers;
	std::mutex *			packets_mutex;
	std::thread *           sendThread;
	volatile bool			connecting;

	volatile bool			active;
	volatile bool			disconnected;
	pthread_t				send_thread;

	int						max_shutdown_time_sec;


	struct dstr				path, key;
	struct dstr				username, password;
	struct dstr				encoder_name;
	struct dstr				bind_ip;

	int64_t					last_dts_usec;

	uint64_t				total_bytes_sent;
	int						dropped_frames;
	jerky_av_packet			*videoSH;
	jerky_av_packet			*audioSH;
	bool					startSend;
	RTMP					rtmp;
};

inline size_t num_buffered_packets(struct rtmp_stream *stream);
inline void set_rtmp_dstr(AVal *val, struct dstr *str);
#ifdef _WIN32
void win32_log_interface_type(struct rtmp_stream *stream);
#endif
inline void free_packets(struct rtmp_stream *stream);
void *rtmp_stream_create();
bool init_connect(struct rtmp_stream *stream, const char * url);
int try_connect(struct rtmp_stream *stream);
void * send_thread(void *data);