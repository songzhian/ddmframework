#ifndef _UTIL_H_
#define _UTIL_H_

#include <sys/types.h> //size_t
#include <stdint.h>  //uint32_t
#include <sys/time.h>  //timeval

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int uint_t;
typedef unsigned char uchar_t;

/**
 * [is_path_exist description]
 * @param  file_path [description]
 * @return           [description]
 */
int is_path_exist(const char * file_path);

/**
 * [get_inode description]
 * @param  file_path [description]
 * @return           [description]
 */
long get_inode(const char * file_path);

/**
 * [create_directory description]
 * @param  file_path [description]
 * @return           [description]
 */
int make_sure_dir(const char * file_path, mode_t mode);

/**
 * [read content from file]
 * @param  filename [file name including path]
 * @param  buffer   [buffer used to save content]
 * @param  len      [buffer's size]
 * @return          [return 0 means success, or failed]
 */
int read_content_from_file(const char * filename, char ** buffer, size_t * len);

/**
 * [write content to file]
 * @param  filename [file name including path]
 * @param  buffer   [buffer used to save content]
 * @param  len      [buffer's size]
 * @return          [return 0 means success, or failed]
 */
int write_content_to_file(const char * filename, const char * buffer, size_t len);


/**
** get now time
**/
void get_now_time(struct timeval * time);

/**
** diff time(mili seconds),end_time - begin_time
**/
uint_t get_time_difference(const struct timeval * begin_time, const struct timeval * end_time);

/**
 * next identifier
 * @return [description]
 */
uint32_t next_identifier();


/**
 * [socket_send description]
 * @param  sockfd  [description]
 * @param  buffer  [description]
 * @param  buf_len [description]
 * @return         [description]
 */
int socket_send(int sockfd, const void * buffer, size_t buf_len);


/**
 * [socket_receive description]
 * @param  sockfd  [description]
 * @param  out_buf [description]
 * @param  buf_len [description]
 * @return         [description]
 */
int socket_receive(int sockfd, void * out_buf, size_t buf_len, int timeout_secs);

#ifdef __cplusplus
}
#endif

#endif