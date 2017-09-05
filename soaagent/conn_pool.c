#include "conn_pool.h"
#include "util.h"
#include "log.h"
#include "conf.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

/**
 * conn init
 * @param
 * @param
 * @param
 * @param
 * @return
 */
int conn_init(conn_obj * conn, const char * host, int port, int timeoutsecs) {
    snprintf(conn->host, HOST_NAME_LEN, "%s", host);
    conn->port = port;
    conn->timeout_secs = timeoutsecs;
    conn->connected = 0;
    conn->error_occured = 0;
    return 0;
}

/**
 * connect
 * @param
 * @return 0 means ok
 */
int conn_connect(conn_obj * conn) {
    int flag = 1;
    int recv_buf = get_config_item_int("pool:recv_buf", 1024 * 1024);
    int send_buf = get_config_item_int("pool:send_buf", 1024 * 1024);
    struct sockaddr_in pin;
    bzero(&pin, sizeof(pin));
    pin.sin_family = AF_INET;
    inet_pton(AF_INET, conn->host, &pin.sin_addr);
    pin.sin_port = htons(conn->port);
    conn->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval timeout = {conn->timeout_secs, 0};
    setsockopt(conn->sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(conn->sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(conn->sock_fd, SOL_SOCKET, SO_RCVBUF, (const char*)&recv_buf, sizeof(recv_buf));
    setsockopt(conn->sock_fd, SOL_SOCKET, SO_SNDBUF, (const char*)&send_buf, sizeof(send_buf));
    setsockopt(conn->sock_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&flag, sizeof(flag));  //close Nagle
    int ret = connect(conn->sock_fd, (void *)&pin, sizeof(pin));
    if (ret) {
        ERROR("conn_connect error : %s", strerror(errno));
        return -1;
    }
    recv_buf = 0, send_buf = 0;
    socklen_t len = sizeof(recv_buf);
    getsockopt(conn->sock_fd, SOL_SOCKET, SO_RCVBUF, &recv_buf, &len);
    len = sizeof(send_buf);
    getsockopt(conn->sock_fd, SOL_SOCKET, SO_SNDBUF, &send_buf, &len);
    conn->connected = 1;
    conn->error_occured = 0;
    get_now_time(&conn->last_used_time);
    get_now_time(&conn->last_check_time);
    INFO("connect successfully %s:%d : send_buf = %d, recv_buf = %d", conn->host, conn->port, send_buf, recv_buf);
    return 0;
}

/**
 * disconnect
 * @param
 * @return 0 means ok
 */
int conn_disconnect(conn_obj * conn) {
    conn->connected = 0;
    conn->error_occured = 0;
    int ret = close(conn->sock_fd);
    if (ret) {
        ERROR("conn_disconnect error : %s", strerror(errno));
        return -1;
    }
    INFO("disconnect successfully %s:%d ", conn->host, conn->port);
    return 0;
}

/**
 * reconnect
 * @param
 * @return
 */
int conn_reconnect(conn_obj * conn) {
    conn_disconnect(conn);
    return conn_connect(conn);
}

/**
 * conn is idle timeout?
 * @param
 * @return 1 means yes, 0 means no
 */
int conn_is_idle_timeout(conn_obj * conn, int max_idle_seconds) {
    struct timeval cur;
    get_now_time(&cur);
    unsigned int passed_milis = get_time_difference ( &conn->last_used_time, &cur );
    if (passed_milis > max_idle_seconds * 1000) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * conn is just checked?
 * @param
 * @return 1 means yes, 0 means no
 */
int conn_is_checked_just_now(conn_obj * conn, int max_idle_seconds) {
    struct timeval cur;
    get_now_time(&cur);
    unsigned int passed_milis = get_time_difference ( &conn->last_check_time, &cur );
    if (passed_milis < max_idle_seconds * 1000) {
        return 1;
    } else {
        return 0;
    }
}

/**
 *  test its validation
 * @param  conn           [description]
 * @param  test_conn_func [description]
 * @return                [description]
 */
int conn_test_its_validation(conn_obj * conn, test_conn_func_type test_conn_func) {
    if (test_conn_func(conn)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * pool init
 * @param
 * @param
 * @param
 * @param
 * @return 0 means ok
 */
int pool_init(pool_obj * pool, const char * host, int port, int timeoutsecs,
              int init_conn_num, int min_conn_num, int max_conn_num, int max_idle_secs) {
    pthread_mutex_init(&pool->lock, NULL);
    pool_lock(pool);
    pool->unused_conn_list = NULL;
    pool->used_conn_list = NULL;
    snprintf(pool->host, HOST_NAME_LEN, "%s", host);
    pool->port = port;
    pool->timeout_secs = timeoutsecs;
    pool->init_conn_num = init_conn_num;
    pool->min_conn_num = min_conn_num;
    pool->max_conn_num = max_conn_num;
    pool->max_idle_secs = max_idle_secs;
    int n;
    for (n = 0; n < init_conn_num; ++n) {
        conn_obj * conn = (conn_obj *) malloc(sizeof(conn_obj));
        conn_init(conn, pool->host, pool->port, pool->timeout_secs);
        if (!conn_connect(conn)) {
            LL_APPEND(pool->unused_conn_list, conn);
        } else {
            free(conn);
        }
    }
    pool->inited = 1;
    pool_unlock(pool);
    INFO("pool %s:%d init successfully", host, port);
    return 0;
}

/**
 * lock pool
 * @param  pool [description]
 * @return      [description]
 */
int pool_lock(pool_obj * pool) {
    return (pthread_mutex_lock(&pool->lock) == 0) ? 1 : 0;
}

/**
 * unlock pool
 * @param  pool [description]
 * @return      [description]
 */
int pool_unlock(pool_obj * pool) {
    return (pthread_mutex_unlock(&pool->lock) == 0) ? 1 : 0;
}

/**
 * pool destroy
 * @param  pool [description]
 * @return      [description]
 */
int pool_destroy(pool_obj * pool) {
    conn_obj * tmp, * elt;
    pool_lock(pool);
    int count = pool_get_used_conn_num(pool);
    if (count > 0) {
        pool_unlock(pool);
        ERROR("pool_destroy failed : %s:%d, used connection num > 0", pool->host, pool->port);
        return -1;
    }
    LL_FOREACH_SAFE(pool->unused_conn_list, elt, tmp) {
        LL_DELETE(pool->unused_conn_list, elt);
        conn_disconnect(elt);
        free(elt);
    }
    pool_unlock(pool);
    pthread_mutex_destroy(&pool->lock);
    return 0;
}

/**
 * require connection
 * NOTE: when you work, if the connection execute failed (timeout or crc error), reconnect it directly
 * @param  pool [description]
 * @return      [description]
 */
conn_obj * pool_require_connection(pool_obj * pool) {
    conn_obj * conn = NULL;
    conn_obj * tmp, * elt;
    pool_lock(pool);
    int count = pool_get_idle_conn_num(pool);
    if (count > 0) {
        // get the first
        conn = pool->unused_conn_list;
        LL_DELETE(pool->unused_conn_list, conn);
        LL_APPEND(pool->used_conn_list, conn);
    } else {
        int total_count = pool_get_used_conn_num(pool) + count;
        if (total_count < pool->max_conn_num ) {
            // new one
            conn = (conn_obj *) malloc(sizeof(conn_obj));
            conn_init(conn, pool->host, pool->port, pool->timeout_secs);
            if (!conn_connect(conn)) {
                LL_APPEND(pool->used_conn_list, conn);
            } else {
                free(conn);
                conn = NULL;
            }
        } else {
            //exceed max conn num
            ERROR("exceed max conn num %s:%d, total = %d, idle = %d, max = %d", pool->host, pool->port,
                  total_count, count, pool->max_conn_num);
        }
    }
    pool_unlock(pool);
    if (conn) {
        DEBUG("require connection of %s:%d successfully", pool->host, pool->port);
    } else {
        ERROR("require connection of %s:%d failed", pool->host, pool->port);
    }
    return conn;
}

/**
 * release connection
 * @param pool              [description]
 * @param conn              [description]
 * @param release_as_normal [description]
 */
void pool_release_connection(pool_obj * pool, conn_obj * conn, int release_as_normal) {
    if (!conn) {
        return;
    }
    pool_lock(pool);
    LL_DELETE(pool->used_conn_list, conn);
    LL_APPEND(pool->unused_conn_list, conn);
    if (release_as_normal) {
        get_now_time(&conn->last_used_time);
    } else {
        conn->error_occured = 1;
    }
    pool_unlock(pool);
    DEBUG("release connection of %s:%d successfully", pool->host, pool->port);
}

/**
 * get available conn num
 * @param  pool [description]
 * @return      [description]
 */
int pool_get_idle_conn_num(pool_obj * pool) {
    conn_obj * elt;
    int count;
    LL_COUNT(pool->unused_conn_list, elt, count);
    return count;
}

/**
 * get used conn num
 * @param  pool [description]
 * @return      [description]
 */
int pool_get_used_conn_num(pool_obj * pool) {
    conn_obj * elt;
    int count;
    LL_COUNT(pool->used_conn_list, elt, count);
    return count;
}

/**
 * get total conn num
 * @param  pool [description]
 * @return      [description]
 */
int pool_get_total_conn_num(pool_obj * pool) {
    return pool_get_idle_conn_num(pool) + pool_get_used_conn_num(pool);
}


/**
 * check pool
 * @param  pool           [description]
 * @param  test_conn_func [description]
 * @return                [description]
 */
int pool_check(pool_obj * pool, test_conn_func_type test_conn_func) {
    conn_obj * tmp, * elt;
    conn_obj * conn = NULL;
    int idle_conn_num, used_conn_num;
    //找出长期空闲的连接关闭掉,一次检查最多只关闭一个连接
    pool_lock(pool);
    used_conn_num = pool_get_used_conn_num(pool);
    //when the whole pool is dile, check is allowed
    if (used_conn_num == 0) {
        idle_conn_num = pool_get_idle_conn_num(pool);
        int can_close_num = idle_conn_num - pool->min_conn_num;
        if (can_close_num > 0) {
            LL_FOREACH_SAFE(pool->unused_conn_list, elt, tmp) {
                if (conn_is_idle_timeout(elt, pool->max_idle_secs * 2)) {
                    INFO("conn_check_thd_func : disconnect %s:%d,%d", elt->host, elt->port, elt->sock_fd);
                    LL_DELETE(pool->unused_conn_list, elt);
                    conn_disconnect(elt);
                    free(elt);
                    break;
                }
            }
        } else {
            INFO("conn_check_thd_func : not found conetions which need to close : %s:%d", pool->host, pool->port);
        }
    }
    pool_unlock(pool);
    return 0;
}

/**
 * pool status
 * @param
 * @param
 * @param
 * @return
 */
int pool_status(pool_obj * pool, char * buffer, size_t buf_len) {
    snprintf(buffer, buf_len, "host:%s, port:%d, idle:%d, used:%d, min:%d, max:%d",
             pool->host,
             pool->port,
             pool_get_idle_conn_num(pool),
             pool_get_used_conn_num(pool),
             pool->min_conn_num,
             pool->max_conn_num
            );
    return 0;
}
