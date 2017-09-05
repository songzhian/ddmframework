#include "zk_handle.h"
#include "log.h"
#include "util.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>  //PATH_MAX


/**
** establish connection to zk
**/
int init_zk_handle(zk_handle * zh) {
    zoo_set_debug_level(ZOO_LOG_LEVEL_INFO);
    char log_file_name[PATH_MAX];
    snprintf(log_file_name, PATH_MAX, "/tmp/soa/zkc.%u.log", getpid() );
    if (make_sure_dir(log_file_name, 0755)) {
        INFO("make_sure_dir failed [%s]", log_file_name);
        return -1;
    }
    FILE * log_file = fopen(log_file_name, "a");
    if (log_file) {
        zh->log_file = log_file;
        INFO("=====zoo_set_log_stream set [%s]", log_file_name);
        zoo_set_log_stream(log_file);
    } else {
        INFO("=====zoo_set_log_stream failed [%s]: %d, %s", log_file_name, errno, strerror(errno));
    }
    zh->zk = zookeeper_init(zh->zk_addrs, zh->callback, zh->receive_timeout, 0, zh->context, 0);
    if (zh->zk == NULL) {
        ERROR("=====init_zk_handle failed : %d, %s", errno, strerror(errno));
        return -1;
    }
    int state = zoo_state(zh->zk);
    if (state == ZOO_CONNECTED_STATE) {
        INFO("=====current state is ZOO_CONNECTED_STATE");
    } else if (state == ZOO_CONNECTING_STATE) {
        INFO("=====current state is ZOO_CONNECTING_STATE");
    } else if (state == ZOO_AUTH_FAILED_STATE) {
        INFO("=====current state is ZOO_AUTH_FAILED_STATE");
    } else if (state == ZOO_ASSOCIATING_STATE) {
        INFO("=====current state is ZOO_ASSOCIATING_STATE");
    } else if (state == ZOO_EXPIRED_SESSION_STATE) {
        INFO("=====current state is ZOO_EXPIRED_SESSION_STATE");
    }
    if (!exist_node(zh, "/")) {
        ERROR("=====exist_node '/' invalid : %d, %s", errno, strerror(errno));
        return -1;
    }
    return 0;
}


/**
** close connection to zk
**/
int fini_zk_handle(zk_handle * zh) {
    if (!zh) {
        return 0;
    }
    int result_status = zookeeper_close(zh->zk);
    if (zh->log_file) {
        fclose(zh->log_file);
    }
    if (result_status == ZOK) {
        return 0;
    } else {
        ERROR("fini_zk_handle error : %s", zerror(result_status));
        return -1;
    }
}


/**
** judge node of zk
** return 1 means exist
** return 0 means not exist
**/
int exist_node(zk_handle * zh, const char * path) {
    struct Stat stat;
    int result_status = zoo_exists(zh->zk, path, 0, &stat);
    DEBUG("exist_node [%s] : %s", path, zerror(result_status));
    if (result_status == ZOK) {
        return 1;
    } else {
        return 0;
    }
}

/**
** create node of zk
**/
int create_node(zk_handle * zh, const char * path, const char * value, node_type type) {
    long flags = (type == EPHEMERAL_NODE) ? ZOO_EPHEMERAL : 0;
    struct ACL ENTIRE_ACL[] = {{ZOO_PERM_ALL, ZOO_ANYONE_ID_UNSAFE}};
    struct ACL_vector ENTIRE_ACL_VECTOR = {1, ENTIRE_ACL};
    int result_status = zoo_create(zh->zk, path, value, (value == NULL) ? -1 : strlen(value), &ENTIRE_ACL_VECTOR, flags,
                                   NULL, 0);
    if (result_status == ZOK) {
        return 0;
    } else {
        ERROR("create_node error : %s", zerror(result_status));
        return -1;
    }
}

/**
** create node of zk
**/
int delete_node(zk_handle * zh, const char * path) {
    int result_status = zoo_delete(zh->zk, path, -1);
    if (result_status == ZOK) {
        return 0;
    } else {
        ERROR("delete_node error : %s", zerror(result_status));
        return -1;
    }
}

/**
** make sure the path of node
** NOTE: path will be modified !!!
**/
int make_sure_node_path(zk_handle * zh, char * path) {
    if (path == NULL) {
        ERROR("invalid path : NULL");
        return -1;
    }
    if (strlen(path) <= 1) {
        ERROR("invalid path : [%s]", path);
        return -1;
    }
    char * str_ptr = path + 1;
    char * pos_ptr = strchr(str_ptr, '/');
    while (pos_ptr != NULL && pos_ptr != str_ptr) {
        *pos_ptr = '\0';
        if (!exist_node(zh, path)) {
            create_node(zh, path, NULL, NORMAL_NODE);
        }
        *pos_ptr = '/';
        str_ptr = pos_ptr + 1;
        pos_ptr = strchr(str_ptr, '/');
    }
    return 0;
}

/**
** make sure the ephemeral node
**/
int make_sure_enode(zk_handle * zh, const char * path, const char * value) {
    char * delim_p = strrchr(path, '/');
    if (delim_p == NULL) {
        ERROR("invalid path [%s] : not contains '/' ", path);
        return -1;
    }
    if (delim_p == path) {
        ERROR("invalid path [%s] : root", path);
        return -1;
    }
    char * parent_path_buf = strndup(path, delim_p - path + 1);
    DEBUG("parent_path_buf = [%s]", parent_path_buf);
    make_sure_node_path(zh, parent_path_buf);
    free(parent_path_buf);
    return create_node(zh, path, value, EPHEMERAL_NODE);
}

/**
** get node value
**/
int get_node(zk_handle * zh, const char * path, watcher_fn watcher, void* watcher_ctx, char * buf, int* buf_len) {
    int result_status = zoo_wget(zh->zk, path, watcher, watcher_ctx, buf, buf_len, NULL);
    if (result_status == ZOK) {
        return 0;
    } else {
        ERROR("get_node error : %s", zerror(result_status));
        return -1;
    }
}

/**
** get children node
**/
int get_children(zk_handle * zh, watcher_fn watcher, change_context * change_cxt) {
    struct String_vector str_vector;
    int ret = 0;
    struct Stat stat;
    int result_status = zoo_wget_children2(zh->zk, change_cxt->node_group_value, watcher, change_cxt, &str_vector, &stat);
    if (result_status == ZOK) {
        if (change_cxt && change_cxt->children_change_func) {
            (*change_cxt->children_change_func)(change_cxt->exe_type, change_cxt->node_group_value, str_vector.data, str_vector.count);
        }
    } else {
        ERROR("get_children error : %s", zerror(result_status));
        ret = -1;
    }
    deallocate_String_vector(&str_vector);
    return ret;
}
