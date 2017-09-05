#include "resource_mgt.h"
#include "cJSON.h"
#include "url_encode.h"
#include "utstring.h"
#include "protocol.h"
#include "log.h"
#include "conf.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>  //sleep

#include <pthread.h>
#include <conn_pool.h>

//lock for node pool
static pthread_mutex_t nodes_pool_lock;
//pool of nodes
static node_resource * nodes_resource_pool;


//lock for service pool
static pthread_mutex_t services_pool_lock;
//pool of services
static service_resource * services_resource_pool;


/******************************node_resource***********************************/

static int node_resource_lock() {
    return (pthread_mutex_lock(&nodes_pool_lock) == 0) ? 1 : 0;
}

static int node_resource_unlock() {
    return (pthread_mutex_unlock(&nodes_pool_lock) == 0) ? 1 : 0;
}

int test_conn_func_impl(struct _conn_obj * conn) {
    int success = 1;
    uint32_t identifier = next_identifier();
    if (!send_message(conn->sock_fd, identifier, HB_REQ_MSG_TYPE, NULL, 0)) {
        uint32_t res_identifier;
        uint8_t res_type;
        if (!receive_message(conn->sock_fd, &res_identifier, &res_type, NULL, 0)) {
            if (res_identifier != identifier) {
                success = 0;
            }
            if (res_type != HB_RES_MSG_TYPE) {
                success = 0;
            }
        } else {
            success = 0;
        }
    } else {
        success = 0;
    }
    if (success) {
        INFO("conn %s:%d is alive", conn->host, conn->port);
    } else {
        INFO("conn %s:%d is broken", conn->host, conn->port);
    }
    return success;
}

static void * conn_check_thd_func(void * arg) {
    INFO("node resource checker thread startup ... ");
    while (1) {
        sleep(60);
        DEBUG("conn_check_thd_func begin ... ");
        node_resource_lock();
        node_resource *current, *tmp;
        HASH_ITER(hh, nodes_resource_pool, current, tmp) {
            pool_check(&current->pool, test_conn_func_impl);
        }
        node_resource_unlock();
        DEBUG("conn_check_thd_func end ");
    }
}

static void * pool_destroy_thd_func(void * arg) {
    node_resource * node = (node_resource *)arg;
    char node_name[30];
    snprintf(node_name, 30, "%s:%d", node->pool.host, node->pool.port);
    INFO("pool_destroy begin : %s", node_name);
    int retry = 10 * 5;
    int success = 0;
    while (retry-- > 0) {
        if (pool_destroy(&node->pool) < 0) {
            WARN("pool_destroy try failed (%d) : %s", retry, node_name);
            sleep(6);
        } else {
            free(node);
            success = 1;
            break;
        }
    }
    INFO("pool_destroy end (%s) : %s", ((success) ? "success" : "failed"), node_name);
}

