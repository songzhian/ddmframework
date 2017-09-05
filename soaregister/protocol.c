#include "protocol.h"
#include "util.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>  //htonl
#include <unistd.h>  //read,write
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <errno.h>

/**
 * get resp type
 * @param  type [description]
 * @return      [description]
 */
uint8_t get_resp_type(uint8_t type) {
    return (((uint8_t)0x01) << 7 ) | type;
}

/**
 * crc32 check
 * @param  header   [description]
 * @param  body     [description]
 * @param  body_len [description]
 * @return          1 means ok, 0 means failed
 */
static int crc32_check(message_header * header, const char * body, size_t body_len) {
    int ret = 0;
    char * buffer = (char *) malloc(body_len + sizeof(message_header));
    memcpy(buffer, header, sizeof(message_header));
    if (body_len > 0) {
        memcpy(buffer + sizeof(message_header), body, body_len);
    }
    uint32_t crc_value = crc32(buffer + 4, body_len + sizeof(message_header) - 4);
    if (ntohl(header->crc) == crc_value) {
        ret = 1;
    }
    free(buffer);
    return ret;
}

/**
 * send request (for client)
 * @param  fd         [description]
 * @param  identifier [description]
 * @param  buffer     [description]
 * @param  buf_len    [description]
 * @return            0 means sucess, -1 means failed
 */
int send_message(int fd, uint32_t identifier, uint8_t type, const char * buffer, size_t buf_len) {
    size_t message_len = sizeof(message_header) + buf_len;
    char * message_buf = (char *) malloc(message_len);
    memset(message_buf, 0, sizeof(message_header));
    if (buf_len > 0) {
        memcpy(message_buf + sizeof(message_header), buffer, buf_len);
    }
    message_header * header = (message_header *) message_buf;
    header->identifier = htonl(identifier);
    header->length = htonl(buf_len);
    header->type = type;
    header->crc = htonl(crc32(message_buf + 4, message_len - 4));
    int ret = socket_send(fd, message_buf, message_len);
    free(message_buf);
    return ret;
}


/**
 * receive reponse (for client)
 * @param  fd          [description]
 * @param  identifier  [description]
 * @param  out_buf     [*out_buf need to be free()]
 * @param  out_buf_len [description]
 * @return             0 means sucess, -1 means failed
 */
int receive_message(int fd, uint32_t * identifier, uint8_t * type, char ** out_buf, size_t * out_buf_len) {
    message_header header;
    if (socket_receive(fd, &header, sizeof(header), 3)) {
        ERROR("read header failed : %s", strerror(errno));
        return -1;
    }
    *type = header.type;
    *identifier = ntohl(header.identifier);
    *out_buf = NULL;
    *out_buf_len = 0;
    size_t body_len = ntohl(header.length);
    if (body_len == 0) {
        if (crc32_check(&header, NULL, 0)) {
            return 0;
        } else {
            ERROR("crc32 check failed");
            return -1;
        }
    }
    char * body_value = (char *) malloc(body_len + 1);
    if (socket_receive(fd, body_value, body_len, 3)) {
        ERROR("read body failed");
        free(body_value);
        return -1;
    }
    if (!crc32_check(&header, body_value, body_len)) {
        ERROR("crc32 check failed");
        free(body_value);
        return -1;
    }
    *(body_value + body_len) = '\0';
    *out_buf = body_value;
    *out_buf_len = body_len;
    return 0;
}
