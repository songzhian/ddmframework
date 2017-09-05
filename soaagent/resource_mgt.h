#ifndef __RESOURCE_MGT_H__
#define __RESOURCE_MGT_H__

#include "zk_handle.h"
#include "cJSON.h"
#include "uthash.h"
#include "utarray.h"
#include "conn_pool.h"
#include "service_conf.h"
#include "util.h"
#include "str_array.h"

#include <stdint.h>  //uint32_t

#ifdef __cplusplus
extern "C" {
#endif




/**
 * node struct (hashtable item)
 * TODO: how to check pool???
 */
typedef struct _node_resource {
	char node_key[NODE_KEY_LEN];   /* key, format is [node ip]#[node port] */
	pool_obj pool;             //the connection pool of the node
	UT_hash_handle hh;         /* makes this structure hashable */
} node_resource;

int test_conn_func_impl(struct _conn_obj * conn);

int node_resource_init();

int node_resource_destroy();

int node_resource_add(const char * host, int port);

int node_resource_delete(const char * host, int port);

node_resource * node_resource_get(const char * host, int port);

node_resource * node_resource_get_by_key(const char * node_key);

conn_obj * node_require_connection(const char * node_key);

void node_release_connection(const char * node_key, conn_obj * conn);

int node_resource_status(char * buffer, size_t buf_len);


/**
 * service struct (hashtable item)
 */
typedef struct _service_resource {
	char service_key[SERVICE_KEY_LEN];   /* key, format is [group name]#[service name]#[service version] */
	string_array node_key_array;
	UT_hash_handle hh;         /* makes this structure hashable */
} service_resource;


int service_resource_init();

int service_resource_destroy();

int service_resource_add(const char * group, const char * service, const char * version, const char * host, int port);

int service_resource_delete(const char * group, const char * service, const char * version, const char * host, int port);

service_resource * service_resource_get(const char * group, const char * service, const char * version);

service_resource * service_resource_get_by_key(const char * service_key);

const char * service_resource_require_node_key(const char * group, const char * service, const char * version);

int service_resource_status(char * buffer, size_t buf_len);

/**
 * judge if the pool is used
 * @param  host [description]
 * @param  port [description]
 * @return      1 means used, 0 means unused
 */
int service_resource_pool_is_used(const char * host, int port);

int resource_add(const char * group, const char * service, const char * version, const char * host, int port);

int resource_delete(const char * group, const char * service, const char * version, const char * host, int port);


#ifdef __cplusplus
}
#endif

#endif
