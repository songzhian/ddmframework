#ifndef __PROXY_MESSAGE_H__
#define __PROXY_MESSAGE_H__

#include <sys/types.h>
#include <stdint.h>  //uint8_t,uint16_t,uint32_t,uint64_t

#include "soa_proxy_msg.pb-c.h"


#ifdef __cplusplus
extern "C" {
#endif


#pragma pack(1)
typedef struct tag_proxy_msg_header
{
	uint32_t  length;   //message body length
} proxy_msg_header;
#pragma pack()

/**
 * [proxy_request_send description]
 * @param  fd              [description]
 * @param  service_name    [description]
 * @param  service_version [description]
 * @param  request_body    [description]
 * @param  request_len     [description]
 * @return                 [description]
 */
int proxy_request_send(int fd, const char * group, const char * service_name, const char * service_version, const void * request_body, size_t request_len);

/**
 * [proxy_request_receive description]
 * @param  fd              [description]
 * @param  request_message [description]
 * @return                 [description]
 */
int proxy_request_receive(int fd, SoaProxyRequest ** request_message);

/**
 * [proxy_response_send description]
 * @param  fd            [description]
 * @param  response_body [description]
 * @param  response_len  [description]
 * @return               [description]
 */
int proxy_response_send(int fd, int status, const void * response_body, size_t response_len);

/**
 * [proxy_response_receive description]
 * @param  fd               [description]
 * @param  response_message [description]
 * @return                  [description]
 */
int proxy_response_receive(int fd, SoaProxyResponse ** response_message);

/**
 * [proxy_encode_request description]
 * @param  service_name    [description]
 * @param  service_version [description]
 * @param  request_body    [description]
 * @param  request_len     [description]
 * @param  buffer          [need to proxy_requset_encode_clean() ]
 * @param  len             [description]
 * @return                 [0 means successfully, -1 means failed]
 */
int proxy_request_encode(const char * group, const char * service_name, const char * service_version, const void * request_body, size_t request_len,
                         void ** buffer, size_t * len);

int proxy_request_encode_clean(void * buffer);

/**
 * [proxy_decode_request description]
 * @param  buffer  [description]
 * @param  len     [description]
 * @param  request_message [need to proxy_request_decode_clean() ]
 * @return         [0 means successfully, -1 means failed]
 */
int proxy_request_decode(const void * buffer, size_t len, SoaProxyRequest ** request_message);

int proxy_request_decode_clean(SoaProxyRequest * request_message);


/**
 * [proxy_encode_response description]
 * @param  response_body [description]
 * @param  response_len  [description]
 * @param  buffer        [need to proxy_encode_response_clean()]
 * @param  len           [description]
 * @return               [description]
 */
int proxy_response_encode(int status, const void * response_body, size_t response_len, void ** buffer, size_t * len);

int proxy_response_encoded_clean(void * buffer);

/**
 * [proxy_decode_response description]
 * @param  buffer           [description]
 * @param  len              [description]
 * @param  response_message [need to proxy_decode_response_clean()]
 * @return                  [description]
 */
int proxy_response_decode(const void * buffer, size_t len, SoaProxyResponse ** response_message);

int proxy_response_decode_clean(SoaProxyResponse * response_message);


#ifdef __cplusplus
}
#endif

#endif
