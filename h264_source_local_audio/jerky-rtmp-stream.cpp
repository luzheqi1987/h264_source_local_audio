#include "jerky-rtmp-stream.h"
#include "jerky-encoder.h"
#include <stdio.h>
#include <fstream>
#include <stdio.h>
#include <chrono>    // std::chrono::seconds
extern "C"{
	#include "util/dstr.h"
	#include "net-if.h"
	#include "util/bmem.h"
}
// rtmp errors
#define JERKY_SUCESS                              0
#define JERKY_FAILED                              -1
#define JERKY_RTMPSEND_ERROR                      100
#define JERKY_RTMPCONNECT_ERROR                   101

#define StreamChannel_Metadata  0x03
#define StreamChannel_Video     0x04
#define StreamChannel_Audio     0x05

void *rtmp_stream_create()
{
	struct rtmp_stream *stream = (rtmp_stream *) bzalloc (sizeof(struct rtmp_stream));
	stream -> packets_mutex = new std::mutex();
	RTMP_Init(&stream->rtmp);
	RTMP_LogSetLevel(RTMP_LOGWARNING);

	stream -> sent_headers = false;

	return stream;
}

bool init_connect(struct rtmp_stream *stream, const char * url)
{
	const char *bind_ip;
	int64_t drop_p;
	int64_t drop_b;

	free_packets(stream);

	stream->disconnected = true;
	stream->total_bytes_sent = 0;
	stream->dropped_frames = 0;

	dstr_copy(&stream->path, url);
	dstr_copy(&stream->username, "");
	dstr_copy(&stream->password, "");
	dstr_depad(&stream->path);
	dstr_depad(&stream->key);
	drop_b = (int64_t)500;
	drop_p = (int64_t)800;
	stream->max_shutdown_time_sec = 5;

	if (drop_p < (drop_b + 200))
		drop_p = drop_b + 200;

	bind_ip = "default";
	dstr_copy(&stream->bind_ip, bind_ip);

	return true;
}

int try_connect(struct rtmp_stream *stream)
{
	if (dstr_is_empty(&stream->path)) {
		printf("URL is empty\n");
		return -1;
	}

	printf("Connecting to RTMP URL %s...\n", stream->path.array);

	memset(&stream->rtmp.Link, 0, sizeof(stream->rtmp.Link));
	if (!RTMP_SetupURL(&stream->rtmp, stream->path.array))
		return -1;

	RTMP_EnableWrite(&stream->rtmp);

	dstr_copy(&stream->encoder_name, "FMLE/3.0 (compatible; FMSc/1.0)");

	set_rtmp_dstr(&stream->rtmp.Link.pubUser, &stream->username);
	set_rtmp_dstr(&stream->rtmp.Link.pubPasswd, &stream->password);
	set_rtmp_dstr(&stream->rtmp.Link.flashVer, &stream->encoder_name);
	stream->rtmp.Link.swfUrl = stream->rtmp.Link.tcUrl;

	if (dstr_is_empty(&stream->bind_ip) ||
		dstr_cmp(&stream->bind_ip, "default") == 0) {
		memset(&stream->rtmp.m_bindIP, 0, sizeof(stream->rtmp.m_bindIP));
	}
	else {
		bool success = netif_str_to_addr(&stream->rtmp.m_bindIP.addr, &stream->rtmp.m_bindIP.addrLen, stream->bind_ip.array);
		if (success)
			printf("Binding to IP\n");
	}

	stream->rtmp.m_outChunkSize = 4096;
	stream->rtmp.m_bSendChunkSizeInfo = true;
	stream->rtmp.m_bUseNagle = true;

#ifdef _WIN32
	win32_log_interface_type(stream);
#endif

	if (!RTMP_Connect(&stream->rtmp, NULL))
		return -2;
	if (!RTMP_ConnectStream(&stream->rtmp, 0))
		return -3;

	printf("Connection to %s successful\n", stream->path.array);

	//return init_send(stream);
	return 0;
}


inline size_t num_buffered_packets(struct rtmp_stream *stream)
{
	return stream->packets.size / sizeof(struct encoder_packet);
}

inline void set_rtmp_dstr(AVal *val, struct dstr *str)
{
	bool valid = !dstr_is_empty(str);
	val->av_val = valid ? str->array : NULL;
	val->av_len = valid ? (int)str->len : 0;
}



