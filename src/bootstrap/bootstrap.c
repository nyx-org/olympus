#include <ichor/charon.h>
#include <ichor/elf.h>
#include <ichor/error.h>
#include <ichor/rights.h>
#include <ichor/syscalls.h>
#include <stdc-shim/string.h>

typedef struct __attribute__((packed))
{
    PortMessageHeader header;
    int empty;
    char character;
    char str[15];
} MyMessage;

void _start(Charon *charon)
{
    Port port;
    Task task;
    VmObject obj;
    MyMessage message;

    // We can receive and send from/to this port
    port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    sys_register_common_port(PORT_COMMON_BOOTSTRAP, port);

    sys_log("Spawning first task..");

    for (size_t i = 0; i < charon->modules.count; i++)
    {
        sys_log(charon->modules.modules[i].name);
    }

    task = sys_create_task(RIGHT_NULL);

    if (sys_errno != ERR_SUCCESS)
    {
        sys_log("failed to create task");
    }

    sys_vm_register_dma_region(NULL, charon->modules.modules[0].address, charon->modules.modules[0].size, 0);

    if (sys_errno != ERR_SUCCESS)
    {
        sys_log("failed to register dma region");
    }

    obj = sys_vm_create(charon->modules.modules[0].size, charon->modules.modules[0].address, VM_MEM_DMA);

    if (sys_errno != ERR_SUCCESS)
    {
        sys_log("failed to create vm object");
    }

    sys_vm_map(NULL, &obj, VM_PROT_READ | VM_PROT_WRITE, 0, VM_MAP_ANONYMOUS | VM_MAP_DMA);

    if (sys_errno != ERR_SUCCESS)
    {
        sys_log("failed to map vm object");
    }

    if (ichor_exec_elf(&task, obj.buf) != ERR_SUCCESS)
    {
        sys_log("Failed to spawn task.. hanging");
        sys_exit(-1);
        while (true)
        {
        }
    }

    int bytes_received = 0;

    while (bytes_received == 0)
    {
        bytes_received = sys_msg(PORT_RECV, port, sizeof(message), &message.header);
    }

    sys_log("Got message: ");
    sys_log(message.str);

    sys_exit(0);
}
