#include <bootstrap.h>
#include <fs/tar.h>
#include <fs/tmpfs.h>
#include <fs/vfs.h>
#include <posix/dirent.h>
#include <posix/fnctl.h>
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

void server_main(Charon *charon)
{
    Port port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    ichor_debug("POSIX subsystem is going up");

    bootstrap_register_server("org.nyx.posix", port);

    uint8_t *ramdisk = get_ramdisk(charon);


    tmpfs_init();

    ichor_debug("Unpacking ramdisk");

    tar_write_on_tmpfs(ramdisk);

    Proc *proc = ichor_malloc(sizeof(Proc));
    struct stat file_stat;

    int fd = posix_sys_open(proc, "/usr/hello.txt", O_RDWR);

    if (posix_sys_stat(proc, fd, "", &file_stat) != 0)
        POSIX_PANIC("error in stat");

    char *buf = ichor_malloc(file_stat.st_size);

    if (posix_sys_read(proc, fd, buf, file_stat.st_size) < 0)
        POSIX_PANIC("error in read");

    ichor_debug("FD: %d, filesize: %d, buf: %s", fd, file_stat.st_size, buf);

    posix_sys_close(proc, fd);
    ichor_free(buf);
    ichor_free(proc);

    while (true)
    {
    }

    sys_exit(0);
}