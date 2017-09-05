#include "conf.h"
#include "iniparser.h"

#include <string.h>  //memcpy,strtok_r
#include <stdlib.h>  //atoi,getenv
#include <ctype.h>   //tolower


static dictionary * global_config_dictionary;

/**
 * [config_init description]
 * @param  config_file_path [description]
 * @return                  [0 means success, -1 means failed]
 */
int config_init(const char * config_file_path) {

	global_config_dictionary = iniparser_load(config_file_path);
	if (global_config_dictionary == NULL) {
		fprintf(stderr, "cannot parse file: %s\n", config_file_path);
		return -1;
	}

	// iniparser_freedict(ini);
	return 0;
}

/**
 * [get_config_item description]
 * @param  config_item_name [format is [section name]:[config item name]]
 * @param  result           [description]
 * @param  len              [description]
 * @param  default_value    [description]
 * @return                  [0 means success, -1 means failed]
 */
int get_config_item(const char * config_item_name, char * result, size_t len, const char * default_value) {
	char * s = iniparser_getstring(global_config_dictionary, config_item_name, NULL);
	if (s) {
		//found
		char * proxy_home_value = getenv("AGENT_HOME");
		if (proxy_home_value) {
			substitute(s, result, len, "${AGENT_HOME}", proxy_home_value);
		} else {
			snprintf(result, len, "%s", s );
		}
	} else {
		//not found
		snprintf(result, len, "%s", default_value );
	}
	return 0;
}

/**
 * [get_config_item_int description]
 * @param  config_item_name [description]
 * @param  default_value    [description]
 * @return                  [real value or default value]
 */
int get_config_item_int(const char * config_item_name, int default_value) {
	char * value = iniparser_getstring(global_config_dictionary, config_item_name, NULL);
	if (value) {
		//found
		int result_value = 1;
		char str [200] = {0};
		memcpy ( str, value, strlen(value) );
		char delims[] = "*";
		char * result = NULL;
		char * ptrptr = NULL;
		result = strtok_r( str, delims, &ptrptr );
		while ( result != NULL )
		{
			result_value *= atoi(result);
			result = strtok_r( NULL, delims, &ptrptr );
		}
		return result_value;
	} else {
		//not found
		return default_value;
	}
}

