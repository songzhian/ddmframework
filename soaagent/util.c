#include "util.h"
#include "log.h"

#include <unistd.h>   //access,getpid,read,write
#include <syslog.h>
#include <fcntl.h>
#include <sys/resource.h>

#include <sys/types.h>
#include <sys/stat.h>  //umask,mkdir
#include <sys/types.h>

#include <signal.h>   //SIG_IGN
#include <stdlib.h>
#include <string.h>   //strlen,strerror
#include <errno.h>   //errno
#include <stdio.h>    //fopen
#include <limits.h>   //PATH_MAX

#include <netinet/in.h>  //sockaddr_in
#include <arpa/inet.h>  //inet_ntoa,htonl
#include <sys/socket.h> //socketpair,recv

#include <fcntl.h>   //fcntl
#include <sys/ioctl.h>  //ioctl

#include <errno.h>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>


/**
** 文件路径是否存在
**/
int is_path_exist(const char * file_path) {
	if ( access(file_path, F_OK) == 0 ) {
		return 1;
	} else {
		return 0;
	}
}

int parse_addr(const char * addr, char * ip_buf, size_t ip_buf_len, int * port) {
	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s", addr );
	char * str_ptr = path + 1;
	char * pos_ptr = strchr(str_ptr, ':');
	if (pos_ptr != NULL) {
		*pos_ptr = '\0';
		snprintf(ip_buf, ip_buf_len, "%s", path);
		*pos_ptr = ':';
		pos_ptr++;
		*port = atoi(pos_ptr);
		return 0;
	}
	return -1;
}

/**
** 守护进程
**/
void daemonize(const char * cwd) {
	int					i, fd0, fd1, fd2;
	pid_t				pid;
	struct rlimit		rl;
	struct sigaction	sa;

	/*
	 * Clear file creation mask.
	 */
	umask(0);

	/*
	 * Get maximum number of file descriptors.
	 */
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
		fprintf(stderr, "can't get file limit\n");
		exit(0);
	}

	/*
	 * Become a session leader to lose controlling TTY.
	 */
	if ((pid = fork()) < 0) {
		fprintf(stderr, "can't fork (first)\n");
		exit(0);
	} else if (pid != 0) {
		/* parent */
		exit(0);
	}

	/*
	* Set new session
	*/
	setsid();

	/*
	 * Ensure future opens won't allocate controlling TTYs.
	 */
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0) {
		fprintf(stderr, "can't ignore SIGHUP\n");
		exit(0);
	}

	if ((pid = fork()) < 0) {
		fprintf(stderr, "can't fork (second)\n");
		exit(0);
	} else if (pid != 0) {
		/* parent */
		exit(0);
	}

	/*
	 * Change the current working directory to the root so
	 * we won't prevent file systems from being unmounted.
	 */
	if (chdir(cwd) < 0) {
		fprintf(stderr, "can't change directory to %s\n", cwd);
		exit(0);
	}

	/*
	 * Close all open file descriptors.
	 */
	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for (i = 0; i < rl.rlim_max; i++)
		close(i);

	/*
	 * Attach file descriptors 0, 1, and 2 to /dev/null.
	 */
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);
}


/**
** 写进程ID入文件
**/
int write_pid(const char * pid_file_path) {
	char dir_path[PATH_MAX] = {0};
	get_dir_path(pid_file_path, dir_path, PATH_MAX);
	mkdirs(dir_path, 0775);
	//
	pid_t pid = getpid();
	char buffer [20] = {0};
	snprintf (buffer, 20, "%d", pid);
	FILE * file = fopen( pid_file_path, "w" );
	if ( !file )
	{
		ERROR ("fopen %s failed : %s", pid_file_path, strerror(errno));
		return -1;
	}
	fwrite ( buffer, 1, strlen(buffer), file );
	fclose ( file );
	return 0;
}

/**
** 取文件的所在路径
**/
int get_dir_path (const char * file_path, char * buffer, size_t len) {
	char * ptr = (char *)file_path;
	int end_pos = strlen(file_path);
	int found_pos = -1;
	while (end_pos-- > 0)
	{
		if (ptr[end_pos] == '/')
		{
			found_pos = end_pos;
			break;
		}
	}
	if (found_pos == -1) return -1;
	*(ptr + found_pos) = '\0';
	snprintf(buffer, len, "%s", ptr);
	*(ptr + found_pos) = '/';
	return 0;
}

