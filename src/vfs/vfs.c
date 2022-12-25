#include <bootstrap.h>
#include <ichor/base.h>
#include <ichor/charon.h>
#include <ichor/debug.h>
#include <vfs_srv.h>

void server_main(Charon *charon)
{
    Port port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);
    ichor_debug("hello i am da vfs");

    bootstrap_register_server("org.nyx.vfs", port);

    while (true)
    {
    }
}