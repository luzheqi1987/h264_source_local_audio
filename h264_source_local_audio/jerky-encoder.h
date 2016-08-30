#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Encoder output packet */
struct encoder_packet {
	uint8_t               *data;        /**< Packet data */
	size_t                size;         /**< Packet size */

	int64_t               pts;          /**< Presentation timestamp */
	int64_t               dts;          /**< Decode timestamp */

	int32_t               timebase_num; /**< Timebase numerator */
	int32_t               timebase_den; /**< Timebase denominator */

	enum obs_encoder_type type;         /**< Encoder type */

	bool                  keyframe;     /**< Is a keyframe */

	/* ---------------------------------------------------------------- */
	/* Internal video variables (will be parsed automatically) */

	/* DTS in microseconds */
	int64_t               dts_usec;

	/* System DTS in microseconds */
	int64_t               sys_dts_usec;

	/**
	* Packet priority
	*
	* This is generally use by video encoders to specify the priority
	* of the packet.
	*/
	int                   priority;

	/**
	* Dropped packet priority
	*
	* If this packet needs to be dropped, the next packet must be of this
	* priority or higher to continue transmission.
	*/
	int                   drop_priority;

	/** Audio track index (used with outputs) */
	size_t                track_idx;

	/** Encoder from which the track originated from */
	//obs_encoder_t         *encoder;
};

void obs_free_encoder_packet(struct encoder_packet *packet);

#ifdef __cplusplus
}
#endif