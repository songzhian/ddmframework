#include "proxy_message.h"
#include "util.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/**
 * [proxy_request_send description]
 * @param  fd              [description]
 * @param  service_name    [description]
 * @param  service_version [description]
 * @param  request_body    [description]
 * @param  request_len     [description]
 * @return                 [description]
 */
int proxy_request_send(int fd, const char * group, const char * service_name, const char * service_version, const void * request_body, size_t request_len) {
	void * buffer = NULL;
	size_t len = 0;
	proxy_request_encode(group, service_name, service_version, request_body, request_len, &buffer, &len);
	//construct message
	size_t message_len = sizeof(proxy_msg_header) + len;
	char * message_buf = (char *) malloc(message_len);
	memset(message_buf, 0, sizeof(proxy_msg_header));
	if (len > 0) {
		memcpy(message_buf + sizeof(proxy_msg_header), buffer, len);
	}
	proxy_msg_header * header = (proxy_msg_header *) message_buf;
	header->length = htonl(len);
	//send message
	int ret = socket_send(fd, message_buf, message_len);
	//clean resource
	proxy_request_encode_clean(buffer);
	free(message_buf);
	if (ret) {
		//send failed
		return -1;
	}
	return 0;
}

/**
 * [proxy_request_receive description]
 * @param  fd              [description]
 * @param  request_message [description]
 * @return                 [description]
 */
int proxy_request_receive(int fd, SoaProxyRequest ** request_message) {
	proxy_msg_header header;
	if (socket_receive(fd, &header, sizeof(header), -1)) {
		ERROR("read proxy_msg_header failed : %s", strerror(errno));
		return -1;
	}
	if (header.length == 0) {
		*request_message = NULL;
		return 0;
	}
	header.length = ntohl(header.length);
	char * body_value = (char *) malloc(header.length);
	if (socket_receive(fd, body_value, header.length, 3)) {
		ERROR("read proxy_msg_body failed : %s", strerror(errno));
		free(body_value);
		return -1;
	}
	if (proxy_request_decode(body_value, header.length, request_message)) {
		ERROR("decode proxy_msg_body failed : %s", strerror(errno));
		free(body_value);
		return -1;
	}
	free(body_value);
	return 0;
}

/**
 * [proxy_response_send description]
 * @param  fd            [description]
 * @param  response_body [description]
 * @param  response_len  [description]
 * @return               [description]
 */
int proxy_response_send(int fd, int status, const void * response_body, size_t response_len) {
	void * buffer = NULL;
	size_t len = 0;
	proxy_response_encode(status, response_body, response_len, &buffer, &len);
	//construct message
	size_t message_len = sizeof(proxy_msg_header) + len;
	char * message_buf = (char *) malloc(message_len);
	memset(message_buf, 0, sizeof(proxy_msg_header));
	if (len > 0) {
		memcpy(message_buf + sizeof(proxy_msg_header), buffer, len);
	}
	proxy_msg_header * header = (proxy_msg_header *) message_buf;
	header->length = htonl(len);
	//send message
	int ret = socket_send(fd, message_buf, message_len);
	//clean resource
	proxy_response_encode_clean(buffer);
	free(message_buf);
	if (ret) {
		//send failed
		return -1;
	}
	// DEBUG("proxy_response_send send bytes %d", message_len);
	return 0;
}

/**
 * [proxy_response_receive description]
 * @param  fd               [description]
 * @param  response_message [description]
 * @return                  [description]
 */
int proxy_response_receive(int fd, SoaProxyResponse ** response_message) {
	proxy_msg_header header;
	if (socket_receive(fd, &header, sizeof(header), 3)) {
		ERROR("read proxy_msg_header failed : %s", strerror(errno));
		return -1;
	}
	if (header.length == 0) {
		*response_message = NULL;
		return 0;
	}
	header.length = ntohl(header.length);
	char * body_value = (char *) malloc(header.length);
	if (socket_receive(fd, body_value, header.length, 3)) {
		ERROR("read proxy_msg_body failed : %s", strerror(errno));
		free(body_value);
		return -1;
	}
	if (proxy_response_decode(body_value, header.length, response_message)) {
		ERROR("decode proxy_msg_body failed : %s", strerror(errno));
		free(body_value);
		return -1;
	}
	free(body_value);
	return 0;
}



