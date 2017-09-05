#include "url_encode.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

//hex chars
static unsigned char hexchars[] = "0123456789ABCDEF";


static int hextoint(char *s)
{
    int value;
    int c;

    c = ((unsigned char *)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char *)s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}


/**
** url encode
**/
char * url_encode(char const *s, int len) {
    register unsigned char c;
    unsigned char *to, *start;
    unsigned char const *from, *end;

    from = (unsigned char *)s;
    end  = (unsigned char *)s + len;
    start = to = (unsigned char *) calloc(1, 3 * len + 1);

    while (from < end)
    {
        c = *from++;

        if (c == ' ')
        {
            *to++ = '+';
        }
        else if ((c < '0' && c != '-' && c != '.') ||
                 (c < 'A' && c > '9') ||
                 (c > 'Z' && c < 'a' && c != '_') ||
                 (c > 'z'))
        {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 0x0F];
            to += 3;
        }
        else
        {
            *to++ = c;
        }
    }
    *to = '\0';
    return (char *) start;
}

/**
** url decode
**/
char * url_decode(char const *str, int len) {
    char *data = str;
    char *dest = (char *)malloc(len);
    char *result = dest;

    while (len--)
    {
        if (*data == '+')
        {
            *dest = ' ';
        }
        else if (*data == '%' && len >= 2 && isxdigit((int) * (data + 1)) && isxdigit((int) * (data + 2)))
        {
            *dest = (char) hextoint(data + 1);
            data += 2;
            len -= 2;
        }
        else
        {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    return result;
}
