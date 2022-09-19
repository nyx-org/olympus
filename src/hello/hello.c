#include <gaia.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct __attribute__((packed))
{
    GaiaMessageHeader header;
    int empty;
    char character;
    char str[15];
} MyMessage;

void *memcpy(void *dest, const void *src, uint64_t n)
{
    char *d = dest;
    const char *s = src;
    for (uint64_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }
    return dest;
}

void _start()
{
    gaia_log("Hello I am 'hello'");

    MyMessage message;
    message.header.dest = gaia_get_common_port(PORT_BOOTSTRAP);
    message.header.type = PORT_MSG_TYPE_DEFAULT;
    message.header.size = sizeof(message);
    message.character = 'h';
    message.empty = 0;
    message.str[0] = 'h';
    message.str[1] = 'e';
    message.str[2] = 'l';
    message.str[3] = 'l';
    message.str[4] = 'o';
    message.str[5] = '\0';

    gaia_log("Sending str \"hello\"");

    gaia_msg(PORT_SEND, PORT_NULL, 0, &message.header);
    gaia_exit(0);
}
