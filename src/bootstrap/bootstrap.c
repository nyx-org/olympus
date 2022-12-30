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

static Binding bindings[64];
static size_t binding_count = 0;

#define CHECK_ERRNO() (sys_errno == ERR_SUCCESS ? (void)0 : ichor_debug("Error: %d", sys_errno))

static void execute_task(CharonModule *module, Rights rights)
{
    VmObject obj = {0};
    Task task = sys_create_task(rights);
    Task my_task;

    CHECK_ERRNO();

    sys_get_current_task(&my_task);

    CHECK_ERRNO();

    sys_vm_register_dma_region(my_task.space, module->address, module->size, 0);

    CHECK_ERRNO();

    obj = sys_vm_create(module->size, module->address, VM_MEM_DMA);

    CHECK_ERRNO();

    sys_vm_map(my_task.space, &obj, VM_PROT_READ | VM_PROT_WRITE, 0, VM_MAP_ANONYMOUS | VM_MAP_DMA);

    if (ichor_exec_elf(&task, obj.buf) != ERR_SUCCESS)
    {
        ichor_debug("Failed to spawn task.. hanging, error: %d", sys_errno);
        sys_exit(-1);
    }
}

static int register_server(BootstrapReq req, BootstrapResponse *resp)
{
    Binding new_binding;

    strncpy(new_binding.name, req.requests.register_server.name, strlen(req.requests.register_server.name));

    new_binding.port = req.header.port_right;

    bindings[binding_count++] = new_binding;

    resp->_data.i32_val = ERR_SUCCESS;
    return ERR_SUCCESS;
}

static int look_up(BootstrapReq req, BootstrapResponse *resp)
{

    bool found = false;

    resp->header.type = PORT_MSG_TYPE_RIGHT;

    for (size_t i = 0; i < binding_count; i++)
    {
        if (!strncmp(bindings[i].name, req.requests.look_up.name, strlen(req.requests.look_up.name)))
        {
            resp->header.port_right = bindings[i].port;
            found = true;
            break;
        }
    }

    if (found)
    {
        resp->_data.i32_val = resp->header.port_right;
    }
    else
    {
        resp->header.type = PORT_MSG_TYPE_DEFAULT;
        resp->header.port_right = PORT_NULL;
    }

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
            ichor_debug("Invalid call: %d", msg.call);
        }
        else
        {
            BootstrapResponse resp = {.header.size = sizeof(resp), .header.dest = msg.header.port_right};
            resp.header.type = PORT_MSG_TYPE_DEFAULT;
            funcs[msg.call](msg, &resp);

            sys_msg(PORT_SEND, PORT_NULL, -1, &resp.header);
        }
    }
}

void server_main(Charon *charon)
{
    Port port;
    CharonModule *hello_module = NULL, *posix_module = NULL;

    // We can receive and send from/to this port
    port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    sys_register_common_port(PORT_COMMON_BOOTSTRAP, port);

    ichor_debug("hello, world");

    for (size_t i = 0; i < charon->modules.count; i++)
    {
        if (strncmp(charon->modules.modules[i].name, "/hello.elf", strlen("/hello.elf")) == 0)
        {
            hello_module = &charon->modules.modules[i];
            continue;
        }

        if (strncmp(charon->modules.modules[i].name, "/posix.elf", strlen("/posix.elf")) == 0)
        {
            posix_module = &charon->modules.modules[i];
            continue;
        }

        if (hello_module && posix_module)
            break;
    }

    if (!hello_module || !posix_module)
    {
        ichor_debug("Failed to find module.. hanging");
        sys_exit(-1);
    }

    execute_task(hello_module, RIGHT_NULL);
    execute_task(posix_module, RIGHT_DMA | RIGHT_REGISTER_DMA);

    server_loop(port);

    sys_exit(0);
}
