#include "jerky-h264-source.h"
#include <thread>
#include <cstdlib>
#include "jerky-circlebuf.h"
#include <windows.h>

unsigned char* m_pFileBuf_tmp = (unsigned char*)malloc(BUFFER_SIZE);

struct jerky_h264_source* jerky_h264_source_init()
{
	struct jerky_h264_source *h264Source = (jerky_h264_source *)malloc(sizeof(h264Source));

	av_register_all();
	avformat_network_init();
	h264Source->m_formatCtx = avformat_alloc_context();
	h264Source->m_streamUrl = "rtsp://192.168.5.1:8557/h264";
	h264Source->m_stop = false;
	h264Source->m_videoIndex = -1;
	h264Source->m_metaData = NULL;

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
	return h264Source;
}

void jerky_h264_source_thread(void *args){
	struct jerky_h264_source *h264Source = (struct jerky_h264_source *) args;
	AVPacket *packet = packet = (AVPacket *)av_malloc(sizeof(AVPacket));
	h264Source->find_video_index();
	bool gotPicture = false;

	while (!h264Source->m_stop && av_read_frame(h264Source->m_formatCtx, packet) >= 0){
		printf("Receive Packet, Packet Stream Index Is: %d, Frame Type: %d \n", packet->stream_index, packet->flags);
		if (packet->stream_index == h264Source->m_videoIndex){
			if (packet->flags && !h264Source->m_metaData){

			}
		}
	}
}

int jerky_h264_source::fetchSpsPps(AVPacket *packet){
	AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
	unsigned char * packetData = (unsigned char*)malloc(packet->size);
	NaluUnit naluUnit;
	this->m_metaData = (RTMPMetadata *)malloc(sizeof(RTMPMetadata));

	memcpy(packetData, packet->data, packet->size);
	int packetSize = packet->size;
	if (packet->size == 0){
		return FALSE;
	}

	bool result = ReadFirstNaluFromBuf(naluUnit, packetSize, packetData);
	if (!result || naluUnit.type != 7){
		return FALSE;
	}
	this->m_metaData->nSpsLen = naluUnit.size;
	this->m_metaData->Sps = NULL;
	this->m_metaData->Sps = (unsigned char*)malloc(naluUnit.size);
	memcpy(this->m_metaData->Sps, naluUnit.data, naluUnit.size);

	// ��ȡPPS֡
	result = ReadOneNaluFromBuf(naluUnit, packetSize, packetData);
	if (!result || naluUnit.type != 8){
		return FALSE;
	}
	this->m_metaData->nPpsLen = naluUnit.size;
	this->m_metaData->Pps = NULL;
	this->m_metaData->Pps = (unsigned char*)malloc(naluUnit.size);
	memcpy(this->m_metaData->Pps, naluUnit.data, naluUnit.size);

	return TRUE;
}

int jerky_h264_source::find_video_index()
{	
	for (unsigned int i = 0; i < this->m_formatCtx->nb_streams; i++) {
		if (this->m_formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
			this->m_videoIndex = i;
			break;
		}
	}

	return this->m_videoIndex;
}

/**
* ���ڴ��ж�ȡ����һ��Nal��Ԫ
*
* @param nalu �洢nalu����
* @param read_buffer �ص������������ݲ����ʱ��ϵͳ���Զ����øú�����ȡ�������ݡ�
*					2���������ܣ�
*					uint8_t *buf���ⲿ���������õ�ַ
*					int buf_size���ⲿ���ݴ�С
*					����ֵ���ɹ���ȡ���ڴ��С
* @�ɹ��򷵻� 1 , ʧ���򷵻�0
*/
int jerky_h264_source::ReadFirstNaluFromBuf(NaluUnit &nalu, int packetSize, unsigned char *packetData){
	int naltail_pos = this->nalhead_pos;

	memset(m_pFileBuf_tmp, 0, BUFFER_SIZE);
	while (this->nalhead_pos < packetSize)
	{
		//search for nal header
		if (packetData[this->nalhead_pos++] == 0x00 &&
			packetData[this->nalhead_pos++] == 0x00)
		{
			if (packetData[this->nalhead_pos++] == 0x01)
				goto gotnal_head;
			else
			{
				//cuz we have done an i++ before,so we need to roll back now
				this->nalhead_pos--;
				if (packetData[this->nalhead_pos++] == 0x00 &&
					packetData[this->nalhead_pos++] == 0x01)
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
		naltail_pos = this->nalhead_pos;
		while (naltail_pos<packetSize)
		{
			if (packetData[naltail_pos++] == 0x00 &&
				packetData[naltail_pos++] == 0x00)
			{
				if (packetData[naltail_pos++] == 0x01)
				{
					nalu.size = (naltail_pos - 3) - this->nalhead_pos;
					break;
				}
				else
				{
					naltail_pos--;
					if (packetData[naltail_pos++] == 0x00 &&
						packetData[naltail_pos++] == 0x01)
					{
						nalu.size = (naltail_pos - 4) - this->nalhead_pos;
						break;
					}
				}
			}
		}

		nalu.type = packetData[this->nalhead_pos] & 0x1f;
		memcpy(m_pFileBuf_tmp, packetData + this->nalhead_pos, nalu.size);
		nalu.data = m_pFileBuf_tmp;
		this->nalhead_pos = naltail_pos;
		return TRUE;
	}
}

int jerky_h264_source::ReadOneNaluFromBuf(NaluUnit &nalu, int packetSize, unsigned char *packetData)
{

	int naltail_pos = this->nalhead_pos;
	int ret;
	int nalustart;//nal�Ŀ�ʼ��ʶ���Ǽ���00
	memset(m_pFileBuf_tmp, 0, BUFFER_SIZE);
	nalu.size = 0;
	while (1)
	{
		if (this->nalhead_pos == NO_MORE_BUFFER_TO_READ)
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
			if (this->nalhead_pos == GOT_A_NAL_CROSS_BUFFER || this->nalhead_pos == GOT_A_NAL_INCLUDE_A_BUFFER)
			{				
				return FALSE;
			}
			//normal case:the whole nal is in this m_pFileBuf
			else
			{
				nalu.type = packetData[this->nalhead_pos] & 0x1f;
				nalu.size = naltail_pos - this->nalhead_pos - nalustart;
				if (nalu.type == 0x06)
				{
					this->nalhead_pos = naltail_pos;
					continue;
				}
				memcpy(m_pFileBuf_tmp, packetData + this->nalhead_pos, nalu.size);
				nalu.data = m_pFileBuf_tmp;
				this->nalhead_pos = naltail_pos;
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
