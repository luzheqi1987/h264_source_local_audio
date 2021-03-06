#include "jerky-h264-source.h"
#include <thread>
#include <cstdlib>
#include "jerky-circlebuf.h"
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")

unsigned char* m_tmp_nalu_data = (unsigned char*)malloc(BUFFER_SIZE);

struct jerky_h264_source* jerky_h264_source_init(const char* url)
{

	av_register_all();
	avformat_network_init();
	struct jerky_h264_source *h264Source = (jerky_h264_source *)malloc(sizeof(h264Source));
	h264Source = (jerky_h264_source *)malloc(sizeof(jerky_h264_source));
	h264Source->m_formatCtx = avformat_alloc_context();
	h264Source->m_streamUrl = (char *)url;
	h264Source->m_stop = false;
	h264Source->m_videoIndex = 0;
	h264Source->m_metaData = NULL;
	h264Source->nalhead_pos = 0;
	h264Source->startSend = false;
	h264Source->firstPush = true;
	h264Source->got_sps_pps = false;

	AVDictionary* options = NULL;
	std::string strStreamUrl = h264Source->m_streamUrl;
	if (strStreamUrl.compare(0, 4, "rtsp") == 0){
		av_dict_set(&options, "rtsp_transport", "tcp", 0);
	}

	if (avformat_open_input(&h264Source->m_formatCtx, h264Source->m_streamUrl, NULL, &options) != 0){
		{
			printf("Couldn't open input stream.\n");
			return NULL;
		}
	}
	if (avformat_find_stream_info(h264Source->m_formatCtx, NULL) < 0){
		printf("Couldn't find stream information.\n");
		return NULL;
	}

	if ((find_video_index(h264Source)) < 0){
		printf("Couldn't find video index.\n");
		return NULL;
	}

	video_codec_init(h264Source);

	return h264Source;
}

int find_video_index(jerky_h264_source* h264Source)
{
	for (unsigned int i = 0; i < h264Source->m_formatCtx->nb_streams; i++) {
		if (h264Source->m_formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
			h264Source->m_videoIndex = i;
			for (int j = 0; j < h264Source->m_formatCtx->streams[i]->codec->extradata_size; j++)
			{
				printf("%02x ", h264Source->m_formatCtx->streams[i]->codec->extradata[j]);
			}
			printf("\n");
			return h264Source->m_videoIndex;
		}
	}
	return -1;
}

int video_codec_init(jerky_h264_source* h264Source){
	printf("m_videoIndex = %d\n", h264Source->m_videoIndex);
	h264Source->m_videoCodecCtx = h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec;
	h264Source->m_videoCodec = avcodec_find_decoder(h264Source->m_videoCodecCtx->codec_id);
	if (h264Source->m_videoCodec == NULL){
		printf("Codec not found.\n");
		return -1;
	}
	if (avcodec_open2(h264Source->m_videoCodecCtx, h264Source->m_videoCodec, NULL) < 0){
		printf("Could not open codec.\n");
		return -1;
	}
	return 0;
}

