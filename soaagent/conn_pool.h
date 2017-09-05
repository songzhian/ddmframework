/**
 * connection pool
 */
#ifndef __CONN_POOL_H__
#define __CONN_POOL_H__

#include <stdint.h>  //uint8_t,uint16_t,uint32_t,uint64_t
#include <stddef.h>
#include <sys/time.h>  //gettimeofday
#include <pthread.h>

#include "utlist.h"


#ifdef __cplusplus
extern "C" {
#endif

#define HOST_NAME_LEN  100

typedef struct _conn_obj;
typedef int test_conn_func_type(struct _conn_obj * conn);

/**
 * connection struct
 */
typedef struct _conn_obj {
    int sock_fd;
    char host[HOST_NAME_LEN];
    int port;
    int timeout_secs;    //timeout for read and write
    struct timeval last_used_time;   //last used timestamp
    struct timeval last_check_time;   //last check timestamp
    int connected;      // 1 means connected, 0 means not connected
    int error_occured;  // 0 means no error occur, 1 means some error occur
    struct _conn_obj *next, *prev;
} conn_obj;

/**
 * pool struct
 */
typedef struct _pool_obj {
    pthread_mutex_t lock;           //lock
    conn_obj * unused_conn_list;    //unused conn list
    conn_obj * used_conn_list;      //used conn list
    int inited;
    char host[HOST_NAME_LEN];
    int port;
    int timeout_secs;    //timeout for read and write
    int init_conn_num;  //init conn num
    int min_conn_num;   //min conn num
    int max_conn_num;   //max conn num
    int max_idle_secs;  //max idle seconds
} pool_obj;


/**
 * pool init
 * @param
 * @param
 * @param
 * @param
 * @return 0 means ok
 */
int pool_init(pool_obj * pool, const char * host, int port, int timeoutsecs,
              int init_conn_num, int min_conn_num, int max_conn_num, int max_idle_secs);

/**
 * lock pool
 * @param  pool [description]
 * @return      [description]
 */
int pool_lock(pool_obj * pool);

/**
 * unlock pool
 * @param  pool [description]
 * @return      [description]
 */
int pool_unlock(pool_obj * pool);

/**
 * pool destroy
 * @param  pool [description]
 * @return      [description]
 */
int pool_destroy(pool_obj * pool);

/**
 * require connection
 * @param  pool [description]
 * @return      [description]
 */
conn_obj * pool_require_connection(pool_obj * pool);

/**
 * release connection
 * @param pool              [description]
 * @param conn              [description]
 * @param release_as_normal [description]
 */
void pool_release_connection(pool_obj * pool, conn_obj * conn, int release_as_normal);

/**
 * get available conn num
 * @param  pool [description]
 * @return      [description]
 */
int pool_get_idle_conn_num(pool_obj * pool);

/**
 * get used conn num
 * @param  pool [description]
 * @return      [description]
 */
int pool_get_used_conn_num(pool_obj * pool);

/**
 * get total conn num
 * @param  pool [description]
 * @return      [description]
 */
int pool_get_total_conn_num(pool_obj * pool);

/**
 * check pool
 * @param  pool           [description]
 * @param  test_conn_func [description]
 * @return                [description]
 */
int pool_check(pool_obj * pool, test_conn_func_type test_conn_func);

/**
 * pool status
 * @param
 * @param
 * @param
 * @return
 */
int pool_status(pool_obj * pool, char * buffer, size_t buf_len);


#ifdef __cplusplus
}
#endif

#endif