#ifdef _WIN32
void win32_log_interface_type(struct rtmp_stream *stream)
{
	RTMP *rtmp = &stream->rtmp;
	MIB_IPFORWARDROW route;
	uint32_t dest_addr, source_addr;
	char hostname[256];
	HOSTENT *h;

	if (rtmp->Link.hostname.av_len >= sizeof(hostname) - 1)
		return;

	strncpy(hostname, rtmp->Link.hostname.av_val, sizeof(hostname));
	hostname[rtmp->Link.hostname.av_len] = 0;

	h = gethostbyname(hostname);
	if (!h)
		return;

	dest_addr = *(uint32_t*)h->h_addr_list[0];

	if (rtmp->m_bindIP.addrLen == 0)
		source_addr = 0;
	else if (rtmp->m_bindIP.addr.ss_family == AF_INET)
		source_addr = (*(struct sockaddr_in*)&rtmp->m_bindIP)
		.sin_addr.S_un.S_addr;
	else
		return;

	if (!GetBestRoute(dest_addr, source_addr, &route)) {
		MIB_IFROW row;
		memset(&row, 0, sizeof(row));
		row.dwIndex = route.dwForwardIfIndex;

		if (!GetIfEntry(&row)) {
			uint32_t speed = row.dwSpeed / 1000000;
			char *type;
			struct dstr other = { 0 };

			if (row.dwType == IF_TYPE_ETHERNET_CSMACD) {
				type = "ethernet";
			}
			else if (row.dwType == IF_TYPE_IEEE80211) {
				type = "802.11";
			}
			else {
				dstr_printf(&other, "type %lu", row.dwType);
				type = other.array;
			}

			printf("Interface: %s (%s, %lu mbps)\n", row.bDescr, type,
				speed);

			dstr_free(&other);
		}
	}
}
#endif

inline void free_packets(struct rtmp_stream *stream)
{
	size_t num_packets;

	num_packets = num_buffered_packets(stream);
	if (num_packets)
		printf("Freeing %d remaining packets", (int)num_packets);

	while (stream->packets.size) {
		struct encoder_packet packet;
		circlebuf_pop_front(&stream->packets, &packet, sizeof(packet));
		obs_free_encoder_packet(&packet);
	}
}

static inline bool get_next_packet(struct rtmp_stream *stream, struct jerky_av_packet *packet)
{
	bool new_packet = false;
	stream->packets_mutex->lock();
	if (stream->packets.size) {
		circlebuf_pop_front(&stream->packets, packet,
			sizeof(struct jerky_video_packet));
		new_packet = true;
	}
	stream->packets_mutex->unlock();

	return new_packet;
}

int send_packet(RTMP* m_pRtmp, const char *data, int dataSize, unsigned long long timestamp, unsigned int pktType, int channel)
{
	if (m_pRtmp == NULL) {
		return FALSE;
	}

	RTMPPacket packet;
	RTMPPacket_Reset(&packet);
	RTMPPacket_Alloc(&packet, dataSize);
#ifdef DEBUG_RTMPMUXER
	int dataSize = data.size();
	//printf("data.size = %d",dataSize);
#endif
	packet.m_packetType = pktType;
	packet.m_nChannel = channel;
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_nTimeStamp = timestamp;

	//log_trace("type:%d,timestamp:%d",pktType,timestamp);
	packet.m_hasAbsTimestamp = 0;
	packet.m_nInfoField2 = m_pRtmp->m_stream_id;
	packet.m_nBodySize = dataSize;
	memcpy(packet.m_body, data, dataSize);

	int ret = RTMP_SendPacket(m_pRtmp, &packet, 0);
	RTMPPacket_Free(&packet);

	if (ret != TRUE)
	{
		//        printf("RTMP_SendPacket error_code %d\n",ret);
		//        fflush(stdout);
		perror("RTMP_SendPacket error");
		return ret;
	}

	return TRUE;
}

void * send_thread(void *data)
{
	struct rtmp_stream *stream = (rtmp_stream *)data;

	while (stream -> startSend) {
		struct jerky_av_packet packet;

		if (!get_next_packet(stream, &packet)){
			std::this_thread::sleep_for(std::chrono::microseconds(10));
			continue;
		}

		if (!stream->sent_headers) {
			if (stream->videoSH) {
				int ret = JERKY_SUCESS;
				if (send_packet(&stream->rtmp, (char *)((array_output_data *)(stream->videoSH->data.data))->bytes.array,
					((array_output_data *)(stream->videoSH->data.data))->bytes.num,
					stream->videoSH->pts, RTMP_PACKET_TYPE_VIDEO, StreamChannel_Video) != TRUE) {
					ret = JERKY_RTMPSEND_ERROR;
					break;
				}
			}
			stream->sent_headers = true;
		}
		
		if (send_packet(&stream->rtmp, (char *)((array_output_data *)(packet.data.data))->bytes.array,
			((array_output_data *)(packet.data.data))->bytes.num,
			packet.pts, RTMP_PACKET_TYPE_VIDEO, StreamChannel_Video) != TRUE) {
			break;
		}
	}

	RTMP_Close(&stream->rtmp);

	free_packets(stream);
	stream->sent_headers = false;
	return NULL;
}