int node_resource_init() {
    nodes_resource_pool = NULL;
    pthread_mutex_init(&nodes_pool_lock, NULL);
    //startup checker thread
    //No check the validation of connections of pool, but need to preheat connections of pool
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, (void*)conn_check_thd_func, NULL);
    if (ret) {
        ERROR("create node resource checker thread error : % s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int node_resource_destroy() {
    node_resource_lock();
    node_resource *current, *tmp;
    HASH_ITER(hh, nodes_resource_pool, current, tmp) {
        HASH_DEL(nodes_resource_pool, current);
        pool_destroy(&current->pool);
        free(current);
    }
    node_resource_unlock();
    pthread_mutex_destroy(&nodes_pool_lock);
    return 0;
}

int node_resource_add(const char * host, int port) {
    node_resource * node = node_resource_get(host, port);
    if (!node) {
        node = malloc(sizeof(node_resource));
        node_resource_construct_key(host, port, node->node_key, NODE_KEY_LEN);
        pool_init(&node->pool, host, port,
                  get_config_item_int("pool:timeout_secs", 3),
                  get_config_item_int("pool:init_conn_num", 1),
                  get_config_item_int("pool:min_conn_num", 1),
                  get_config_item_int("pool:max_conn_num", 6),
                  get_config_item_int("pool:max_idle_secs", 60));
        node_resource_lock();
        HASH_ADD_STR(nodes_resource_pool, node_key, node);
        node_resource_unlock();
    }
    return 0;
}

int node_resource_delete(const char * host, int port) {
    node_resource * node = node_resource_get(host, port);
    if (node) {
        node_resource_lock();
        HASH_DEL(nodes_resource_pool, node);
        node_resource_unlock();
        //destroy pool
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, (void*)pool_destroy_thd_func, node);
        if (ret) {
            ERROR("create pool_destroy_thd_func thread error : % s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}

node_resource * node_resource_get(const char * host, int port) {
    char node_key[NODE_KEY_LEN];
    node_resource_construct_key(host, port, node_key, NODE_KEY_LEN);
    return node_resource_get_by_key(node_key);
}

node_resource * node_resource_get_by_key(const char * node_key) {
    if (!node_key) {
        return NULL;
    }
    node_resource_lock();
    node_resource * node = NULL;
    HASH_FIND_STR(nodes_resource_pool, node_key, node);
    node_resource_unlock();
    return node;
}

conn_obj * node_require_connection(const char * node_key) {
    if (!node_key) {
        return NULL;
    }
    conn_obj * conn = NULL;
    node_resource * node = NULL;
    node_resource_lock();
    HASH_FIND_STR(nodes_resource_pool, node_key, node);
    if (node) {
        conn = pool_require_connection(&node->pool);
    }
    node_resource_unlock();
    return conn;
}

void node_release_connection(const char * node_key, conn_obj * conn) {
    if (!node_key || !conn) {
        return;
    }
    node_resource * node = NULL;
    node_resource_lock();
    HASH_FIND_STR(nodes_resource_pool, node_key, node);
    if (node) {
        pool_release_connection(&node->pool, conn, 1);
    }
    node_resource_unlock();
    return conn;
}

int node_resource_status(char * buffer, size_t buf_len) {
    UT_string *s;
    utstring_new(s);
    node_resource_lock();
    node_resource *current, *tmp;
    HASH_ITER(hh, nodes_resource_pool, current, tmp) {
        char buf[100];
        pool_status(&current->pool, buf, 100);
        utstring_printf(s, "pool : %s, status : %s || ", current->node_key, buf);
    }
    node_resource_unlock();
    snprintf(buffer, buf_len, "%s", utstring_body(s));
    utstring_free(s);
    return 0;
}

/******************************service_resource***********************************/

static int service_resource_lock() {
    return (pthread_mutex_lock(&services_pool_lock) == 0) ? 1 : 0;
}

static int service_resource_unlock() {
    return (pthread_mutex_unlock(&services_pool_lock) == 0) ? 1 : 0;
}


int service_resource_init() {
    services_resource_pool = NULL;
    pthread_mutex_init(&services_pool_lock, NULL);
    return 0;
}

int service_resource_destroy() {
    service_resource_lock();
    service_resource *current, *tmp;
    HASH_ITER(hh, services_resource_pool, current, tmp) {
        HASH_DEL(services_resource_pool, current);
        string_array_destroy(&current->node_key_array);
        free(current);
    }
    service_resource_unlock();
    pthread_mutex_destroy(&services_pool_lock);
    return 0;
}

int service_resource_add(const char * group, const char * service, const char * version, const char * host, int port) {
    service_resource * service_item = service_resource_get(group, service, version);
    char node_key[NODE_KEY_LEN];
    node_resource_construct_key(host, port, node_key, NODE_KEY_LEN);
    if (service_item) {
        service_resource_lock();
        string_array_add(&service_item->node_key_array, node_key);
        service_resource_unlock();
    } else {
        service_item = malloc(sizeof(service_resource));
        service_resource_construct_key(group, service, version, service_item->service_key, SERVICE_KEY_LEN);
        string_array_init(&service_item->node_key_array);
        string_array_add(&service_item->node_key_array, node_key);
        service_resource_lock();
        HASH_ADD_STR(services_resource_pool, service_key, service_item);
        service_resource_unlock();
    }
    return 0;
}

int service_resource_delete(const char * group, const char * service, const char * version, const char * host, int port) {
    service_resource * service_item = service_resource_get(group, service, version);
    if (service_item) {
        service_resource_lock();
        char node_key[NODE_KEY_LEN];
        node_resource_construct_key(host, port, node_key, NODE_KEY_LEN);
        string_array_delete(&service_item->node_key_array, node_key);
        if (string_array_size(&service_item->node_key_array) == 0) {
            HASH_DEL(services_resource_pool, service_item);
            free(service_item);
        }
        service_resource_unlock();
    }
    return 0;
}

service_resource * service_resource_get(const char * group, const char * service, const char * version) {
    char service_key[SERVICE_KEY_LEN];
    service_resource_construct_key(group, service, version, service_key, SERVICE_KEY_LEN);
    return service_resource_get_by_key(service_key);
}

service_resource * service_resource_get_by_key(const char * service_key) {
    service_resource_lock();
    service_resource * service = NULL;
    HASH_FIND_STR(services_resource_pool, service_key, service);
    service_resource_unlock();
    return service;
}

const char * service_resource_require_node_key(const char * group, const char * service, const char * version) {
    service_resource * service_item = service_resource_get(group, service, version);
    if (service_item == NULL) {
        return NULL;
    }
    service_resource_lock();
    const char * pool_key = string_array_next(&service_item->node_key_array);
    service_resource_unlock();
    return pool_key;
}

int service_resource_status(char * buffer, size_t buf_len) {
    UT_string *s;
    utstring_new(s);
    service_resource_lock();
    service_resource *current, *tmp;
    HASH_ITER(hh, services_resource_pool, current, tmp) {
        char buf[100];
        string_array_content(&current->node_key_array, buf, 100);
        utstring_printf(s, "service : %s, nodes : %s || ", current->service_key, buf);
    }
    service_resource_unlock();
    snprintf(buffer, buf_len, "%s", utstring_body(s));
    utstring_free(s);
    return 0;
}

/**
 * judge if the pool is used
 * @param  host [description]
 * @param  port [description]
 * @return      1 means used, 0 means unused
 */
int service_resource_pool_is_used(const char * host, int port) {
    char node_key[NODE_KEY_LEN];
    node_resource_construct_key(host, port, node_key, NODE_KEY_LEN);
    int used = 0;
    service_resource_lock();
    service_resource *current, *tmp;
    HASH_ITER(hh, services_resource_pool, current, tmp) {
        int idx = string_array_find(&current->node_key_array, node_key);
        if (idx != -1) {
            //found
            used = 1;
            break;
        }
    }
    service_resource_unlock();
    return used;
}

int resource_add(const char * group, const char * service, const char * version, const char * host, int port) {
    INFO("======resource_add : %s, %s, %s, %s, %d", group, service, version, host, port);
    node_resource_add(host, port);
    service_resource_add(group, service, version, host, port);
    return 0;
}

int resource_delete(const char * group, const char * service, const char * version, const char * host, int port) {
    INFO("======resource_delete : %s, %s, %s, %s, %d", group, service, version, host, port);
    service_resource_delete(group, service, version, host, port);
    if (!service_resource_pool_is_used(host, port)) {
        INFO("======delete pool : %s, %d", host, port);
        node_resource_delete(host, port);
    }
    return 0;
}
