#include "jerky-h264-source.h"
#include <thread>
#include <cstdlib>
#include "jerky-circlebuf.h"
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")

unsigned char* m_pFileBuf_tmp = (unsigned char*)malloc(BUFFER_SIZE);

struct jerky_h264_source* jerky_h264_source_init()
{

	av_register_all();
	avformat_network_init();
	struct jerky_h264_source *h264Source = (jerky_h264_source *)malloc(sizeof(h264Source));
	h264Source = (jerky_h264_source *)malloc(sizeof(jerky_h264_source));
	h264Source->m_formatCtx = avformat_alloc_context();
	h264Source->m_streamUrl = "rtmp://192.168.1.158/live/asdasd";
	h264Source->m_stop = false;
	h264Source->m_videoIndex = 0;
	h264Source->m_metaData = NULL;
	h264Source->nalhead_pos = 0;



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
	if (avformat_find_stream_info(h264Source->m_formatCtx, NULL)<0){
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
			printf("m_videoIndex = %d\n", h264Source->m_videoIndex);
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
	if (avcodec_open2(h264Source->m_videoCodecCtx, h264Source->m_videoCodec, NULL)<0){
		printf("Could not open codec.\n");
		return -1;
	}
	return 0;
}

void jerky_h264_source_thread(void *args){
	struct jerky_h264_source *h264Source = (struct jerky_h264_source *) args;
	AVPacket *packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	bool gotPicture = false;
	int ret;
	AVOutputFormat *ofmt = NULL;
	AVFormatContext *ofmt_ctx = NULL;
	//const char * out_filename = "rtmp://192.168.1.158/live/test2"; //输出 URL（Output URL）[RTMP]  
	const char * out_filename = "rtmp://gs.push.rgbvr.com/rgbvr/test2"; //输出 URL（Output URL）[RTMP]  
	//av_register_all();
	//Network  
	//avformat_network_init();
	//输出（Output）  
	avformat_alloc_output_context2(&ofmt_ctx, NULL, "flv", out_filename); //RTMP  
	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		return;
	}
	ofmt = ofmt_ctx->oformat;
	for (int i = 0; i < h264Source->m_formatCtx->nb_streams; i++) {
		//根据输入流创建输出流（Create output AVStream according to input AVStream）  
		AVStream *in_stream = h264Source->m_formatCtx->streams[i];
		AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		//printf("codec pix_fmts: %d \n", in_stream->codec->codec->pix_fmts);
		if (!out_stream) {
			printf("Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			return;
		}
		//复制AVCodecContext的设置（Copy the settings of AVCodecContext）  
		ret = avcodec_copy_context(out_stream->codec, in_stream->codec);
		if (ret < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
			return;
		}
		out_stream->codec->codec_tag = 0;
		if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
	//Dump Format------------------  
	av_dump_format(ofmt_ctx, 0, out_filename, 1);

	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf("Could not open output URL '%s'", out_filename);
			return;
		}
	}


