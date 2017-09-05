#include "invoke_stub.h"

#include "zk_handle.h"
#include "resource_mgt.h"
#include "url_encode.h"
#include "cJSON.h"
#include "util.h"
#include "conn_pool.h"
#include "protocol.h"
#include "utarray.h"
#include "service_conf.h"
#include "log.h"
#include "str_array.h"

#include <sys/types.h> //size_t
#include <sys/time.h>  //timeval
#include <stdint.h>  //uint8_t,uint16_t,uint32_t,uint64_t
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <pthread.h>
#include <unistd.h>  //sleep


#define RETRY_TIMES   1

//lock for watch
static pthread_mutex_t global_watch_lock;

//lock for modify resource
static pthread_mutex_t global_modify_resource_lock;

//zk client
static zk_handle zk_h;

static volatile int g_inited = 0;

//all filters
static UT_array * watchers_context_array;

static time_t global_config_file_last_modified;

static int watch_lock() {
    return (pthread_mutex_lock(&global_watch_lock) == 0) ? 1 : 0;
}

static int watch_unlock() {
    return (pthread_mutex_unlock(&global_watch_lock) == 0) ? 1 : 0;
}

static int modify_resource_lock() {
    return (pthread_mutex_lock(&global_modify_resource_lock) == 0) ? 1 : 0;
}

static int modify_resource_unlock() {
    return (pthread_mutex_unlock(&global_modify_resource_lock) == 0) ? 1 : 0;
}

static void notify_change(register_info * reg_info) {
    if (reg_info->status == -1) {
        //delete
        resource_delete(reg_info->srv_info.group_value, reg_info->srv_info.service_value, reg_info->srv_info.version_value, reg_info->host_value, reg_info->port_value);
    } else if (reg_info->status == 1) {
        //add
        resource_add(reg_info->srv_info.group_value, reg_info->srv_info.service_value, reg_info->srv_info.version_value, reg_info->host_value, reg_info->port_value);
    }
}

static void children_change_func_impl(execute_type exe_type, const char * node_group, char ** node_names, int node_count) {
    modify_resource_lock();
    if (exe_type == EXE_FULL) {
        INFO("======node_group : %s -- FULL executed!", node_group);
    } else if (exe_type == EXE_INCREMENT) {
        INFO("======node_group : %s -- INCREMENT executed!", node_group);
    }
    register_info_array * remote_reg_info_arr = NULL;
    int i;
    for (i = 0; i < node_count; ++i) {
        char * node_value = node_names[i];
        char * debuf = url_decode(node_value, strlen(node_value));
        INFO("======changed node : %s", debuf);
        if (parse_node_register_info(&remote_reg_info_arr, debuf)) {
            ERROR("======parse_node_register_info failed : %s", debuf);
        }
        free(debuf);
    }
    ////
    register_info_array * local_reg_info_arr = register_mapping_find(node_group);
    register_info_array * diff_reg_info_arr = diff_register_info(local_reg_info_arr, remote_reg_info_arr);
    if (diff_reg_info_arr) {
        register_info * p = NULL;
        while ( (p = (register_info*)utarray_next(diff_reg_info_arr->array, p))) {
            notify_change(p);
        }
    }
    register_mapping_put(node_group, remote_reg_info_arr);
    destroy_register_info_array(local_reg_info_arr);
    destroy_register_info_array(diff_reg_info_arr);
    modify_resource_unlock();
}

static void my_zk_watcher_marshal(zhandle_t *zk, int type, int state, const char *path, void *context) {
    DEBUG(" -------------  type:%d, state: %d, path: [%s]", type, state, path);
    if (type == ZOO_CREATED_EVENT) {
        DEBUG(" -------------  ZOO_CREATED_EVENT event");
    } else if (type == ZOO_DELETED_EVENT) {
        DEBUG(" -------------  ZOO_DELETED_EVENT event");
    } else if (type == ZOO_CHANGED_EVENT) {
        DEBUG(" -------------  ZOO_CHANGED_EVENT event");
    } else if (type == ZOO_CHILD_EVENT) {
        DEBUG(" -------------  ZOO_CHILD_EVENT event");
    } else if (type == ZOO_SESSION_EVENT) {
        DEBUG(" -------------  ZOO_SESSION_EVENT event");
    } else if (type == ZOO_NOTWATCHING_EVENT) {
        DEBUG(" -------------  ZOO_NOTWATCHING_EVENT event");
    } else {
        DEBUG(" -------------  unknow event");
    }
    if (state == ZOO_EXPIRED_SESSION_STATE) {
        DEBUG(" -------------  ZOO_EXPIRED_SESSION_STATE state");
    } else if (state == ZOO_AUTH_FAILED_STATE) {
        DEBUG(" -------------  ZOO_AUTH_FAILED_STATE state");
    } else if (state == ZOO_CONNECTING_STATE) {
        DEBUG(" -------------  ZOO_CONNECTING_STATE state");
    } else if (state == ZOO_ASSOCIATING_STATE) {
        DEBUG(" -------------  ZOO_ASSOCIATING_STATE state");
    } else if (state == ZOO_CONNECTED_STATE) {
        DEBUG(" -------------  ZOO_CONNECTED_STATE state");
    } else {
        DEBUG(" -------------  unknow state");
    }
    if (type == ZOO_CHILD_EVENT) {
        change_context * cxt = (change_context *)context;
        cxt->exe_type = EXE_INCREMENT;
        get_children(&zk_h, my_zk_watcher_marshal, cxt);
    }
    if (type == ZOO_SESSION_EVENT && state == ZOO_EXPIRED_SESSION_STATE) {
        //session expired
        INFO("session expired");
        fini_zk_handle(&zk_h);
        if (init_zk_handle(&zk_h)) {
            ERROR("reconnect session failed");
        } else {
            INFO("reconnect session successfully");
            if (watch_declared_services(zk_h.config_file_path, 1)) {
                ERROR("rewatch declared services failed");
            } else {
                ERROR("rewatch declared services successfully");
            }
        }
    }
}

