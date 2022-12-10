#include "ichor/debug.h"
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

    Port port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    MyMessage message;
    message.header.dest = sys_get_common_port(PORT_COMMON_BOOTSTRAP);
    message.header.type = PORT_MSG_TYPE_RIGHT_ONCE;
    message.header.size = sizeof(message);
    message.character = 'h';
    message.empty = 0;
    message.header.port_right = port;

    char *str = "hello world";

    memcpy(message.str, str, strlen(str));

    ichor_debug("Sending str \"%s\"", str);

    sys_msg(PORT_SEND, PORT_NULL, 0, &message.header);

    int bytes_received = 0;
    MyMessage received_message;
    while (!bytes_received)
    {
        bytes_received = sys_msg(PORT_RECV, port, sizeof(MyMessage), &received_message.header);
    }
    ichor_debug("Received string %s", received_message.str);

    sys_exit(0);
}
