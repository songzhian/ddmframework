#ifndef __LOG_H__
#define __LOG_H__

#include <stdarg.h>

#include <zlog.h>

#ifdef __cplusplus
extern "C" {
#endif


extern zlog_category_t *zc;


/*** the following codes are just for compiling, copied from zlog's sources ***/
#define MAXLEN_PATH 1024
typedef zc_arraylist_t;
typedef struct zlog_category_s {
	char name[MAXLEN_PATH + 1];
	size_t name_len;
	unsigned char level_bitmap[32];
	unsigned char level_bitmap_backup[32];
	zc_arraylist_t *fit_rules;
	zc_arraylist_t *fit_rules_backup;
} zlog_category_t;


#define my_zlog_category_needless_level(a_category, lv) \
        !((a_category->level_bitmap[lv/8] >> (7 - lv % 8)) & 0x01)


/**
 * [log_init description]
 * @param  config_file_path [description]
 * @return                  [description]
 */
int log_init(const char * config_file_path );



/**
** macro functions for text message
**/
#define DEBUG(fmt, args...)  \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_DEBUG)) { \
  } else {  \
  	zlog_debug(zc, fmt, ##args); \
  }

#define INFO(fmt, args...)    \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_INFO)) { \
  } else {  \
  	zlog_info(zc, fmt, ##args); \
  }

#define NOTICE(fmt, args...)  \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_NOTICE)) { \
  } else {  \
  	zlog_notice(zc, fmt, ##args); \
  }

#define WARN(fmt, args...)    \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_WARN)) { \
  } else {  \
  	zlog_warn(zc, fmt, ##args); \
  }

#define ERROR(fmt, args...)    \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_ERROR)) { \
  } else {  \
  	zlog_error(zc, fmt, ##args); \
  }

#define FATAL(fmt, args...)   \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_FATAL)) { \
  } else {  \
  	zlog_fatal(zc, fmt, ##args); \
  }


/**
** macro functions for binary message
**/
#define HEXDEBUG(msg,len)   \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_DEBUG)) { \
  } else {  \
  	hzlog_debug(zc, msg, len); \
  }

#define HEXINFO(msg,len)    \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_INFO)) { \
  } else {  \
  	hzlog_info(zc, msg, len); \
  }

#define HEXNOTICE(msg,len)  \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_NOTICE)) { \
  } else {  \
  	hzlog_notice(zc, msg, len); \
  }

#define HEXWARN(msg,len)    \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_WARN)) { \
  } else {  \
  	hzlog_warn(zc, msg, len); \
  }

#define HEXERROR(msg,len)   \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_ERROR)) { \
  } else {  \
  	hzlog_error(zc, msg, len); \
  }

#define HEXFATAL(msg,len)   \
  if (zc && my_zlog_category_needless_level(zc, ZLOG_LEVEL_FATAL)) { \
  } else {  \
  	hzlog_fatal(zc, fmt, ##args); \
  }


#ifdef __cplusplus
}
#endif

#endif