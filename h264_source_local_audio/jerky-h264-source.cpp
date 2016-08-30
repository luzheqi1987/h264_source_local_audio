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

	while (!h264Source->m_stop && av_read_frame(h264Source->m_formatCtx, packet) >= 0){
		printf("Receive Packet, Packet Stream Index Is: %d, Frame Type: %d \n", packet->stream_index, packet->flags);
	}
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
	int nalustart;//nal的开始标识符是几个00
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
