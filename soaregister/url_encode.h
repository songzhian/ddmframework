#ifndef _URL_ENCODE_H_
#define _URL_ENCODE_H_



#ifdef __cplusplus
extern "C" {
#endif

/**
** url encode
**/
char * url_encode(char const *s, int len);

/**
** url decode
**/
char * url_decode(char const *str, int len);


#ifdef __cplusplus
}
#endif

#endif