	//写文件头（Write file header）
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		printf("Error occurred when opening output URL\n");
		return;
	}
	bool start = false;
	float timestamp = 0.0;
	int got_picture = false;
	AVFrame	*m_curFrame = av_frame_alloc();
	int64_t start_time = av_gettime();
	int videoTimestamp = 0;
	bool got_sps_pps = false;
	while (!h264Source->m_stop && av_read_frame(h264Source->m_formatCtx, packet) >= 0){

		//printf("Receive Packet, Packet Stream Index Is: %d, Frame Type: %d \n", packet->stream_index, packet->flags);
		if (packet->stream_index == h264Source->m_videoIndex){
			ret = av_interleaved_write_frame(ofmt_ctx, packet);
			//if (packet->flags && !h264Source->m_metaData){
			//
			//}

			//int ret = avcodec_decode_video2(h264Source->m_videoCodecCtx, m_curFrame, &got_picture, packet);

			/*if (got_picture && packet->flags == 1 && !got_sps_pps){
				 //写文件头（Write file header）  
				printf("%x\n", packet->data[100]);
				AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
				av_bitstream_filter_filter(h264bsfc, h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec, NULL, &packet->data, &packet->size, packet->data, packet->size, 0);
				//av_bitstream_filter_filter(h264bsfc, h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec, NULL, &packet->data, &packet->size, NULL, 0, 0);
				if (packet->size <= 0){
					av_free_packet(packet);
					continue;
				}
				printf("%x\n", h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata);
				//fwrite(h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata, h264Source->m_formatCtx->streams[h264Source->m_videoIndex]->codec->extradata_size, 1, fp);
				fetchSpsPps(h264Source, packet);
				if (ret < 0) {
					printf("Error occurred when opening output URL\n");
					av_free_packet(packet);
					continue;
				}
				AVPacket *spsPPsPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
				char * body = (char *)malloc(1024);
				int i = 0;
				body[i++] = 0x17;
				body[i++] = 0x00;

				body[i++] = 0x00;
				body[i++] = 0x00;
				body[i++] = 0x00;

				//AVCDecoderConfigurationRecord
				body[i++] = 0x01;
				body[i++] = h264Source->m_metaData->Sps[1];
				body[i++] = h264Source->m_metaData->Sps[2];
				body[i++] = h264Source->m_metaData->Sps[3];
				body[i++] = 0xff;

				//sps
				body[i++] = 0xe1;
				body[i++] = (h264Source->m_metaData->nSpsLen >> 8) & 0xff;
				body[i++] = h264Source->m_metaData->nSpsLen & 0xff;
				memcpy(&body[i], h264Source->m_metaData->Sps, h264Source->m_metaData->nSpsLen);
				i += h264Source->m_metaData->nSpsLen;

				//pps
				body[i++] = 0x01;
				body[i++] = (h264Source->m_metaData->nPpsLen >> 8) & 0xff;
				body[i++] = (h264Source->m_metaData->nPpsLen) & 0xff;
				memcpy(&body[i], h264Source->m_metaData->Pps, h264Source->m_metaData->nPpsLen);
				i += h264Source->m_metaData->nPpsLen;

				spsPPsPacket->data = (uint8_t *)malloc(i + 4);
				memcpy(spsPPsPacket->data + 4, body, i);
				spsPPsPacket->data[0] = i >> 24 & 0xFF;
				spsPPsPacket->data[1] = i >> 16 & 0xFF;
				spsPPsPacket->data[2] = i >> 8 & 0xFF;
				spsPPsPacket->data[3] = i >> 0 & 0xFF;


				spsPPsPacket->dts = 0;
				spsPPsPacket->flags = 1;
				spsPPsPacket->pts = 0;
				spsPPsPacket->size = i + 4;
				spsPPsPacket->stream_index = h264Source->m_videoIndex;

				ret = av_interleaved_write_frame(ofmt_ctx, packet);
				if (ret < 0){
					printf("av_interleaved_write_frame header errot\n");
				}

				av_free_packet(packet);				
				got_sps_pps = true;
				continue;
			}*/

			//if (got_picture && packet->flags == 1 && start == false){
			//	start = true;
			//}

			//if (start){
				/*int videoPts = 0;
				if (packet->stream_index == h264Source->m_videoIndex){
					videoPts = videoTimestamp;
					videoTimestamp += 40;

					int64_t now_time = av_gettime() - start_time;
					if (videoPts > now_time)
						av_usleep(videoPts - now_time);

				}

				packet->pts = videoPts;
				packet->dts = packet->pts;
				packet->duration = 40;
				packet->pos = -1;*/
				//timestamp += 40;
			//}
		}
		av_free_packet(packet);
	}
	//写文件尾（Write file trailer）
	av_write_trailer(ofmt_ctx);
	/* close output */
	if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
		avio_close(ofmt_ctx->pb);
	avformat_free_context(ofmt_ctx);
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

	bool result = ReadFirstNaluFromBuf(h264Source, naluUnit, packetSize, packetData);
	if (!result || naluUnit.type != 7){
		return FALSE;
	}
	h264Source->m_metaData->nSpsLen = naluUnit.size;
	h264Source->m_metaData->Sps = NULL;
	h264Source->m_metaData->Sps = (unsigned char*)malloc(naluUnit.size);
	memcpy(h264Source->m_metaData->Sps, naluUnit.data, naluUnit.size);

	// 读取PPS帧
	result = ReadOneNaluFromBuf(h264Source, naluUnit, packetSize, packetData);
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
int ReadFirstNaluFromBuf(jerky_h264_source* h264Source, NaluUnit &nalu, int packetSize, unsigned char *packetData){
	int naltail_pos = h264Source->nalhead_pos;

	memset(m_pFileBuf_tmp, 0, BUFFER_SIZE);
	while (h264Source->nalhead_pos < packetSize)
	{
		//search for nal header
		if (packetData[h264Source->nalhead_pos++] == 0x00 &&
			packetData[h264Source->nalhead_pos++] == 0x00)
		{
			if (packetData[h264Source->nalhead_pos++] == 0x01)
				goto gotnal_head;
			else
			{
				//cuz we have done an i++ before,so we need to roll back now
				h264Source->nalhead_pos--;
				if (packetData[h264Source->nalhead_pos++] == 0x00 &&
					packetData[h264Source->nalhead_pos++] == 0x01)
					goto gotnal_head;
				else
					continue;
			}
		}
		else
			continue;

		//search for nal tail which is also the head of next nal
	gotnal_head:
		//normal case:the whole nal is in this m_pFileBuf
		naltail_pos = h264Source->nalhead_pos;
		while (naltail_pos<packetSize)
		{
			if (packetData[naltail_pos++] == 0x00 &&
				packetData[naltail_pos++] == 0x00)
			{
				if (packetData[naltail_pos++] == 0x01)
				{
					nalu.size = (naltail_pos - 3) - h264Source->nalhead_pos;
					break;
				}
				else
				{
					naltail_pos--;
					if (packetData[naltail_pos++] == 0x00 &&
						packetData[naltail_pos++] == 0x01)
					{
						nalu.size = (naltail_pos - 4) - h264Source->nalhead_pos;
						break;
					}
				}
			}
		}

		nalu.type = packetData[h264Source->nalhead_pos] & 0x1f;
		memcpy(m_pFileBuf_tmp, packetData + h264Source->nalhead_pos, nalu.size);
		nalu.data = m_pFileBuf_tmp;
		h264Source->nalhead_pos = naltail_pos;
		return TRUE;
	}
}

