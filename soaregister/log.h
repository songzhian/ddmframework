#ifndef _LOG_H_
#define _LOG_H_

#include <inttypes.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>


#ifdef __cplusplus
extern "C" {
#endif


#define LEVEL_NAME_LEN 8
#define LOG_BUF_LEN 4096

enum {
	LOCK_NONE = 0,
	LOCK_BETWEEN_THREAD  = 1,
	LOCK_BETWEEN_PROCESS = 2,
};

enum {
	LEVEL_NONE = -1,
	LEVEL_MIN = 0,
	LEVEL_DEBUG = 0,
	LEVEL_INFO = 1,
	LEVEL_NOTICE = 2,
	LEVEL_WARN = 3,
	LEVEL_ERROR = 4,
	LEVEL_FATAL = 5,
	LEVEL_MAX = 5,
};

typedef struct _log_handle
{
	volatile int status;
	FILE *fp;
	char filenametpl[PATH_MAX];
	char filename[PATH_MAX];
	char buffer[LOG_BUF_LEN];
	long inode_value;
	int level;
	int threadsafe;
	pthread_mutex_t lock;
	uint64_t rotate_size;
	uint64_t w_curr;
	uint64_t w_total;
} log_handle;


extern log_handle g_log_h;


int get_locktype(const char *lock_type_name);


int get_level(const char *level_name);


int open_log(const char *filename, const char * levelname, const char * lock_type, uint64_t rotate_size);


int close_log();


void set_log_level(const char *levelname);


int logv(int level, const char *fmt, va_list ap);


int log_write(int level, const char *fmt, ...);



#define DEBUG(fmt, args...) \
	if (!g_log_h.status || g_log_h.level == LEVEL_NONE || LEVEL_DEBUG < g_log_h.level) { \
    } else { \
    	log_write(LEVEL_DEBUG, "%s(%d): " fmt, __FILE__, __LINE__, ##args);  \
    }

#define INFO(fmt, args...) \
	if (!g_log_h.status || g_log_h.level == LEVEL_NONE || LEVEL_INFO < g_log_h.level) { \
    } else { \
    	log_write(LEVEL_INFO, "%s(%d): " fmt, __FILE__, __LINE__, ##args);  \
    }

#define NOTICE(fmt, args...) \
	if (!g_log_h.status || g_log_h.level == LEVEL_NONE || LEVEL_NOTICE < g_log_h.level) { \
    } else { \
    	log_write(LEVEL_NOTICE, "%s(%d): " fmt, __FILE__, __LINE__, ##args);  \
    }

#define WARN(fmt, args...) \
	if (!g_log_h.status || g_log_h.level == LEVEL_NONE || LEVEL_WARN < g_log_h.level) { \
    } else { \
    	log_write(LEVEL_WARN, "%s(%d): " fmt, __FILE__, __LINE__, ##args);  \
    }

#define ERROR(fmt, args...) \
	if (!g_log_h.status || g_log_h.level == LEVEL_NONE || LEVEL_ERROR < g_log_h.level) { \
    } else { \
    	log_write(LEVEL_ERROR, "%s(%d): " fmt, __FILE__, __LINE__, ##args);  \
    }

#define FATAL(fmt, args...) \
	if (!g_log_h.status || g_log_h.level == LEVEL_NONE || LEVEL_FATAL < g_log_h.level) { \
    } else { \
    	log_write(LEVEL_FATAL, "%s(%d): " fmt, __FILE__, __LINE__, ##args);  \
    }


#define log_output(logger, level, fmt, args...) \
	if (!g_log_h.status || g_log_h.level == LEVEL_NONE || level < g_log_h.level) { \
    } else { \
    	log_write(level, "%s(%d) %s: " fmt, __FILE__, __LINE__, logger, ##args);  \
    }



#ifdef __cplusplus
}
#endif

#endif