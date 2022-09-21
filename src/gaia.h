#ifndef _GAIA_H
#define _GAIA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PORT_SEND (0)
#define PORT_RECV (1)

#define PORT_RIGHT_RECV (1 << 0)
#define PORT_RIGHT_SEND (1 << 1)

#define PORT_QUEUE_MAX 16

#define PORT_MSG_TYPE_DEFAULT (1 << 0)
#define PORT_MSG_TYPE_RIGHT (1 << 1)

#define PORT_DEAD (~0)
#define PORT_NULL (0)

#define PORT_BOOTSTRAP 0

#define GAIA_SYS_LOG 0
#define GAIA_SYS_EXIT 1
#define GAIA_SYS_MSG 2
#define GAIA_SYS_ALLOC_PORT 3
#define GAIA_SYS_GET_PORT 4
#define GAIA_SYS_REGISTER_PORT 5
#define GAIA_SYS_SPAWN 6

typedef uint32_t GaiaPort;

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint32_t size;
    uint32_t dest;
    uint32_t port_right;
    uint8_t port_type;
} GaiaMessageHeader;

void gaia_log(const char *str);
size_t gaia_msg(uint8_t type, GaiaPort port_to_receive, size_t bytes_to_receive, GaiaMessageHeader *msg);

GaiaPort gaia_allocate_port(uint8_t rights);
void gaia_register_common_port(uint8_t index, GaiaPort port);
GaiaPort gaia_get_common_port(uint8_t index);

void gaia_exit(int status);
void gaia_spawn_module(const char *name);

#endif