int ReadOneNaluFromBuf(jerky_h264_source* h264Source, NaluUnit &nalu, int packetSize, unsigned char *packetData)
{

	int naltail_pos = h264Source->nalhead_pos;
	int ret;
	int nalustart;//nal的开始标识符是几个00
	memset(m_pFileBuf_tmp, 0, BUFFER_SIZE);
	nalu.size = 0;
	while (1)
	{
		if (h264Source->nalhead_pos == NO_MORE_BUFFER_TO_READ)
			return FALSE;
		while (naltail_pos<packetSize)
		{
			//search for nal tail
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
					//cuz we have done an i++ before,so we need to roll back now
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
			/**
			*special case1:parts of the nal lies in a m_pFileBuf and we have to read from buffer
			*again to get the rest part of this nal
			*/
			if (h264Source->nalhead_pos == GOT_A_NAL_CROSS_BUFFER || h264Source->nalhead_pos == GOT_A_NAL_INCLUDE_A_BUFFER)
			{				
				return FALSE;
			}
			//normal case:the whole nal is in this m_pFileBuf
			else
			{
				nalu.type = packetData[h264Source->nalhead_pos] & 0x1f;
				nalu.size = naltail_pos - h264Source->nalhead_pos - nalustart;
				if (nalu.type == 0x06)
				{
					h264Source->nalhead_pos = naltail_pos;
					continue;
				}
				memcpy(m_pFileBuf_tmp, packetData + h264Source->nalhead_pos, nalu.size);
				nalu.data = m_pFileBuf_tmp;
				h264Source->nalhead_pos = naltail_pos;
				return TRUE;
			}
		}
	}
	return FALSE;
}


int main(int argc, char* argv[]){
	jerky_h264_source* h264Source = jerky_h264_source_init();
	std::thread h264SourceThread(jerky_h264_source_thread, h264Source);
	h264SourceThread.join();	

	system("pause");
	return 0;
}
