/**
 * datagram packet protocol:
 * crc          : 32bit crc32 value of all fields excluding itself,
 *                including 'identifier', 'length', 'type', 'reversed', 'body'
 * identifier   : 32bit unsigned int,
 *                the request is same as the response
 * length       : 32bit unsigned int,
 *                just means the size of 'body' field
 * type         : 8bit char,
 *                request type is [0,128],  [0,0x0F] is for system need,
 *                response type is (128,256],
 *                [response type] = [response type] | (0x01 << 7)
 *                defined types as following:
 *                0x01  - heartbeat request
 *                0x10  - business request
 * reversed     : 24bit char[3], padding with '\0'
 * body         : bytes' count indicated by the 'length' field,
 *                for the better performance,
 *                if this is request, max value is 8192 (8k),
 *                if this is response, max value is 81920 (80k)
 */

#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <sys/types.h> //size_t
#include <sys/time.h>  //timeval
#include <stdint.h>  //uint8_t,uint16_t,uint32_t,uint64_t


#ifdef __cplusplus
extern "C" {
#endif

#define BIZ_REQ_MSG_TYPE  ((uint8_t)0x10)
#define BIZ_RES_MSG_TYPE  ( BIZ_REQ_MSG_TYPE | (((uint8_t)0x01) << 7 ) )

#define HB_REQ_MSG_TYPE  ((uint8_t)0x01)
#define HB_RES_MSG_TYPE  ( HB_REQ_MSG_TYPE | (((uint8_t)0x01) << 7 ) )

//message header
#pragma pack(1)
typedef struct _message_header
{
	uint32_t  crc;
	uint32_t  identifier;
	uint32_t  length;
	uint8_t   type;
	uint8_t   reversed[3];
} message_header;
#pragma pack()

/**
 * get resp type
 * @param  type [description]
 * @return      [description]
 */
uint8_t get_resp_type(uint8_t type);


/**
 * send request (for client)
 * @param  fd         [description]
 * @param  identifier [description]
 * @param  buffer     [description]
 * @param  buf_len    [description]
 * @return            1 means sucess, 0 means failed
 */
int send_message(int fd, uint32_t identifier, uint8_t type, const void * buffer, size_t buf_len);


/**
 * receive request (for server)
 * @param  fd          [description]
 * @param  identifier  [description]
 * @param  type        [description]
 * @param  out_buf     [description]
 * @param  out_buf_len [description]
 * @return             [description]
 */
int receive_message(int fd, uint32_t * identifier, uint8_t * type, void ** out_buf, size_t * out_buf_len);


#ifdef __cplusplus
}
#endif

#endif