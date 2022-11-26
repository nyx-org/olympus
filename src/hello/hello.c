#include <ichor/port.h>
#include <ichor/syscalls.h>
#include <stdc-shim/string.h>

typedef struct __attribute__((packed))
{
    PortMessageHeader header;
    int empty;
    char character;
    char str[15];
} MyMessage;

void _start()
{
    sys_log("Hello I am 'hello'");

    MyMessage message;
    message.header.dest = sys_get_common_port(PORT_COMMON_BOOTSTRAP);
    message.header.type = PORT_MSG_TYPE_DEFAULT;
    message.header.size = sizeof(message);
    message.character = 'h';
    message.empty = 0;

    char *str = "hello world";

    memcpy(message.str, str, strlen(str));

    sys_log("Sending str \"hello\"");

    sys_msg(PORT_SEND, PORT_NULL, 0, &message.header);
    sys_exit(0);
}
