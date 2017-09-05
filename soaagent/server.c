#include "server.h"
#include "log.h"
#include "conf.h"
#include "proxy_message.h"
#include "invoke_stub.h"
#include "util.h"


/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>
#include <arpa/inet.h>

#include <sys/time.h>  //gettimeofday
#include <sys/types.h>

#include <unistd.h>  //read,write
#include <string.h>   //snprintf,strerror
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>  //errno

#include <event2/event.h>
#include <event2/event_struct.h>  //event
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_struct.h>  //bufferevent
#include <event2/thread.h>  //evthread_use_pthreads



//插入libevent的日志反调函数
void my_libevent_event_log_cb(int severity, const char *msg)
{
	switch (severity) {
	case _EVENT_LOG_DEBUG:
		DEBUG ( "LIBEVENT:debug :: %s", msg );
		break;
	case _EVENT_LOG_MSG:
		INFO ( "LIBEVENT:info :: %s", msg );
		break;
	case _EVENT_LOG_WARN:
		WARN ( "LIBEVENT:warn :: %s", msg );
		break;
	case _EVENT_LOG_ERR:
		ERROR ( "LIBEVENT:error :: %s", msg );
		break;
	default:
		DEBUG ( "LIBEVENT:??? :: %s", msg );
		break;
	}
}


//工作线程数量
static int global_worker_num = 0;
//工作线程列表
static worker_info_ptr * global_worker_info_list = NULL;
//全局循环计数器
static int global_cycle_counter = 0;


/***
** 取得工作线程的索引(by round)
***/
static int get_next_worker_index(int thread_num)
{
	int index = global_cycle_counter % thread_num;
	global_cycle_counter++;
	if (global_cycle_counter > 0xFFFF)
	{
		global_cycle_counter = 0;
	}
	return index;
}


/**
** 处理缓冲中的报文
**/
static int handle_buffer (const char * worker_name, int client_fd, struct evbuffer *input, struct evbuffer *output, struct bufferevent *bev)
{
	//判断缓冲中是否包含完整报文
	int bCanRead = 0;
	size_t availableByteNum = evbuffer_get_length(input);
	proxy_msg_header header;
	//check header
	if ( availableByteNum >= sizeof(proxy_msg_header) )
	{
		memset (&header, 0, sizeof(header));
		if ( evbuffer_copyout(input, &header, sizeof(proxy_msg_header)) > 0 )
		{
			header.length = ntohl(header.length);

				printf("proxy_msg_header body  %d and availableByteNum %d and header.length %d\n", sizeof(proxy_msg_header), availableByteNum,header.length );
			//check body
			if (availableByteNum >= sizeof(proxy_msg_header) + header.length )
			{
				bCanRead = 1;
			}
			else
			{
				DEBUG ("%s handle client connection [fd=%d] : availableByteNum=%u,head need=%u,body need=%u", worker_name, client_fd, availableByteNum, sizeof(proxy_msg_header), header.length);
			}
		}
	}
	//取出报文并进行处理
	if ( bCanRead )
	{
		//从缓冲中先扔掉已经分析过的报文头
		if ( evbuffer_drain (input, sizeof(proxy_msg_header)) == 0 )
		{
			SoaProxyRequest * req_msg = NULL;
			struct timeval start_time, end_time;
			get_now_time(&start_time);
			/***********************报文处理开始*************************/
			//读报文体
			char * body = (char *) malloc(header.length);
			if ( evbuffer_remove (input, body, header.length) > 0 )
			{
				if (proxy_request_decode(body, header.length, &req_msg)) {
					ERROR("decode proxy_msg_body failed : %s", strerror(errno));
					free(body);
					return -1;
				}

				free(body);
				int status = 0;
				char * resp_buffer = NULL;
				size_t resp_buf_len = 0;
				int ret = invoke_service(req_msg->group_name, req_msg->service_name, req_msg->service_version, req_msg->request.data, req_msg->request.len, &resp_buffer, &resp_buf_len);
				if (!ret) {
					status = 0;
				} else {
					status = 1;
					resp_buffer = NULL;
					resp_buf_len = 0;
				}
				void * buffer = NULL;
				size_t len = 0;
				proxy_response_encode(status, resp_buffer, resp_buf_len, &buffer, &len);
				//construct message
				size_t message_len = sizeof(proxy_msg_header) + len;
				char * message_buf = (char *) malloc(message_len);
				memset(message_buf, 0, sizeof(proxy_msg_header));
				if (len > 0) {
					memcpy(message_buf + sizeof(proxy_msg_header), buffer, len);
				}
				proxy_msg_header * header_ptr = (proxy_msg_header *) message_buf;
				header_ptr->length = htonl(len);
				//send message
				if ( evbuffer_add (output, message_buf, message_len) == 0 )
				{
					DEBUG ("%s handle client connection [fd=%d] : ack successfully", worker_name, client_fd);
				}
				else
				{
					ERROR ("%s handle client connection [fd=%d] : ack failed", worker_name, client_fd);
				}
				//clean resource
				proxy_response_encode_clean(buffer);
				free(message_buf);
				if (status == 0) {
					free(resp_buffer);
				}
				proxy_request_decode_clean(req_msg);
			}
			/***********************报文处理结束*************************/
			get_now_time(&end_time);
			unsigned int cost_time = get_time_difference(&start_time, &end_time);
			DEBUG ("%s handle client connection [fd=%d] : cost time (ms) = %u", worker_name, client_fd, cost_time);
		}
		else
		{
			ERROR ("%s handle client connection [fd=%d] : drain header failed", worker_name, client_fd);
		}
	}
	return 0;
}



