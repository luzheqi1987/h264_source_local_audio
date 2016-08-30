
#include <inttypes.h>
#include "librtmp/rtmp.h"
#include "librtmp/log.h"
#include "jerky-circlebuf.h"
#include "w32-pthreads/pthread.h"

struct dstr {
	char *array;
	size_t len; 
};

struct rtmp_stream {
	struct jerky_circlebuf packets;
	bool             sent_headers;

	volatile bool    connecting;

	volatile bool    active;
	volatile bool    disconnected;
	pthread_t        send_thread;

	int              max_shutdown_time_sec;


	struct dstr      path, key;
	struct dstr      username, password;
	struct dstr      encoder_name;
	struct dstr      bind_ip;

	int64_t          last_dts_usec;

	uint64_t         total_bytes_sent;
	int              dropped_frames;
	RTMP             rtmp;
};