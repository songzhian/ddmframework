#ifndef __CONF_H__
#define __CONF_H__

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

// #include "uthash.h"

// #define CONFIG_ITEM_NAME_LEN    256
// #define CONFIG_ITEM_VALUE_LEN   512

// typedef struct _config_map_item {
// 	char config_item_name[CONFIG_ITEM_NAME_LEN];    key, format is [section name]/[config item name]
// 	char config_item_value[CONFIG_ITEM_VALUE_LEN];
// 	UT_hash_handle hh;         /* makes this structure hashable */
// } config_map_item;

/**
 * [config_init description]
 * @param  config_file_path [description]
 * @return                  [0 means success, -1 means failed]
 */
int config_init(const char * config_file_path);

/**
 * [get_config_item description]
 * @param  config_item_name [description]
 * @param  result           [description]
 * @param  len              [description]
 * @param  default_value    [description]
 * @return                  [0 means success, -1 means failed]
 */
int get_config_item(const char * config_item_name, char * result, size_t len, const char * default_value);

/**
 * [get_config_item_int description]
 * @param  config_item_name [description]
 * @param  default_value    [description]
 * @return                  [real value or default value]
 */
int get_config_item_int(const char * config_item_name, int default_value);


#ifdef __cplusplus
}
#endif

#endif