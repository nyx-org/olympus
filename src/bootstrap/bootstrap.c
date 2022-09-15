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

static uint32_t alloc_port(uint8_t rights)
{
    uint32_t ret;
    __asm__ volatile("int $0x42"
                     : "=a"(ret)
                     : "a"(1), "D"(rights));
    return ret;
}

static int recv_port(uint32_t port, PortMessageHeader *message, uint64_t bytes)
{
    int ret;
    __asm__ volatile("int $0x42"
                     : "=a"(ret)
                     : "a"(3), "D"(port), "S"(message), "d"(bytes));
    return ret;
}

static void register_port(uint32_t port, uint8_t index)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(5), "D"(port), "S"(index));
}

static int spawn(const char *name)
{
    int ret;
    __asm__ volatile("int $0x42"
                     : "=a"(ret)
                     : "a"(4), "D"(name));
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

    // We can receive and send from/to this port
    uint32_t name = alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    register_port(PORT_BOOTSTRAP, name);

    log("Spawning hello...");
    spawn("/hello");

    MyMessage message;
    int found = 0;

    while (!found)
    {
        found = recv_port(name, &message.header, sizeof(message));
    }

    log("Got message: ");
    log(message.str);

    exit();
}
