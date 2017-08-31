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

#ifndef PHP_SOAPROTOBUF_H
#define PHP_SOAPROTOBUF_H

extern zend_module_entry soaprotobuf_module_entry;
#define phpext_soaprotobuf_ptr &soaprotobuf_module_entry

#define PHP_SOAPROTOBUF_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_SOAPROTOBUF_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_SOAPROTOBUF_API __attribute__ ((visibility("default")))
#else
#	define PHP_SOAPROTOBUF_API
#endif


#ifdef __cplusplus
extern "C" {
#endif

#include "soa_proxy_msg.pb-c.h"
#pragma pack(1)
typedef struct tag_proxy_msg_header
{
  uint32_t  length;   //message body length
} proxy_msg_header;
#pragma pack()

int prequest_encode(const char * group, const char * service_name, const char * service_version, const void * request_body, size_t request_len,
  char ** message_buf, size_t * message_len);
int proxy_request_encode(const char * group, const char * service_name, const char * service_version, const void * request_body, size_t request_len,
                         void ** buffer, size_t * len) ;
int proxy_response_decode_clean(SoaProxyResponse * response_message);

#ifdef __cplusplus
}
#endif


#ifdef ZTS
#include "TSRM.h"
#endif

/* 
  	Declare any global variables you may need between the BEGIN
	and END macros here:     

ZEND_BEGIN_MODULE_GLOBALS(soaprotobuf)
	long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(soaprotobuf)
*/

/* In every utility function you add that needs to use variables 
   in php_soaprotobuf_globals, call TSRMLS_FETCH(); after declaring other 
   variables used by that function, or better yet, pass in TSRMLS_CC
   after the last function argument and declare your utility function
   with TSRMLS_DC after the last declared argument.  Always refer to
   the globals in your function as SOAPROTOBUF_G(variable).  You are 
   encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/

#ifdef ZTS
#define SOAPROTOBUF_G(v) TSRMG(soaprotobuf_globals_id, zend_soaprotobuf_globals *, v)
#else
#define SOAPROTOBUF_G(v) (soaprotobuf_globals.v)
#endif

#endif	/* PHP_SOAPROTOBUF_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
