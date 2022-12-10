#include <ichor/charon.h>
#include <ichor/debug.h>
#include <ichor/elf.h>
#include <ichor/error.h>
#include <ichor/port.h>
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
    CharonModule *module;

    // We can receive and send from/to this port
    port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    sys_register_common_port(PORT_COMMON_BOOTSTRAP, port);

    for (size_t i = 0; i < charon->modules.count; i++)
    {
        ichor_debug("Module: %s", charon->modules.modules[i].name);

        if (strncmp(charon->modules.modules[i].name, "/hello.elf", 10) == 0)
        {
            module = &charon->modules.modules[i];
            ichor_debug("Found hello.elf module! Starting task..");
            break;
        }
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
        while (true)
        {
        }
    }

    while (true)
    {
        ichor_wait_for_message(port, sizeof(message), &message.header);

        if (message.header.port_right == PORT_NULL)
        {
            continue;
        }

        MyMessage message_to_send;
        message_to_send.header.dest = message.header.port_right;
        message_to_send.header.type = PORT_MSG_TYPE_DEFAULT;
        message_to_send.header.size = sizeof(message);
        message_to_send.character = 'h';
        message_to_send.empty = 0;

        char *str = "hello lol";

        memcpy(message_to_send.str, str, strlen(str));

        ichor_debug("Got message: %s, sending message: %s", (char *)message.str, str);

        sys_msg(PORT_SEND, PORT_NULL, -1, &message_to_send.header);
    }
}
