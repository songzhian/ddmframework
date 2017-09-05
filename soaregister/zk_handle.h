#ifndef __ZK_HANDLE_H__
#define __ZK_HANDLE_H__

#include <zookeeper/zookeeper.h>
#include <zookeeper/zookeeper.jute.h>
#include <limits.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ZK_ADDR_LEN  4096
#define MAX_SERVICES_DEFINE_CONTENT_SIZE  1024*10
#define MAX_GROUP_VALUE_LEN    512
#define MAX_NODE_PATH_LEN   (MAX_SERVICES_DEFINE_CONTENT_SIZE+MAX_GROUP_VALUE_LEN+100)


typedef struct _zk_handle_
{
    char zk_addrs[MAX_ZK_ADDR_LEN];
    char config_file_path[PATH_MAX];
    zhandle_t * zk;
    watcher_fn callback;
    void * context;
    long receive_timeout;
    char node_value[MAX_NODE_PATH_LEN];  //save node name (full path)
    FILE * log_file;
} zk_handle;

typedef enum _node_type {
    NORMAL_NODE    = 1,   // normal
    EPHEMERAL_NODE = 2,   // ephemeral
} node_type;

typedef enum _execute_type {
    EXE_FULL       = 1,
    EXE_INCREMENT  = 2,
} execute_type;

/**
** children node notify function
**/
typedef void children_change_func_type(execute_type exe_type, const char * node_group, char ** node_names, int node_count);

typedef struct _change_context
{
    children_change_func_type * children_change_func;
    execute_type exe_type;
    char node_group_value[MAX_GROUP_VALUE_LEN];
} change_context;

/**
** establish connection to zk
**/
int init_zk_handle(zk_handle * zh);

/**
** close connection to zk
**/
int fini_zk_handle(zk_handle * zh);

/**
** judge node of zk
**/
int exist_node(zk_handle * zh, const char * path);

/**
** create node of zk
**/
int create_node(zk_handle * zh, const char * path, const char * value, node_type type);

/**
** create node of zk
**/
int delete_node(zk_handle * zh, const char * path);

/**
** make sure the path of node
**/
int make_sure_node_path(zk_handle * zh, char * path);

/**
** make sure the ephemeral node
**/
int make_sure_enode(zk_handle * zh, const char * path, const char * value);

/**
** get node value
**/
int get_node(zk_handle * zh, const char * path, watcher_fn watcher, void* watcher_ctx, char * buf, int* buf_len);

/**
** get children node
**/
int get_children(zk_handle * zh, watcher_fn watcher, change_context * change_cxt);


#ifdef __cplusplus
}
#endif

#endif