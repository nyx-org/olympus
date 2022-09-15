#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PORT_RIGHT_RECV (1 << 0)
#define PORT_RIGHT_SEND (1 << 1)

#define PORT_QUEUE_MAX 16

#define PORT_MSG_TYPE_DEFAULT (1 << 0)
#define PORT_MSG_TYPE_RIGHT (1 << 1)
#define PORT_BOOTSTRAP 0

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint32_t size;
    uint32_t dest;
    uint32_t port_right;
    uint8_t port_type;
} PortMessageHeader;

static void log(const char *str)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(0), "D"(str));
}

static void send_port(PortMessageHeader *message)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(2), "D"(message));
}

static int get_port(uint8_t index)
{
    int ret;
    __asm__ volatile("int $0x42"
                     : "=a"(ret)
                     : "a"(6), "D"(index));
    return ret;
}

static void exit(void)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(7));
}

typedef struct __attribute__((packed))
{
    PortMessageHeader header;
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
    log("Hello I am 'hello'");

    MyMessage message;
    message.header.dest = get_port(PORT_BOOTSTRAP);
    message.header.type = PORT_MSG_TYPE_DEFAULT;
    message.header.size = sizeof(message);
    message.character = 'h';
    message.str[0] = 'h';
    message.str[1] = 'e';
    message.str[2] = 'l';
    message.str[3] = 'l';
    message.str[4] = 'o';
    message.str[5] = '\0';

    log("Sending str \"hello\"");

    send_port(&message.header);
    exit();
}
