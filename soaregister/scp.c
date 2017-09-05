/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Kevin xu                                                     |
  +----------------------------------------------------------------------+
*/

/**
 * configuration as following:
 * extension=scp.so
 *
 *
 *
 */


/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_scp.h"
#include "service_mgt.h"
#include "log.h"
#include "util.h"
#include <pthread.h>

/* If you declare any globals in php_scp.h uncomment this:*/
ZEND_DECLARE_MODULE_GLOBALS(scp)



/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
*/
/* }}} */
PHP_INI_BEGIN()
STD_PHP_INI_ENTRY("scp.zk_addrs",               "127.0.0.1:2181", PHP_INI_ALL, OnUpdateString, g_zk_addrs, zend_scp_globals, scp_globals)
STD_PHP_INI_ENTRY("scp.log_path",               "/tmp/soa/provider.log", PHP_INI_ALL, OnUpdateString, g_log_path, zend_scp_globals, scp_globals)
STD_PHP_INI_ENTRY("scp.log_level",              "info", PHP_INI_ALL, OnUpdateString, g_log_level, zend_scp_globals, scp_globals)
STD_PHP_INI_ENTRY("scp.log_lock_type",          "thread", PHP_INI_ALL, OnUpdateString, g_log_lock_type, zend_scp_globals, scp_globals)
STD_PHP_INI_ENTRY("scp.log_rotate_size",        "104857600", PHP_INI_ALL, OnUpdateLongGEZero, g_log_rotate_size, zend_scp_globals, scp_globals)
PHP_INI_END()



/* {{{ proto int register_service_by_file(string zk_addrs, string service_define_config_file)
    */
PHP_FUNCTION(register_service_by_file)
{
  char *zk_addrs = NULL;
  char *service_define_config_file = NULL;
  int argc = ZEND_NUM_ARGS();
  int zk_addrs_len;
  int service_define_config_file_len;

  if (zend_parse_parameters(argc TSRMLS_CC, "ss", &zk_addrs, &zk_addrs_len, &service_define_config_file, &service_define_config_file_len) == FAILURE)
    return;

  if (provider_stub_init(SCP_G(g_zk_addrs), service_define_config_file)) {
    RETURN_LONG(-1);
  }

  RETURN_LONG(0);
}
/* }}} */


/* {{{ proto int scp_write_node_value(string output_file_path)
    */
PHP_FUNCTION(scp_write_node_value)
{
  char *output_file_path = NULL;
  int argc = ZEND_NUM_ARGS();
  int output_file_path_len;

  if (zend_parse_parameters(argc TSRMLS_CC, "s", &output_file_path, &output_file_path_len) == FAILURE)
    return;

  const char * node_value = get_cur_node_path();
  write_content_to_file(output_file_path, node_value, strlen(node_value));

  RETURN_LONG(0);
}
/* }}} */




/* {{{ php_scp_init_globals
 */
/* Uncomment this function if you have INI entries
*/
/* }}} */
static void php_scp_init_globals(zend_scp_globals *scp_globals)
{
  scp_globals->g_zk_addrs = NULL;
  // scp_globals->g_service_define_config = NULL;
  scp_globals->g_log_path = NULL;
  scp_globals->g_log_level = NULL;
  scp_globals->g_log_lock_type = NULL;
  scp_globals->g_log_rotate_size = 1024 * 1024 * 100;
}


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(scp)
{
  ZEND_INIT_MODULE_GLOBALS(scp, php_scp_init_globals, NULL);
  REGISTER_INI_ENTRIES();

  if (open_log(SCP_G(g_log_path), SCP_G(g_log_level), SCP_G(g_log_lock_type), SCP_G(g_log_rotate_size))) {
    return FAILURE;
  }

  return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(scp)
{
  UNREGISTER_INI_ENTRIES();

  provider_stub_destroy();
  close_log();

  return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(scp)
{
  return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(scp)
{
  return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(scp)
{
  php_info_print_table_start();
  php_info_print_table_header(2, "scp support", "enabled");
  php_info_print_table_end();

  /* Remove comments if you have entries in php.ini
  */
  DISPLAY_INI_ENTRIES();

}
/* }}} */

/* {{{ scp_functions[]
 *
 * Every user visible function must have an entry in scp_functions[].
 */
const zend_function_entry scp_functions[] = {
  PHP_FE(register_service_by_file,  NULL)
  PHP_FE(scp_write_node_value,  NULL)
  PHP_FE_END  /* Must be the last line in scp_functions[] */
};
/* }}} */

/* {{{ scp_module_entry
 */
zend_module_entry scp_module_entry = {
  STANDARD_MODULE_HEADER,
  "scp",
  scp_functions,
  PHP_MINIT(scp),
  PHP_MSHUTDOWN(scp),
  PHP_RINIT(scp),     /* Replace with NULL if there's nothing to do at request start */
  PHP_RSHUTDOWN(scp), /* Replace with NULL if there's nothing to do at request end */
  PHP_MINFO(scp),
  PHP_SCP_VERSION,
  STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_SCP
ZEND_GET_MODULE(scp)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
