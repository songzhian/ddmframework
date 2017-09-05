#include "service_mgt.h"
#include "cJSON.h"
#include "url_encode.h"
#include "zk_handle.h"
#include "log.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


//zk client
static zk_handle zk_h;

static volatile int g_inited = 0;

/**
 * zookeeper watcher function
 * @param zk      [description]
 * @param type    [description]
 * @param state   [description]
 * @param path    [description]
 * @param context [description]
 */
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
    if (type == ZOO_SESSION_EVENT && state == ZOO_EXPIRED_SESSION_STATE) {
        //session expired
        INFO("session expired");
        fini_zk_handle(&zk_h);
        if (init_zk_handle(&zk_h)) {
            ERROR("reconnect session failed");
        } else {
            INFO("reconnect session successfully");
            if (register_services(&zk_h)) {
                ERROR("reregister failed");
            } else {
                INFO("reregister successfully");
            }
        }
    }
}

/**
 * [provider_stub_init description]
 * @return [description]
 */
int provider_stub_init(const char * zk_addrs, const char * config_file_path) {
    memset(&zk_h, 0, sizeof(zk_h));
    snprintf(zk_h.zk_addrs, MAX_ZK_ADDR_LEN, "%s", zk_addrs);
    snprintf(zk_h.config_file_path, PATH_MAX, "%s", config_file_path);
    zk_h.callback = my_zk_watcher_marshal;
    zk_h.receive_timeout = 1000 * 30;
    if (init_zk_handle(&zk_h)) {
        ERROR("init_zk_handle failed");
        return -1;
    }
    INFO("init_zk_handle successfully");
    if (register_services(&zk_h)) {
        return -1;
    }
    g_inited = 1;
    return 0;
}

/**
 * [provider_stub_destroy description]
 * @return [description]
 */
int provider_stub_destroy() {
    if (g_inited) {
        fini_zk_handle(&zk_h);
        INFO("provider destroy successfully");
        g_inited = 0;
    }
    return 0;
}

/**
 * register services
 * @param  services_define_file_path [file path of services define]
 * @param  zh                        [zk handle]
 * @return                           [return 0 means success , or failed]
 */
int register_services(zk_handle * zh) {
    char * services_info_buffer = NULL;
    int rstat = read_content_from_file(zh->config_file_path, &services_info_buffer, NULL);
    if (rstat) {
        ERROR("read services content failed");
        return -1;
    }
    DEBUG("services define : [%s]", services_info_buffer);
    cJSON *root = cJSON_Parse(services_info_buffer);
    if (!root) {
        ERROR("parse services content failed : %s", services_info_buffer);
        free(services_info_buffer);
        return -1;
    }
    char * group_name = cJSON_GetObjectItem(root, "group")->valuestring;
    char * encoded_services_info_buffer = url_encode(services_info_buffer, strlen(services_info_buffer));
    char node_path_value[MAX_NODE_PATH_LEN];
    snprintf(node_path_value, MAX_NODE_PATH_LEN, "/%s/providers/%s", group_name, encoded_services_info_buffer);
    int ret = make_sure_enode(zh, node_path_value, NULL);
    if (ret) {
        free(encoded_services_info_buffer);
        cJSON_Delete(root);
        free(services_info_buffer);
        ERROR("create node of services content failed");
        return -1;
    }
    //save node value into zk handle
    snprintf(zh->node_value, MAX_NODE_PATH_LEN, "%s", node_path_value);
    free(encoded_services_info_buffer);
    cJSON_Delete(root);
    free(services_info_buffer);
    INFO("register services successfully");
    return 0;
}


/**
 * [get_cur_node_path description]
 * @return [description]
 */
const char * get_cur_node_path() {
    return zk_h.node_value;
}
