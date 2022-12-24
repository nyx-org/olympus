#include <bootstrap.h>
#include <ichor/debug.h>
#include <ichor/port.h>
#include <ichor/syscalls.h>
#include <stdc-shim/string.h>

void _start()
{
    Port port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    bootstrap_register_server("org.nyx.hello", port);

    sys_free_port(port);

    sys_exit(0);
}