/**
** 取文件名
**/
int get_file_name (const char * file_path, char * buffer, size_t len) {
	char * ptr = (char *)file_path;
	int end_pos = strlen(file_path);
	int found_pos = -1;
	while (end_pos-- > 0)
	{
		if (ptr[end_pos] == '/')
		{
			found_pos = end_pos;
			break;
		}
	}
	if (found_pos == -1) return -1;
	ptr += found_pos + 1;
	snprintf(buffer, len, "%s", ptr);
	return 0;
}

/**
 * [get_file_last_modified description]
 * @param  file_path [description]
 * @return           [description]
 */
time_t get_file_last_modified(const char * file_path) {
	struct stat buf;
	int fd = open(file_path, O_RDONLY);
	if (fd < 0) {
		ERROR("open file failed : %s", file_path);
		return 0;
	}
	fstat(fd, &buf);
	time_t last_modified = buf.st_mtime;
	close(fd);
	return last_modified;
}

/**
 * [make_sure_dir description]
 * @param  file_path [description]
 * @return           [description]
 */
int make_sure_dir(const char * file_path, mode_t mode) {
	if (file_path == NULL) {
		return -1;
	}
	if (strlen(file_path) <= 1) {
		return -1;
	}
	char path[PATH_MAX];
	snprintf(path, PATH_MAX, "%s", file_path );
	char * str_ptr = path + 1;
	char * pos_ptr = strchr(str_ptr, '/');
	while (pos_ptr != NULL && pos_ptr != str_ptr) {
		*pos_ptr = '\0';
		if (access(path, F_OK)) {
			if (mkdir(path, mode)) {
				*pos_ptr = '/';
				return -1;
			}
		}
		*pos_ptr = '/';
		str_ptr = pos_ptr + 1;
		pos_ptr = strchr(str_ptr, '/');
	}
	return 0;
}

/**
** 创建多级目录
**/
int mkdirs (const char * dir_path, mode_t mode) {
	char * ptr = (char *) dir_path;
	int len = strlen(dir_path);
	int index = 0;
	int sepPos = -1;
	while ( index < len ) {
		if ( ptr[index] == '/' && index != 0 ) {
			sepPos = index;
			*(ptr + index) = '\0';
			if (!is_path_exist(ptr)) {
				if (mkdir(ptr, mode) == 0) {
				} else {
					ERROR ("mkdirs %s failed : %s", ptr, strerror(errno));
					*(ptr + index) = '/';
					return -1;
				}
			}
			*(ptr + index) = '/';
		}
		index++;
	}
	if ( sepPos != index - 1 ) {
		//表明路径不是以'/'结束的
		if (!is_path_exist(dir_path)) {
			if (mkdir(dir_path, mode) == 0) {
			} else {
				ERROR ("mkdirs %s failed : %s", dir_path, strerror(errno));
				return -1;
			}
		}
	}
	return 0;
}

/**
 * [read content from file]
 * @param  filename [file name including path]
 * @param  buffer   [buffer used to save content]
 * @param  len      [buffer's size]
 * @return          [return 0 means success, or failed]
 */
int read_content_from_file(const char * filename, char ** buffer, size_t * len) {
	if (filename == NULL) {
		return -1;
	}
	FILE * file = fopen(filename, "r");
	if (!file) {
		return -1;
	}
	fseek(file, 0, SEEK_END);
	size_t size = ftell(file);
	if (len) {
		*len = size;
	}
	*buffer = malloc( size + 1 );
	fseek(file, 0, SEEK_SET);
	size_t ret = fread(*buffer, size, 1, file);
	if (ret != 1) {
		fclose(file);
		free(*buffer);
		return -1;
	}
	*(*buffer + size) = '\0';
	fclose(file);
	return 0;
}

/**
** 写文件
**/
int write_content_to_file (const char * file_path, const void * buffer, size_t len) {
	FILE * file = fopen( file_path, "wb" );
	if ( !file )
	{
		ERROR ("fopen %s failed : %s", file_path, strerror(errno));
		return -1;
	}
	fwrite ( buffer, 1, len, file );
	fclose ( file );
	return 0;
}

/**
** 获取当前时间
**/
void get_now_time(struct timeval * time) {
	memset(time, 0, sizeof(struct timeval));
	gettimeofday(time, NULL);
}


