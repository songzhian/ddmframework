#include "util.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>


/**
 * [is_path_exist description]
 * @param  file_path [description]
 * @return           [description]
 */
int is_path_exist(const char * file_path) {
    if ( access(file_path, F_OK) == 0 ) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * [get_inode description]
 * @param  file_path [description]
 * @return           [description]
 */
long get_inode(const char * file_path) {
    struct stat buf;
    if (stat(file_path, &buf) == 0) {
        return buf.st_ino;
    } else {
        return -1;
    }
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
 * [write content to file]
 * @param  filename [file name including path]
 * @param  buffer   [buffer used to save content]
 * @param  len      [buffer's size]
 * @return          [return 0 means success, or failed]
 */
int write_content_to_file(const char * filename, const char * buffer, size_t len) {
    if (filename == NULL || buffer == NULL || len == 0) {
        return -1;
    }
    FILE * file = fopen(filename, "w");
    if (!file) {
        return -1;
    }
    size_t ret = fwrite(buffer, len, 1, file);
    if (ret != 1) {
        fclose(file);
        return -1;
    }
    fclose(file);
    return 0;
}

/**
** get now time
**/
void get_now_time(struct timeval * time) {
    memset(time, 0, sizeof(struct timeval));
    gettimeofday(time, NULL);
}

/**
** diff time(mili seconds),end_time - begin_time
**/
uint_t get_time_difference(const struct timeval * begin_time, const struct timeval * end_time) {
    uint_t passed_milis = (end_time->tv_sec * 1000 + end_time->tv_usec / 1000) -
                          (begin_time->tv_sec * 1000 + begin_time->tv_usec / 1000);
    return passed_milis;
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
            ERROR("timeout error : %s", strerror(errno));
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
