#ifndef __UTIL_H__
#define __UTIL_H__

#include <sys/stat.h>  //mode_t
#include <sys/time.h>  //timeval
#include <sys/types.h> //size_t
#include <stdint.h>  //uint8_t,uint16_t,uint32_t,uint64_t


#ifdef __cplusplus
extern "C" {
#endif


#define int_min(a,b) (((a)>(b))?(b):(a))

/**
** 文件路径是否存在
**/
int is_path_exist(const char * file_path);


int parse_addr(const char * addr, char * ip_buf, size_t ip_buf_len, int * port);

/**
** 守护进程
**/
void daemonize(const char * cwd);


/**
** 写进程ID入文件
**/
int write_pid(const char * pid_file_path);


/**
** 取文件的所在路径
**/
int get_dir_path (const char * file_path, char * buffer, size_t len);

/**
** 取文件名
**/
int get_file_name (const char * file_path, char * buffer, size_t len);

/**
 * [get_file_last_modified description]
 * @param  file_path [description]
 * @return           [description]
 */
time_t get_file_last_modified(const char * file_path);

/**
 * [create_directory description]
 * @param  file_path [description]
 * @return           [description]
 */
int make_sure_dir (const char * file_path, mode_t mode);

/**
** 创建多级目录
**/
int mkdirs (const char * dir_path, mode_t mode);

/**
 * [read content from file]
 * @param  filename [file name including path]
 * @param  buffer   [buffer used to save content]
 * @param  len      [buffer's size]
 * @return          [return 0 means success, or failed]
 */
int read_content_from_file(const char * filename, char ** buffer, size_t * len);

/**
** 写文件
**/
int write_content_to_file (const char * file_path, const void * buffer, size_t len);

/**
** 获取当前时间
**/
void get_now_time(struct timeval * time);


/**
** 获取时间差(单位:毫秒),end_time - begin_time
**/
unsigned int get_time_difference(const struct timeval * begin_time, const struct timeval * end_time);


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


/**
 * next identifier
 * @return [description]
 */
uint32_t next_identifier();

/**
 * [substitute description]
 * @param pInput     [description]
 * @param pOutput    [description]
 * @param output_len [description]
 * @param pSrc       [description]
 * @param pDst       [description]
 */
void substitute(const char *pInput, char * pOutput, size_t output_len, char *pSrc, char *pDst);


#ifdef __cplusplus
}
#endif

#endif