static void watch_group_change(const char * group_name) {
    INFO("========watch_group_change : %s", group_name);
    change_context chg_cxt;
    memset(&chg_cxt, 0, sizeof(change_context));
    chg_cxt.children_change_func = children_change_func_impl;
    chg_cxt.exe_type = EXE_FULL;
    provider_construct_key(group_name, chg_cxt.node_group_value, MAX_NODE_PATH_LEN);
    utarray_push_back(watchers_context_array, &chg_cxt);
    change_context * cxt = utarray_eltptr(watchers_context_array, utarray_len(watchers_context_array) - 1);
    get_children(&zk_h, my_zk_watcher_marshal, cxt);
}

/**
 * [watch_declared_services description]
 * @param  config_file_path [description]
 * @return                  [description]
 */
int watch_declared_services(const char * config_file_path, int force_refresh) {
    if (!is_path_exist(config_file_path)) {
        ERROR("======%s not found", config_file_path);
        return -1;
    }
    watch_lock();
    time_t curr_config_file_last_modified = get_file_last_modified(config_file_path);
    if (!force_refresh && curr_config_file_last_modified == global_config_file_last_modified) {
        watch_unlock();
        INFO("======config file not changed : %s", config_file_path);
        return 0;
    }
    INFO("======refresh config file begin : %s", config_file_path);
    //read services declare config file
    char * services_info_buffer = NULL;
    int rstat = read_content_from_file(config_file_path, &services_info_buffer, NULL);
    if (rstat) {
        watch_unlock();
        ERROR("======read services content failed");
        return -1;
    }
    DEBUG("======declared services = %s", services_info_buffer);
    cJSON *root = cJSON_Parse(services_info_buffer);
    if (!root) {
        free(services_info_buffer);
        watch_unlock();
        ERROR("======parse services content failed");
        return -1;
    }
    string_array group_name_array;
    string_array_init(&group_name_array);
    int array_len = cJSON_GetArraySize(root);
    int i, j, srv_len;
    service_info tmp_service_info;
    for (i = 0; i < array_len; ++i) {
        cJSON * define_item = cJSON_GetArrayItem(root, i);
        char * group_name = cJSON_GetObjectItem(define_item, "group")->valuestring;
        string_array_add(&group_name_array, group_name);
        cJSON * services_array = cJSON_GetObjectItem(define_item, "services");
        srv_len = cJSON_GetArraySize(services_array);
        for (j = 0; j < srv_len; ++j) {
            cJSON * service_item = cJSON_GetArrayItem(services_array, j);
            char * service_name = cJSON_GetObjectItem(service_item, "service")->valuestring;
            char * version_value = cJSON_GetObjectItem(service_item, "version")->valuestring;
            service_filter_padding(&tmp_service_info, group_name, service_name, version_value);
            //add declared services filter
            service_filter_add(&tmp_service_info);
        }
    }
    string_array_walk(&group_name_array, watch_group_change);
    string_array_destroy(&group_name_array);
    cJSON_Delete(root);
    free(services_info_buffer);
    global_config_file_last_modified = get_file_last_modified(config_file_path);
    INFO("======refresh config file done : %s", config_file_path);
    watch_unlock();
    return 0;
}

static void * config_file_change_checker_thread_func(void * arg) {
    INFO("config file change checker thread startup ...");
    while (1) {
        sleep(60);
        INFO("check config file's change : %s", zk_h.config_file_path);
        watch_declared_services(zk_h.config_file_path, 0);
    }
    INFO("config file change checker thread shutdown");
}

/**
 * [show_resource_status description]
 * @return [description]
 */
int show_resource_status() {
    INFO("======*******************resource status begin********************");
    char service_status[81920];
    service_resource_status(service_status, 81920);
    char pool_status[81920];
    node_resource_status(pool_status, 81920);
    INFO("======service_status = [%s]", service_status);
    INFO("======pool_status = [%s]", pool_status);
    INFO("======*******************resource status end********************");
    return 0;
}

/**
 * [refresh_declared_services description]
 * @return [description]
 */
int refresh_declared_services() {
    watch_declared_services(zk_h.config_file_path, 0);
    return 0;
}

