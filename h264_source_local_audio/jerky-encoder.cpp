#include "jerky-encoder.h"
#include "util/bmem.h"

void obs_free_encoder_packet(struct encoder_packet *packet)
{
	bfree(packet->data);
	memset(packet, 0, sizeof(struct encoder_packet));
}