/**
** 处理读数据
**/
static void readcb(struct bufferevent *bev, void *ctx)
{
	worker_info * worker_info_item = (worker_info *)ctx;
	int client_fd = bev->ev_read.ev_fd;

	struct evbuffer *input, *output;
	input = bufferevent_get_input(bev);
	output = bufferevent_get_output(bev);

	if (handle_buffer(worker_info_item->m_thread_name, client_fd, input, output, bev)) {
		ERROR ("%s handle client connection : some error occurs, close it : fd=%d", worker_info_item->m_thread_name, client_fd);
		bufferevent_free(bev);
	}
}


/**
** 处理异常
**/
static void errorcb(struct bufferevent *bev, short error, void *ctx)
{
	worker_info * worker_info_item = (worker_info *)ctx;
	int client_fd = bev->ev_read.ev_fd;

	if (error & BEV_EVENT_EOF) {
		INFO ("%s handle client connection : client connection close , close it : fd=%d", worker_info_item->m_thread_name, client_fd);
		/* connection has been closed, do any clean up here */
		/* ... */
	} else if (error & BEV_EVENT_ERROR) {
		ERROR ("%s handle client connection : some error occurs : %s , close it : fd=%d", worker_info_item->m_thread_name, strerror(errno), client_fd);
		/* check errno to see what error occurred */
		/* ... */
	} else if (error & BEV_EVENT_TIMEOUT) {
		INFO ("%s handle client connection : client connection timeout , close it : fd=%d", worker_info_item->m_thread_name, client_fd);
		/* must be a timeout event handle, handle it */
		/* ... */
	}
	else
	{
		ERROR ("%s handle client connection : unknown error occurs : %d , close it : fd=%d", worker_info_item->m_thread_name, errno, client_fd);
	}
	bufferevent_free(bev);
}


/**
** 处理来自主线程的通知
**/
static void do_handle_notify(evutil_socket_t notify_fd, short event, void *arg)
{
	worker_info * worker_info_item = (worker_info *)arg;
	//得到客户端套接描述字
	int fd;
	int result = recv(notify_fd, &fd, sizeof(fd), 0);

	if (result <= 0)
	{
		ERROR ("%s handle notify failed : %s", worker_info_item->m_thread_name, strerror(errno) );
		return;
	}
	//注册客户端套接描述字的读事件到本工作线程的event base里
	INFO ("%s receive client connection : fd=%d", worker_info_item->m_thread_name, fd);
	struct bufferevent *bev;
	evutil_make_socket_nonblocking(fd);
	bev = bufferevent_socket_new(worker_info_item->m_base, fd, BEV_OPT_CLOSE_ON_FREE);
	bufferevent_setcb(bev, readcb, NULL, errorcb, arg);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}


/**
** 接受客户端连接
**/
static void do_accept(evutil_socket_t listener, short event, void *arg)
{
	worker_info_ptr * worker_info_list = (worker_info_ptr *)arg;
	struct sockaddr_in peer_addr;
	memset (&peer_addr, 0, sizeof(peer_addr));
	socklen_t slen = sizeof(peer_addr);
	int fd = accept(listener, (struct sockaddr*)&peer_addr, &slen);
	if (fd < 0) {
		ERROR ("accept failed : %s", strerror(errno) );
	} else if (fd > FD_SETSIZE) {
		ERROR ("fd %d > FD_SETSIZE : %s", fd, strerror(errno) );
		close(fd);
	} else {
		char client_info[50] = {0};
		snprintf (client_info, 50, "%s:%d", inet_ntoa(peer_addr.sin_addr), ntohs(peer_addr.sin_port));
		INFO ("accept client connection => %s , fd=%d", client_info, fd);
		//将套接描述字通知给工作线程
		int worker_index = get_next_worker_index(global_worker_num);
		write (worker_info_list[worker_index]->m_notify_send_fd, &fd, sizeof(fd));
	}
}