void * jerky_h264_source_thread(void *args){
	struct jerky_h264_source *h264Source = (struct jerky_h264_source *) args;
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	bool gotPicture = false;
	int ret;
	int got_picture = false;
	AVFrame	*m_curFrame = av_frame_alloc();
	uint64_t timestamp = 33;
	while (!h264Source->m_stop && av_read_frame(h264Source->m_formatCtx, packet) >= 0){
		if (packet == NULL){
			continue;
		}
		if (packet->stream_index == h264Source->m_videoIndex){
			if (h264Source->startSend && (!h264Source->firstPush || packet->flags == 1 && h264Source->got_sps_pps)){
				if (h264Source->firstPush){
					h264Source->firstPush = false;
				}

				jerky_video_packet * avPacket = new jerky_video_packet(Video_Type_H264);
				struct array_output_data data;
				array_output_serializer_init(&avPacket->data, &data);

				if (packet->flags == 1){
					avPacket->is_key = true;
				}
				else{
					avPacket->is_key = false;
				}
				unsigned char frameType;
				if (packet->flags == 1) {
					frameType = 0x17;// 1:Iframe  7:AVC
				}
				else {
					frameType = 0x27;// 2:Pframe  7:AVC
				}
				//int now = RTMP_GetTime();

				s_w8(&avPacket->data, frameType);

				s_w8(&avPacket->data, 0x01);// AVC NALU
				s_w8(&avPacket->data, 0x00);
				s_w8(&avPacket->data, 0x00);
				s_w8(&avPacket->data, 0x00);

				// NALU size
				int extraLength = 4;
				NaluUnit naluUnit;
				int nalhead_pos = 0;
				bool result = ReadFirstNaluFromBuf(naluUnit, nalhead_pos, packet->size, packet->data);
				if (result && naluUnit.type == 7){
					extraLength += 4 + naluUnit.size;
					// 读取PPS帧
					result = ReadOneNaluFromBuf(naluUnit, nalhead_pos, packet->size, packet->data);
					if (result && naluUnit.type == 8){
						extraLength += 4 + naluUnit.size;
					}
				}
				s_w8(&avPacket->data, (packet->size - 4) >> 24 & 0xff);
				s_w8(&avPacket->data, (packet->size - 4) >> 16 & 0xff);
				s_w8(&avPacket->data, (packet->size - 4) >> 8 & 0xff);
				s_w8(&avPacket->data, (packet->size - 4) >> 0 & 0xff);
				s_write(&avPacket->data, (const char*)packet->data + 4, packet->size - 4);
				avPacket->has_captured = true;
				avPacket->has_encoded = true;

				//XhxAVQueue::instance()->update_packet(bleAVPacket);
				avPacket->pts = timestamp;
				avPacket->dts = avPacket->pts;
				timestamp += 33;
				
				if (h264Source->m_rtmpStream){
					h264Source->m_rtmpStream->packets_mutex->lock();
					circlebuf_push_back(&h264Source->m_rtmpStream->packets, avPacket,
						sizeof(struct jerky_video_packet));
					h264Source->m_rtmpStream->packets_mutex->unlock();
				}
				//XhxAVQueue::instance()->enqueue(bleAVPacket);
			}


			int ret = avcodec_decode_video2(h264Source->m_videoCodecCtx, m_curFrame, &got_picture, packet);

			if (!h264Source->got_sps_pps && packet->flags == 1 && got_picture){

				//写文件头（Write file header）  
				AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
				av_bitstream_filter_filter(h264bsfc, h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec, NULL, &packet->data, &packet->size, packet->data, packet->size, 0);
				if (packet->size <= 0){
					av_free_packet(packet);
					continue;
				}
				printf("%x\n", h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata);
				fetchSpsPps(h264Source, packet);

				jerky_video_packet * videoHeadPacket = new jerky_video_packet(Video_Type_H264);
				videoHeadPacket->dts = 0;
				videoHeadPacket->pts = 0;

				struct array_output_data data;
				array_output_serializer_init(&videoHeadPacket->data, &data);

				s_w8(&videoHeadPacket->data, 0x17);

				s_w8(&videoHeadPacket->data, 0x00);
				s_w8(&videoHeadPacket->data, 0x00);
				s_w8(&videoHeadPacket->data, 0x00);
				s_w8(&videoHeadPacket->data, 0x00);

				/*AVCDecoderConfigurationRecord*/
				s_w8(&videoHeadPacket->data, 0x01);
				s_w8(&videoHeadPacket->data, h264Source->m_metaData->Sps[1]);
				s_w8(&videoHeadPacket->data, h264Source->m_metaData->Sps[2]);
				s_w8(&videoHeadPacket->data, h264Source->m_metaData->Sps[3]);
				s_w8(&videoHeadPacket->data, 0xff);

				/*sps*/
				s_w8(&videoHeadPacket->data, 0xe1);
				s_wb16(&videoHeadPacket->data, h264Source->m_metaData->nSpsLen);
				s_write(&videoHeadPacket->data, h264Source->m_metaData->Sps, h264Source->m_metaData->nSpsLen);

				/*pps*/
				s_w8(&videoHeadPacket->data, 0x01);
				s_wb16(&videoHeadPacket->data, h264Source->m_metaData->nPpsLen);
				s_write(&videoHeadPacket->data, h264Source->m_metaData->Pps, h264Source->m_metaData->nPpsLen);
				if (!h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata){
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata = (uint8_t *)malloc((11 + h264Source->m_metaData->nPpsLen + h264Source->m_metaData->nSpsLen) * sizeof(uint8_t));
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[0] = 0x01;
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[1] = h264Source->m_metaData->Sps[1];
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[2] = h264Source->m_metaData->Sps[2];
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[3] = h264Source->m_metaData->Sps[3];
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[4] = 0xFF;
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[5] = 0xE1;
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[6] = (h264Source->m_metaData->nSpsLen >> 8) && 0xFF;
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[7] = (h264Source->m_metaData->nSpsLen >> 0) && 0xFF;
					memcpy(h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata + 8, h264Source->m_metaData->Sps, h264Source->m_metaData->nSpsLen);
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[8 + h264Source->m_metaData->nSpsLen] = 0x01;
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[8 + h264Source->m_metaData->nSpsLen + 1] = (h264Source->m_metaData->nPpsLen >> 8) && 0xFF;
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata[8 + h264Source->m_metaData->nSpsLen + 2] = (h264Source->m_metaData->nPpsLen >> 0) && 0xFF;
					memcpy(h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata + 8 + h264Source->m_metaData->nSpsLen + 3, h264Source->m_metaData->Pps, h264Source->m_metaData->nPpsLen);
					h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata_size = 11 + h264Source->m_metaData->nPpsLen + h264Source->m_metaData->nSpsLen;
				}
				if (h264Source->m_rtmpStream){
					h264Source->m_rtmpStream->videoSH = videoHeadPacket;
				}
				av_free_packet(packet);
				h264Source->got_sps_pps = true;
				continue;
			}
		}
		av_free_packet(packet);
	}
	return NULL;
}

