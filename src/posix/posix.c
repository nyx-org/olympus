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

static int close(PosixReq request, PosixResponse *resp)
{
    RESP_VAL(resp, i32) = posix_sys_close(procs[REQ_MEMBER(close, pid)], REQ_MEMBER(close, fd));
    return 0;
}

//
static int readdir(PosixReq request, PosixResponse *resp)
{
    void *buf = (void *)(GET_SHMD(request, requests.readdir.buf).address);
    void *bytes_read = (void *)(GET_SHMD(request, requests.readdir.bytes_read).address);

    RESP_VAL(resp, i32) = posix_sys_readdir(procs[REQ_MEMBER(readdir, pid)], REQ_MEMBER(readdir, fd), buf, REQ_MEMBER(readdir, buf_size), bytes_read);
    return 0;
}

#define PROT_NONE 0x00
#define PROT_READ 0x01
#define PROT_WRITE 0x02
#define PROT_EXEC 0x04

#define MAP_FAILED ((void *)(-1))
#define MAP_FILE 0x00
#define MAP_SHARED 0x01
#define MAP_PRIVATE 0x02
#define MAP_FIXED 0x10
#define MAP_ANON 0x20
#define MAP_ANONYMOUS 0x20

static int mmap(PosixReq request, PosixResponse *resp)
{
    PosixMmapReq req = request.requests.mmap;

    void **out = (void **)(GET_SHMD(request, requests.mmap.out).address);

    VmObject obj;
    obj.size = req.size;
    size_t actual_flags = req.flags & 0xFFFFFFFF;

    ichor_debug("Hi");
    if (actual_flags & MAP_ANONYMOUS)
    {
        uint16_t gaia_prot = 0;

        if (req.prot & PROT_READ)
            gaia_prot |= VM_PROT_READ;
        if (req.prot & PROT_WRITE)
            gaia_prot |= VM_PROT_WRITE;
        if (req.prot & PROT_EXEC)
            gaia_prot |= VM_PROT_EXEC;

        sys_vm_map(req.space, &obj, gaia_prot, (uintptr_t)req.hint, VM_MAP_ANONYMOUS);
    }

    *out = obj.buf;

    resp->_data.i32_val = 0;

    return 0;
}

static int (*posix_funcs[])(PosixReq, PosixResponse *) = {
    [POSIX_OPEN] = open,
    [POSIX_WRITE] = write,
    [POSIX_STAT] = stat,
    [POSIX_SEEK] = seek,
    [POSIX_READ] = read,
    [POSIX_CLOSE] = close,
    [POSIX_READDIR] = readdir,
    [POSIX_MMAP] = mmap,
};

void server_main(Charon *charon)
{
    Port port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    ichor_debug("POSIX subsystem is going up");

    bootstrap_register_server("org.nyx.posix", port);

    uint8_t *ramdisk = get_ramdisk(charon);

    tmpfs_init();

    ichor_debug("Unpacking ramdisk..");

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