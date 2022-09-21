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

void *memset(void *ptr, int c, size_t n)
{

    for (size_t i = 0; i < n; i++)
    {
        ((char *)ptr)[i] = c;
    }

    return ptr;
}

void _start(void)
{

    // We can receive and send from/to this port
    GaiaPort name = gaia_allocate_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    gaia_register_common_port(PORT_BOOTSTRAP, name);

    gaia_log("Spawning hello...");
    gaia_spawn_module("/hello");

    MyMessage message = {0};

    int bytes_received = 0;

    while (bytes_received == 0)
    {
        bytes_received = gaia_msg(PORT_RECV, name, sizeof(message), &message.header);
    }

    gaia_log("Got message: ");
    gaia_log(message.str);

    gaia_exit(0);
}