/**
 * [proxy_encode_request description]
 * @param  service_name    [description]
 * @param  service_version [description]
 * @param  request_body    [description]
 * @param  request_len     [description]
 * @param  buffer          [need to proxy_encode_requset_clean() ]
 * @param  len             [description]
 * @return                 [0 means successfully, -1 means failed]
 */
int proxy_request_encode(const char * group, const char * service_name, const char * service_version, const void * request_body, size_t request_len,
                         void ** buffer, size_t * len) {
	SoaProxyRequest msg = SOA_PROXY_REQUEST__INIT;
	msg.group_name = (char *) group;
	msg.service_name = (char *) service_name;
	msg.service_version = (char *) service_version;
	if (request_body) {
		msg.request.data = (uint8_t	*) request_body;
	}
	if (request_len > 0) {
		msg.request.len = request_len;
	}
	*len = soa_proxy_request__get_packed_size(&msg);
	if (*len > 0 ) {
		*buffer = malloc(*len);
		soa_proxy_request__pack(&msg, *buffer);
	} else {
		*buffer = NULL;
		*len = 0;
	}
	return 0;
}

int proxy_request_encode_clean(void * buffer) {
	if (buffer) {
		free(buffer);
	}
	return 0;
}

/**
 * [proxy_decode_request description]
 * @param  buffer  [description]
 * @param  len     [description]
 * @param  request_message [need to proxy_decode_request_clean() ]
 * @return         [0 means successfully, -1 means failed]
 */
int proxy_request_decode(const void * buffer, size_t len, SoaProxyRequest ** request_message) {
	*request_message = soa_proxy_request__unpack(NULL, len, buffer);
	if (*request_message == NULL) {
		return -1;
	}
	return 0;
}

int proxy_request_decode_clean(SoaProxyRequest * request_message) {
	if (request_message) {
		soa_proxy_request__free_unpacked(request_message, NULL);
	}
	return 0;
}


/**
 * [proxy_encode_response description]
 * @param  response_body [description]
 * @param  response_len  [description]
 * @param  buffer        [need to proxy_encode_response_clean()]
 * @param  len           [description]
 * @return               [description]
 */
int proxy_response_encode(int status, const void * response_body, size_t response_len, void ** buffer, size_t * len) {
	SoaProxyResponse msg = SOA_PROXY_RESPONSE__INIT;
	msg.status = status;
	if (response_body) {
		msg.response.data = (uint8_t*) response_body;
	}
	if (response_len > 0) {
		msg.response.len = response_len;
	}
	*len = soa_proxy_response__get_packed_size(&msg);
	if (*len > 0 ) {
		*buffer = malloc(*len);
		soa_proxy_response__pack(&msg, *buffer);
	} else {
		*buffer = NULL;
		*len = 0;
	}
	return 0;
}

int proxy_response_encode_clean(void * buffer) {
	if (buffer) {
		free(buffer);
	}
	return 0;
}

/**
 * [proxy_decode_response description]
 * @param  buffer           [description]
 * @param  len              [description]
 * @param  response_message [need to proxy_decode_response_clean()]
 * @return                  [description]
 */
int proxy_response_decode(const void * buffer, size_t len, SoaProxyResponse ** response_message) {
	*response_message = soa_proxy_response__unpack(NULL, len, buffer);
	if (*response_message == NULL) {
		return -1;
	}
	return 0;
}

int proxy_response_decode_clean(SoaProxyResponse * response_message) {
	if (response_message) {
		soa_proxy_response__free_unpacked(response_message, NULL);
	}
	return 0;
}