/**
** 获取时间差(单位:毫秒),end_time - begin_time
**/
unsigned int get_time_difference(const struct timeval * begin_time, const struct timeval * end_time) {
	unsigned int passed_milis =  (end_time->tv_sec * 1000 + end_time->tv_usec / 1000) -
	                             (begin_time->tv_sec * 1000 + begin_time->tv_usec / 1000);
	return passed_milis;
}


/**
 * [socket_send description]
 * @param  sockfd  [description]
 * @param  buffer  [description]
 * @param  buf_len [description]
 * @return         [description]
 */
int socket_send(int sockfd, const void * buffer, size_t buf_len) {
	ssize_t tmp;
	size_t need_send_num = buf_len;
	const char *p = buffer;
	while (1) {
		tmp = send(sockfd, p, need_send_num, 0);
		if (tmp < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				usleep(200);
				continue;
			}
			return -1;
		}
		if ((size_t)tmp == need_send_num)
		{
			return 0;
		}
		need_send_num -= tmp;
		p += tmp;
	}
	return -1;
}


/**
 * [socket_receive description]
 * @param  sockfd  [description]
 * @param  out_buf [description]
 * @param  buf_len [description]
 * @return         [description]
 */
int socket_receive(int sockfd, void * out_buf, size_t buf_len, int timeout_secs) {
	int ret = 0;
	if (timeout_secs > 0) {
		fd_set read_fdset;
		struct  timeval timeout;
		FD_ZERO(&read_fdset);
		FD_SET(sockfd, &read_fdset);
		timeout.tv_sec = timeout_secs;
		timeout.tv_usec = 0;
		do {
			ret = select(sockfd + 1, &read_fdset, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);
		if (ret == 0) {
			errno = ETIMEDOUT;
			return -1;
		} else if (ret < 0) {
			return -1;
		}
	}
	ssize_t tmp;
	size_t need_receive_num = buf_len;
	char *p = out_buf;
	while (1) {
		tmp = recv(sockfd, p, need_receive_num, 0);
		if (tmp == 0) {
			//the peer have shutdown
			ERROR("peer have shutdown");
			return -2;
		}
		if (tmp < 0)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				usleep(200);
				continue;
			}
			return -1;
		}
		if ((size_t)tmp == need_receive_num)
		{
			return 0;
		}
		need_receive_num -= tmp;
		p += tmp;
	}
	return -1;
}


/**
 * next identifier
 * @return [description]
 */
uint32_t next_identifier() {
	//temporary solution
	static volatile uint32_t counter = 0;
	return ++counter;
}


/**
 * [substitute description]
 * @param pInput     [description]
 * @param pOutput    [description]
 * @param output_len [description]
 * @param pSrc       [description]
 * @param pDst       [description]
 */
void substitute(const char *pInput, char * pOutput, size_t output_len, char *pSrc, char *pDst) {
	char    *pi, *po, *p;
	int     nSrcLen, nDstLen, nLen, nLeft, nCopy;

	// 指向输入字符串的游动指针.
	pi = pInput;
	// 指向输出字符串的游动指针.
	po = pOutput;
	// 计算被替换串和替换串的长度.
	nSrcLen = strlen(pSrc);
	nDstLen = strlen(pDst);
	nLeft = output_len;

	// 查找pi指向字符串中第一次出现替换串的位置,并返回指针(找不到则返回null).
	p = strstr(pi, pSrc);
	if (p) {
		// 找到.
		while (p) {
			// 计算被替换串前边字符串的长度.
			nLen = (int)(p - pi);
			if (nLeft - nLen < nDstLen) {
				break;
			}
			// 复制到输出字符串.
			memcpy(po, pi, nLen);
			memcpy(po + nLen, pDst, nDstLen);
			nLeft = nLeft - nLen - nDstLen;
			// 跳过被替换串.
			pi = p + nSrcLen;
			// 调整指向输出串的指针位置.
			po = po + nLen + nDstLen;
			// 继续查找.
			p = strstr(pi, pSrc);
		}
		// 复制剩余字符串.
		if (nLeft > strlen(pi) ) {
			nCopy = strlen(pi);
		} else {
			nCopy = nLeft - 1;
		}
		memcpy(po, pi, nCopy);
		*(po + nCopy) = '\0';
	} else {
		// 没有找到则原样复制.
		if (nLeft > strlen(pi) ) {
			nCopy = strlen(pi);
		} else {
			nCopy = nLeft - 1;
		}
		memcpy(po, pi, nCopy);
		*(po + nCopy) = '\0';
	}
}
