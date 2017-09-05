#include "log.h"


#include <string.h>  //strcmp
#include <stdio.h>
#include <errno.h>

#include <zlog.h>

zlog_category_t *zc;
static int global_inited = 0;

/**
 * [log_init description]
 * @param  config_file_path [description]
 * @return                  [description]
 */
int log_init(const char * config_file_path ) {
	if (global_inited) return 0;
	int rc = zlog_init(config_file_path);
	if ( rc ) {
		fprintf(stderr, "zlog_init failed : %d\n", rc);
		return -1;
	}
	zc = zlog_get_category("my_cat");
	if (!zc) {
		fprintf(stderr, "zlog_get_category failed : %d\n", errno);
		zlog_fini();
		return -1;
	}
	global_inited = 1;
	return 0;
}


