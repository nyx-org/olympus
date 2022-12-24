#include <bootstrap_srv.h>
#include <ichor/alloc.h>
#include <ichor/charon.h>
#include <ichor/debug.h>
#include <ichor/elf.h>
#include <ichor/error.h>
#include <ichor/port.h>
#include <ichor/rights.h>
#include <ichor/syscalls.h>
#include <stdc-shim/string.h>

void _start(Charon *charon)
{
    Port port;
    Task task;
    VmObject obj;
    CharonModule *module = NULL;

    // We can receive and send from/to this port
    port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    sys_register_common_port(PORT_COMMON_BOOTSTRAP, port);

    ichor_debug("hello, world");

    for (size_t i = 0; i < charon->modules.count; i++)
    {
        if (strncmp(charon->modules.modules[i].name, "/hello.elf", 10) == 0)
        {
            module = &charon->modules.modules[i];
            ichor_debug("Found hello.elf module! Starting task..");
            break;
        }
    }

    if (!module)
    {
        ichor_debug("Failed to find hello.elf module.. hanging");
        sys_exit(-1);
    }

    task = sys_create_task(RIGHT_NULL);

    if (sys_errno != ERR_SUCCESS)
    {
        ichor_debug("failed to create task, error: %d", sys_errno);
    }

    sys_vm_register_dma_region(NULL, module->address, module->size, 0);

    if (sys_errno != ERR_SUCCESS)
    {
        ichor_debug("failed to register dma region, error: %d", sys_errno);
    }

    obj = sys_vm_create(module->size, module->address, VM_MEM_DMA);

    if (sys_errno != ERR_SUCCESS)
    {
        ichor_debug("failed to create vm object, error: %d", sys_errno);
    }

    sys_vm_map(NULL, &obj, VM_PROT_READ | VM_PROT_WRITE, 0, VM_MAP_ANONYMOUS | VM_MAP_DMA);

    if (sys_errno != ERR_SUCCESS)
    {
        ichor_debug("failed to map vm object, error: %d", sys_errno);
    }

    if (ichor_exec_elf(&task, obj.buf) != ERR_SUCCESS)
    {
        ichor_debug("Failed to spawn task.. hanging, error: %d", sys_errno);
        sys_exit(-1);
    }

    ichor_debug("Waiting for messages");

    BootstrapReq msg = {0};

    while (true)
    {
        ichor_wait_for_message(port, sizeof(msg), &msg.header);

        if (msg.call == BOOTSTRAP_REGISTER_SERVER)
        {
            ichor_debug("register_server(%s, %d)", msg.requests.register_server.name, msg.header.port_right);

            BootstrapResponse response = {0};

            response.header.dest = msg.header.port_right;
            response.header.size = sizeof(response);

            response._data.i32_val = 0;

            sys_msg(PORT_SEND, PORT_NULL, -1, &response.header);
        }
    }

    sys_exit(0);
}