int fetchSpsPps(jerky_h264_source* h264Source, AVPacket *packet){
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	unsigned char * packetData = (unsigned char*)malloc(packet->size);
	NaluUnit naluUnit;
	h264Source->m_metaData = (RTMPMetadata *)malloc(sizeof(RTMPMetadata));

	memcpy(packetData, packet->data, packet->size);
	int packetSize = packet->size;
	if (packet->size == 0){
		return FALSE;
	}

	bool result = ReadFirstNaluFromBuf(naluUnit, h264Source->nalhead_pos, packetSize, packetData);
	if (!result || naluUnit.type != 7){
		return FALSE;
	}
	h264Source->m_metaData->nSpsLen = naluUnit.size;
	h264Source->m_metaData->Sps = NULL;
	h264Source->m_metaData->Sps = (unsigned char*)malloc(naluUnit.size);
	memcpy(h264Source->m_metaData->Sps, naluUnit.data, naluUnit.size);

	// 读取PPS帧
	result = ReadOneNaluFromBuf(naluUnit, h264Source->nalhead_pos, packetSize, packetData);
	if (!result || naluUnit.type != 8){
		return FALSE;
	}
	h264Source->m_metaData->nPpsLen = naluUnit.size;
	h264Source->m_metaData->Pps = NULL;
	h264Source->m_metaData->Pps = (unsigned char*)malloc(naluUnit.size);
	memcpy(h264Source->m_metaData->Pps, naluUnit.data, naluUnit.size);

	return TRUE;
}

