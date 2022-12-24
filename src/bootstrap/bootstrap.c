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

typedef struct
{
    char name[256];
    Port port;
} Binding;

#define CHECK_ERRNO() (sys_errno == ERR_SUCCESS ? (void)0 : ichor_debug("Error: %d", sys_errno))

static void execute_task(CharonModule *module)
{
    VmObject obj;
    Task task = sys_create_task(RIGHT_NULL);

    CHECK_ERRNO();

    sys_vm_register_dma_region(NULL, module->address, module->size, 0);

    CHECK_ERRNO();

    obj = sys_vm_create(module->size, module->address, VM_MEM_DMA);

    CHECK_ERRNO();

    sys_vm_map(NULL, &obj, VM_PROT_READ | VM_PROT_WRITE, 0, VM_MAP_ANONYMOUS | VM_MAP_DMA);

    if (ichor_exec_elf(&task, obj.buf) != ERR_SUCCESS)
    {
        ichor_debug("Failed to spawn task.. hanging, error: %d", sys_errno);
        sys_exit(-1);
    }
}

static int register_server(BootstrapReq req, BootstrapResponse *resp)
{
    ichor_debug("register_server(%s, %d)", req.requests.register_server.name, req.header.port_right);
    resp->_data.i32_val = 0;
    return ERR_SUCCESS;
}

static int look_up(BootstrapReq req, BootstrapResponse *resp)
{
    ichor_debug("look_up(%s)", req.requests.look_up.name);
    resp->_data.i32_val = 0;
    return ERR_SUCCESS;
}

static void server_loop(Port port)
{
    BootstrapReq msg = {0};

    int (*funcs[])(BootstrapReq, BootstrapResponse *) = {
        [BOOTSTRAP_REGISTER_SERVER] = register_server,
        [BOOTSTRAP_LOOK_UP] = look_up,
    };

    while (true)
    {
        ichor_wait_for_message(port, sizeof(msg), &msg.header);

        if (msg.call > sizeof(funcs) / sizeof(funcs[0]))
        {
            ichor_debug("Invalid call");
        }
        else
        {
            BootstrapResponse resp = {.header.size = sizeof(resp), .header.dest = msg.header.port_right};
            funcs[msg.call](msg, &resp);

            sys_msg(PORT_SEND, PORT_NULL, -1, &resp.header);
        }
    }
}

void _start(Charon *charon)
{
    Port port;
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

    execute_task(module);

    server_loop(port);

    sys_exit(0);
}
