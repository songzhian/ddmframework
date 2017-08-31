/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_soaprotobuf.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* If you declare any globals in php_soaprotobuf.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(soaprotobuf)
*/

/* True global resources - no need for thread safety here */
static int le_soaprotobuf;





/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/

/* {{{ proto string invoke_service(string group_name, string service_name, string version, string req_body)
    */
PHP_FUNCTION(get_buffer_request_body)
{
  char *group_name = NULL;
  char *service_name = NULL;
  char *version = NULL;
  char *req_body = NULL;
  int group_name_len;
  int service_name_len;
  int version_len;
  int req_body_len;


  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssss", &group_name, &group_name_len,&service_name, &service_name_len, &version, &version_len, &req_body, &req_body_len) == FAILURE)
    return;

  char * res_buffer;
  size_t res_buf_len;
  if (prequest_encode(group_name, service_name, version, req_body, req_body_len, &res_buffer, &res_buf_len)) {
    ZVAL_STRINGL(return_value, res_buffer, res_buf_len, 1);
    if (res_buffer != NULL) {
      efree(res_buffer);
    }
  } else {
    RETVAL_NULL();
  }

}



int prequest_encode(const char * group, const char * service_name, const char * service_version, const void * request_body, size_t request_len,
	char ** message_buf, size_t * message_len){


	void * buffer = NULL;
	size_t len = 0;
	proxy_request_encode(group, service_name, service_version, request_body, request_len, &buffer, &len);

	//construct message
	*message_len = sizeof(proxy_msg_header) + len;

	*message_buf = (char *) emalloc(*message_len);
	bzero(*message_buf, sizeof(proxy_msg_header));
	if (len > 0) {
		memcpy(*message_buf + sizeof(proxy_msg_header), buffer, len);
	}
	proxy_msg_header * header = (proxy_msg_header *) *message_buf;
	header->length = htonl(len);
	if(buffer != NULL){
		efree(buffer);
	}
	return 1;
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
		*buffer = emalloc(*len);
		soa_proxy_request__pack(&msg, *buffer);
	} else {
		*buffer = NULL;
		*len = 0;
	}
	return 0;
}




/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_soaprotobuf_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_soaprotobuf_init_globals(zend_soaprotobuf_globals *soaprotobuf_globals)
{
	soaprotobuf_globals->global_value = 0;
	soaprotobuf_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(soaprotobuf)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(soaprotobuf)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(soaprotobuf)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(soaprotobuf)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(soaprotobuf)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "soaprotobuf support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ soaprotobuf_functions[]
 *
 * Every user visible function must have an entry in soaprotobuf_functions[].
 */
const zend_function_entry soaprotobuf_functions[] = {
	PHP_FE(get_buffer_request_body,	NULL)		/* For testing, remove later. */
	PHP_FE_END	/* Must be the last line in soaprotobuf_functions[] */
};
/* }}} */

/* {{{ soaprotobuf_module_entry
 */
zend_module_entry soaprotobuf_module_entry = {
	STANDARD_MODULE_HEADER,
	"soaprotobuf",
	soaprotobuf_functions,
	PHP_MINIT(soaprotobuf),
	PHP_MSHUTDOWN(soaprotobuf),
	PHP_RINIT(soaprotobuf),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(soaprotobuf),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(soaprotobuf),
	PHP_SOAPROTOBUF_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SOAPROTOBUF
ZEND_GET_MODULE(soaprotobuf)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
