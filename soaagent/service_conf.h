#ifndef __SERVICE_CONF_H__
#define __SERVICE_CONF_H__

#include "uthash.h"
#include "utarray.h"

#include <stdint.h>  //uint32_t


#ifdef __cplusplus
extern "C" {
#endif

#define FIELD_HOST_LEN     20
#define FIELD_GROUP_LEN    50
#define FIELD_SERVICE_LEN  30
#define FIELD_VERSION_LEN  20

#define SERVICE_KEY_LEN  (FIELD_GROUP_LEN+FIELD_SERVICE_LEN+FIELD_VERSION_LEN)
#define NODE_KEY_LEN     (FIELD_HOST_LEN+6)
#define PROVIDER_KEY_LEN (FIELD_GROUP_LEN+10)

#define KEY_SEPARATOR   "#"

int node_resource_construct_key(const char * host, int port, char * node_key, size_t len);

int service_resource_construct_key(const char * group, const char * service, const char * version, char * service_key, size_t len);

int provider_construct_key(const char * group, char * provider_key, size_t len);


//service info
typedef struct _service_info
{
    char group_value[FIELD_GROUP_LEN];
    char service_value[FIELD_SERVICE_LEN];
    char version_value[FIELD_VERSION_LEN];
} service_info;

int service_filter_init();

int service_filter_destroy();

int service_filter_padding(service_info * srv_info, const char * group, const char * service, const char * version);

/**
 * [service_filter_add description]
 * @param  srv_info [description]
 * @return          0 means success, -1 means failed
 */
int service_filter_add(service_info * srv_info);

/**
 * [service_filter_find description]
 * @param  srv_info [description]
 * @return          1 means found, 0 means not found
 */
int service_filter_find(service_info * srv_info);


//register info
typedef struct _register_info
{
    service_info srv_info;
    char host_value[FIELD_HOST_LEN];
    int port_value;
    int status;   /*** 0 means normal, 1 means add, -1 means delete ***/
} register_info;

int register_info_padding(register_info * reg_info, const char * group, const char * service, const char * version, const char * host, int port);

/**
 * [service_filter_match description]
 * @param  reg_info [description]
 * @return          1 means match, 0 means not match
 */
int service_filter_match(register_info * reg_info);


typedef struct _register_info_array
{
    UT_array * array;
} register_info_array;

int parse_node_register_info(register_info_array ** result, const char * content);

int find_register_info(register_info_array * result, register_info * reg_info);

int copy_register_info(register_info_array * src, register_info_array * dest, int status);

register_info_array * diff_register_info(register_info_array * left_array, register_info_array * right_array);

int destroy_register_info_array(register_info_array * array);


//register map item
typedef struct _register_map_item {
    char provider_key[PROVIDER_KEY_LEN];   /* key, format is [provider name] */
    register_info_array * info_array;
    UT_hash_handle hh;         /* makes this structure hashable */
} register_map_item;


int register_mapping_init();

int register_mapping_destroy();

register_map_item * register_mapping_find_item(const char * provider_key);

register_info_array * register_mapping_find(const char * provider_key);

int register_mapping_put(const char * provider_key, register_info_array * info_array);


#ifdef __cplusplus
}
#endif

#endif