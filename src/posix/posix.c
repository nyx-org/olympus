#include <bootstrap.h>
#include <fs/tar.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <posix/dirent.h>
#include <posix/fcntl.h>
#include <posix/posix.h>
#include <posix_srv.h>

static void *get_ramdisk(Charon *charon)
{
    CharonModule *module = NULL;
    VmObject obj;

    for (size_t i = 0; i < charon->modules.count; i++)
    {
        if (!strncmp(charon->modules.modules[i].name, "/ramdisk.tar", strlen(charon->modules.modules[i].name)))
        {
            module = &charon->modules.modules[i];
            break;
        }
    }

    if (!module)
    {
        POSIX_PANIC("Couldn't find ramdisk");
    }

    sys_vm_register_dma_region(NULL, module->address, module->size, 0);

    POSIX_CHECK_ERRNO();

    obj = sys_vm_create(module->size, module->address, VM_MEM_DMA);

    POSIX_CHECK_ERRNO();

    sys_vm_map(NULL, &obj, VM_PROT_READ | VM_PROT_WRITE, 0, VM_MAP_ANONYMOUS | VM_MAP_DMA);

    return obj.buf;
}

static Proc *procs[1024];

#define GET_SHMD(req, member) (req.header.shmds[req.member##_shmd])
#define RESP_VAL(resp, type) resp->_data.type##_val
#define REQ_MEMBER(call, member) request.requests.call.member

static int open(PosixReq request, PosixResponse *resp)
{
    if (!procs[request.requests.open.pid])
    {
        procs[request.requests.open.pid] = ichor_malloc(sizeof(Proc));
        procs[request.requests.open.pid]->pid = request.requests.open.pid;
        vec_init(&procs[request.requests.open.pid]->fds);
    }

    RESP_VAL(resp, i32) = posix_sys_open(procs[REQ_MEMBER(open, pid)], REQ_MEMBER(open, path), REQ_MEMBER(open, mode));

    return 0;
}

static int read(PosixReq request, PosixResponse *resp)
{
    void *buf = (void *)(GET_SHMD(request, requests.read.buf).address);

    RESP_VAL(resp, i32) = posix_sys_read(procs[REQ_MEMBER(read, pid)], REQ_MEMBER(read, fd), buf, REQ_MEMBER(read, buf_size));

    return 0;
}

static int write(PosixReq request, PosixResponse *resp)
{
    void *buf = (void *)(GET_SHMD(request, requests.write.buf).address);

    RESP_VAL(resp, i32) = posix_sys_write(procs[REQ_MEMBER(write, pid)], REQ_MEMBER(write, fd), buf, REQ_MEMBER(write, buf_size));

    return 0;
}

static int stat(PosixReq request, PosixResponse *resp)
{
    void *buf = (void *)(GET_SHMD(request, requests.stat.out).address);

    if ((size_t)REQ_MEMBER(stat, out_size) < sizeof(struct stat))
    {
        POSIX_PANIC("invalid stat size");
    }

    RESP_VAL(resp, i32) = posix_sys_stat(procs[REQ_MEMBER(stat, pid)], REQ_MEMBER(stat, fd), REQ_MEMBER(stat, path), buf);

    return 0;
}

static int seek(PosixReq request, PosixResponse *resp)
{
    RESP_VAL(resp, i32) = posix_sys_seek(procs[REQ_MEMBER(seek, pid)], REQ_MEMBER(seek, fd), REQ_MEMBER(seek, offset), REQ_MEMBER(seek, whence));
    return 0;
}

static int (*posix_funcs[])(PosixReq, PosixResponse *) = {
    [POSIX_OPEN] = open,
    [POSIX_WRITE] = write,
    [POSIX_STAT] = stat,
    [POSIX_SEEK] = seek,
    [POSIX_READ] = read,
};

void server_main(Charon *charon)
{
    Port port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    ichor_debug("My port is %d", port);

    ichor_debug("POSIX subsystem is going up");

    bootstrap_register_server("org.nyx.posix", port);

    uint8_t *ramdisk = get_ramdisk(charon);

    tmpfs_init();

    ichor_debug("Unpacking ramdisk");

    tar_write_on_tmpfs(ramdisk);

    PosixReq request = {0};

    while (true)
    {
        ichor_wait_for_message(port, sizeof(PosixReq), &request.header);
        PosixResponse resp = {0};

        resp.header.dest = request.header.port_right;
        resp.header.size = sizeof(resp);
        resp.header.type = PORT_MSG_TYPE_DEFAULT;

        if (request.call > sizeof(posix_funcs) / sizeof(posix_funcs[0]))
        {
            ichor_debug("Invalid call: %d", request.call);
        }
        else
        {
            PosixResponse resp = {.header.size = sizeof(resp), .header.dest = request.header.port_right};
            posix_funcs[request.call](request, &resp);

            sys_msg(PORT_SEND, PORT_NULL, -1, &resp.header);
        }
    }

    sys_exit(0);
}