/**
* 从内存中读取出第一个Nal单元
*
* @param nalu 存储nalu数据
* @param read_buffer 回调函数，当数据不足的时候，系统会自动调用该函数获取输入数据。
*					2个参数功能：
*					uint8_t *buf：外部数据送至该地址
*					int buf_size：外部数据大小
*					返回值：成功读取的内存大小
* @成功则返回 1 , 失败则返回0
*/
int ReadFirstNaluFromBuf(NaluUnit &nalu, int &nalhead_pos, int packetSize, unsigned char *packetData){
	int naltail_pos = nalhead_pos;

	memset(m_tmp_nalu_data, 0, BUFFER_SIZE);
	while (nalhead_pos < packetSize)
	{
		//搜索NAL头部
		if (packetData[nalhead_pos++] == 0x00 &&
			packetData[nalhead_pos++] == 0x00)
		{
			if (packetData[nalhead_pos++] == 0x01)
				goto gotnal_head;
			else
			{
				//判断0000 0001
				nalhead_pos--;
				if (packetData[nalhead_pos++] == 0x00 &&
					packetData[nalhead_pos++] == 0x01)
					goto gotnal_head;
				else
					continue;
			}
		}
		else
			continue;

		//搜索NAL尾部，也是下个NALU的头部
	gotnal_head:
		//正常情况下NALU在一个packetData内部
		naltail_pos = nalhead_pos;
		while (naltail_pos < packetSize)
		{
			if (packetData[naltail_pos++] == 0x00 &&
				packetData[naltail_pos++] == 0x00)
			{
				if (packetData[naltail_pos++] == 0x01)
				{
					nalu.size = (naltail_pos - 3) - nalhead_pos;
					break;
				}
				else
				{
					naltail_pos--;
					if (packetData[naltail_pos++] == 0x00 &&
						packetData[naltail_pos++] == 0x01)
					{
						nalu.size = (naltail_pos - 4) - nalhead_pos;
						break;
					}
				}
			}
		}

		nalu.type = packetData[nalhead_pos] & 0x1f;
		memcpy(m_tmp_nalu_data, packetData + nalhead_pos, nalu.size);
		nalu.data = m_tmp_nalu_data;
		nalhead_pos = naltail_pos;
		return TRUE;
	}
}

int ReadOneNaluFromBuf(NaluUnit &nalu, int &nalhead_pos, int packetSize, unsigned char *packetData)
{

	int naltail_pos = nalhead_pos;
	int ret;
	int nalustart;//nal的开始标识符是几个00
	memset(m_tmp_nalu_data, 0, BUFFER_SIZE);
	nalu.size = 0;
	while (1)
	{
		if (naltail_pos >= packetSize){
			break;
		}
		if (nalhead_pos == NO_MORE_BUFFER_TO_READ)
			return FALSE;
		while (naltail_pos < packetSize)
		{
			//寻找NALU的尾部
			if (packetData[naltail_pos++] == 0x00 &&
				packetData[naltail_pos++] == 0x00)
			{
				if (packetData[naltail_pos++] == 0x01)
				{
					nalustart = 3;
					goto gotnal;
				}
				else
				{
					//寻找0000 0001
					naltail_pos--;
					if (packetData[naltail_pos++] == 0x00 &&
						packetData[naltail_pos++] == 0x01)
					{
						nalustart = 4;
						goto gotnal;
					}
					else
						continue;
				}
			}
			else
				continue;

		gotnal:
			// 整个NALU不在packetData内
			if (nalhead_pos == GOT_A_NAL_CROSS_BUFFER || nalhead_pos == GOT_A_NAL_INCLUDE_A_BUFFER)
			{
				return FALSE;
			}
			// 整个NALU在packetData内
			else
			{
				nalu.type = packetData[nalhead_pos] & 0x1f;
				nalu.size = naltail_pos - nalhead_pos - nalustart;
				if (nalu.type == 0x06)
				{
					nalhead_pos = naltail_pos;
					continue;
				}
				memcpy(m_tmp_nalu_data, packetData + nalhead_pos, nalu.size);
				nalu.data = m_tmp_nalu_data;
				nalhead_pos = naltail_pos;
				return TRUE;
			}
		}
	}
	return FALSE;
}

void startCapture(void *args){
	struct jerky_h264_source *h264Source = (struct jerky_h264_source *) args;
	pthread_t thread;
	int ret = pthread_create(&thread, NULL, jerky_h264_source_thread, h264Source);
}
/*int main(int argc, char* argv[]){
jerky_h264_source* h264Source = jerky_h264_source_init();
std::thread h264SourceThread(jerky_h264_source_thread, h264Source);
h264SourceThread.join();

system("pause");
return 0;
}*/