static void * worker_thread_func(void * context) {
	worker_info * w_info = (worker_info *)context;
	INFO ( "Worker [%s] begin to execute ...", w_info->m_thread_name );
	event_base_dispatch(w_info->m_base);
}

/**
** 启动服务器
**/
int tcp_server_startup_with_multibase(const char * listen_host, int listen_port, int thread_num)
{
	if (thread_num < 0) {
		ERROR ("thread_num config value is wrong!");
		return -1;
	}
	global_worker_num = thread_num;

	INFO ("begin to startup tcp server with multi base (host=%s, port=%d, threadnum=%d) ...", listen_host, listen_port, thread_num);

	/*********************初始化主event base**********************/
	event_set_log_callback ( my_libevent_event_log_cb );

	evutil_socket_t listener;
	struct sockaddr_in sin;
	struct event_base *base;
	struct event *listener_event;

	base = event_base_new();
	if (!base) {
		ERROR ("create event_base failed!");
		return -1;
	}

	bzero (&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(listen_host);
	sin.sin_port = htons(listen_port);

	listener = socket(AF_INET, SOCK_STREAM, 0);
	evutil_make_socket_nonblocking(listener);
	evutil_make_socket_closeonexec(listener);
	evutil_make_listen_socket_reuseable(listener);

	if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		ERROR ("bind failed : %s", strerror(errno) );
		return -1;
	}

	if (listen(listener, 16) < 0) {
		ERROR ("listen failed : %s", strerror(errno) );
		return -1;
	}

	/*********************初始化每个工作线程的event base**********************/
	global_worker_info_list = (worker_info_ptr)malloc(sizeof(worker_info_ptr) * thread_num);
	int i;
	for (i = 0; i < thread_num; ++i)
	{
		worker_info * w_info = (worker_info *)malloc(sizeof(worker_info));
		if (w_info == NULL) {
			ERROR ("worker_info malloc failed" );
			return -1;
		}
		*(global_worker_info_list + i) = w_info;
		snprintf (w_info->m_thread_name, 20, "worker-%02d", i);
		int sockfd[2];
		if ((socketpair(AF_LOCAL, SOCK_STREAM, 0, sockfd)) == -1)
		{
			ERROR ("socketpair failed : %s", strerror(errno) );
			return -1;
		}
		w_info->m_notify_receive_fd = sockfd[0];
		w_info->m_notify_send_fd    = sockfd[1];
		w_info->m_base = event_base_new();
		if (!w_info->m_base) {
			ERROR ("create event_base failed!");
			return -1;
		}
		//在工作线程的event base里注册通知描述字的读事件
		struct event *notify_event = event_new(w_info->m_base,
		                                       w_info->m_notify_receive_fd,
		                                       EV_READ | EV_PERSIST, do_handle_notify, (void*)w_info);
		event_add(notify_event, NULL);
	}

	/*********************启动每个工作线程**********************/
	for (i = 0; i < thread_num; ++i)
	{
		worker_info * w_info = *(global_worker_info_list + i);
		pthread_attr_t attr;
		pthread_attr_init (&attr);
		pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);  //avoid memory leak
		pthread_t tid;
		int ret = pthread_create(&tid, &attr, (void*)worker_thread_func, (void*)w_info);
		pthread_attr_destroy (&attr);
		if (ret) {
			ERROR("pthread_create error : %s", strerror(errno) );
			return -1;
		}
		w_info->m_thread_id = tid;
	}

	/*********************给主event base添加accept事件**********************/
	listener_event = event_new(base, listener, EV_READ | EV_PERSIST, do_accept, (void*)global_worker_info_list);
	event_add(listener_event, NULL);

	/*********************启动主event base(在进程的主线程中运行)**********************/
	event_base_dispatch(base);

	INFO ("end server (host=%s, port=%d, threadnum=%d) ", listen_host, listen_port, thread_num);
	return 0;
}


/**
 * [server_startup_multibase description]
 * @param  listen_addr [description]
 * @return             [description]
 */
int server_startup_multibase(const char * listen_addr) {
	char ip_buf[20];
	int port;
	if (parse_addr(listen_addr, ip_buf, 20, &port) == 0) {
		int thread_num = get_config_item_int("server:thread_num", 10);
		return tcp_server_startup_with_multibase(ip_buf, port, thread_num);
	} else {
		ERROR("Not support unix domain socket address");
		return -1;
	}
	return 0;
}