static void * resource_monitor_thread_func(void * arg) {
    INFO("resource monitor thread startup ...");
    while (1) {
        sleep(45);
        show_resource_status();
    }
}

/**
 * [invoke_stub_init description]
 * @return [description]
 */
int invoke_stub_init(const char * zk_addrs, const char * config_file_path) {
    pthread_mutex_init(&global_watch_lock, NULL);
    pthread_mutex_init(&global_modify_resource_lock, NULL);
    UT_icd change_context_icd = {sizeof(change_context), NULL, NULL, NULL };
    utarray_new(watchers_context_array, &change_context_icd);
    memset(&zk_h, 0, sizeof(zk_h));
    snprintf(zk_h.zk_addrs, MAX_ZK_ADDR_LEN, "%s", zk_addrs);
    snprintf(zk_h.config_file_path, PATH_MAX, "%s", config_file_path);
    zk_h.callback = my_zk_watcher_marshal;
    zk_h.receive_timeout = 1000 * 30;
    if (init_zk_handle(&zk_h)) {
        ERROR("init zk failed");
        return -1;
    }
    INFO("init zk successfully");
    if (node_resource_init()) {
        ERROR("init node resource failed");
        return -1;
    }
    INFO("init node resource successfully");
    if (service_resource_init()) {
        ERROR("init service resource failed");
        return -1;
    }
    INFO("init service resource successfully");
    if (register_mapping_init()) {
        ERROR("register mapping init failed");
        return -1;
    }
    if (service_filter_init()) {
        ERROR("service filter init failed");
        return -1;
    }
    if (watch_declared_services(config_file_path, 1)) {
        ERROR("watch declared services failed");
        return -1;
    }
    INFO("watch declared services successfully");

    INFO("init invoke stub successfully");
    g_inited = 1;
    return 0;
}

/**
 * [invoke_stub_destroy description]
 * @return [description]
 */
int invoke_stub_destroy() {
    if (g_inited) {
        utarray_free(watchers_context_array);
        service_filter_destroy();
        fini_zk_handle(&zk_h);
        service_resource_destroy();
        node_resource_destroy();
        register_mapping_destroy();
        pthread_mutex_destroy(&global_watch_lock);
        pthread_mutex_destroy(&global_modify_resource_lock);
        INFO("destroy invoke stub successfully");
        g_inited = 0;
    }
    return 0;
}

/**
 * invoke service with preheating connections
 * @param  group       [description]
 * @param  service     [description]
 * @param  version     [description]
 * @param  req_body    [description]
 * @param  res_buf     [description]
 * @param  res_buf_len [description]
 * @return             0 means success, -1 means failed
 */
int invoke_service(const char * group, const char * service, const char * version, const void * req_body, size_t req_len, void ** res_buf, size_t * res_buf_len) {
    int return_code = 0;
    const char * node_key_const = service_resource_require_node_key(group, service, version);
    if (!node_key_const) {
        ERROR("not found node key for %s,%s,%s", group, service, version);
        return -1;
    }
    char node_key[30];
    snprintf(node_key, 30, "%s", node_key_const);  //need to copy because of free
    INFO("### using %s\n", node_key);
    node_resource * node = node_resource_get_by_key(node_key);
    if (!node) {
        ERROR("not found node for %s", node_key);
        return -1;
    }
    conn_obj * conn = pool_require_connection(&node->pool);
    if (!conn) {
        ERROR("cannot require conn from %s", node_key);
        return -1;
    }
    struct timeval start_time, end_time;
    uint32_t identifier = next_identifier();
    int retry = RETRY_TIMES + 1;
    get_now_time(&start_time);
    while (retry-- > 0) {
        int success = 0;
        if (!send_message(conn->sock_fd, identifier, BIZ_REQ_MSG_TYPE, req_body, req_len)) {
            DEBUG("send ok");
            uint32_t res_identifier;
            uint8_t res_type;
            if (!receive_message(conn->sock_fd, &res_identifier, &res_type, res_buf, res_buf_len)) {
                DEBUG("receive ok");
                if (res_identifier == identifier) {
                    if (res_type == BIZ_RES_MSG_TYPE) {
                        success = 1;
                    } else {
                        ERROR("type %x is wrong", res_type);
                    }
                } else {
                    ERROR("identifier %x is wrong", res_identifier);
                }
            } else {
                ERROR("receive failed");
            }
        } else {
            ERROR("send failed");
        }
        if (success) {
            break;
        } else {
            if (retry > 0) {
                if (conn_reconnect(conn)) {
                    ERROR("reconnect failed : %s", strerror(errno));
                    WARN("left %d retry times", retry);
                } else {
                    INFO("reconnect successfully");
                }
            } else {
                ERROR("invoke failed : still not done after retrying %d times", RETRY_TIMES);
                return_code = -1;
                break;
            }
        }
    }
    pool_release_connection(&node->pool, conn, 1);
    get_now_time(&end_time);
    unsigned int cost = get_time_difference(&start_time, &end_time);
    INFO("%s,%s,%s,%s invoke cost time (ms) : %d", group, service, version, node_key, cost);
    return return_code;
}

