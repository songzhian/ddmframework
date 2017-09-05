#ifndef __SERVER_H__
#define __SERVER_H__

#include <pthread.h>
#include <stdint.h>  //uint8_t,uint16_t,uint32_t,uint64_t

#include <event2/buffer.h>

#include "uthash.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tag_thread_context {
	int  conn_fd;         /* use client fd */
	pthread_t  thread_id;
	UT_hash_handle hh;         /* makes this structure hashable */
} thread_context;

/**
 * [server_startup description]
 * @param  unix_sock_path [description]
 * @return                [description]
 */
int server_startup(const char * unix_sock_path);


typedef struct
{
	char                m_thread_name[20];
	pthread_t           m_thread_id;
	struct event_base * m_base;
	int                 m_notify_receive_fd;
	int                 m_notify_send_fd;
} worker_info, *worker_info_ptr;

/**
 * [server_startup_multibase description]
 * @param  listen_addr [description]
 * @return             [description]
 */
int server_startup_multibase(const char * listen_addr);

#ifdef __cplusplus
}
#endif

#endif
