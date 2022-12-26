#include <bootstrap.h>
#include <ichor/alloc.h>
#include <ichor/base.h>
#include <ichor/charon.h>
#include <ichor/debug.h>
#include <ichor/error.h>
#include <vfs_srv.h>

typedef struct
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char link_name[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char dev_major[8];
    char dev_minor[8];
    char prefix[155];
} TarHeader;

#define CHECK_ERRNO() (sys_errno == ERR_SUCCESS ? (void)0 : ichor_debug("Error: %d", sys_errno))
#define PANIC(string)    \
    ichor_debug(string); \
    sys_exit(-1)

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
        PANIC("Couldn't find ramdisk");
    }

    sys_vm_register_dma_region(NULL, module->address, module->size, 0);

    CHECK_ERRNO();

    obj = sys_vm_create(module->size, module->address, VM_MEM_DMA);

    CHECK_ERRNO();

    sys_vm_map(NULL, &obj, VM_PROT_READ | VM_PROT_WRITE, 0, VM_MAP_ANONYMOUS | VM_MAP_DMA);

    return obj.buf;
}

static inline uint64_t oct2int(const char *str, size_t len)
{
    uint64_t value = 0;
    while (*str && len > 0)
    {
        value = value * 8 + (*str++ - '0');
        len--;
    }
    return value;
}

static void list_files(uint8_t *address)
{
    TarHeader *current_file = (TarHeader *)address;

    while (strncmp(current_file->magic, "ustar", 5) == 0)
    {
        char *name = current_file->name;
        uint64_t size = oct2int(current_file->size, sizeof(current_file->size));

        ichor_debug("File: %s", name);

        current_file = (void *)current_file + 512 + ALIGN_UP(size, 512);
    }
}

static int read_file(const char *filename, uint8_t *address, void *buf)
{
    TarHeader *current_file = (TarHeader *)address;

    while (strncmp(current_file->magic, "ustar", 5) == 0)
    {
        char *name = current_file->name;
        uint64_t size = oct2int(current_file->size, sizeof(current_file->size));

        if (strncmp(name, filename, strlen(name)) == 0)
        {
            memcpy(buf, (void *)current_file + 512, size);
            return size;
        }

        current_file = (void *)current_file + 512 + ALIGN_UP(size, 512);
    }

    return 0;
}

void server_main(Charon *charon)
{
    Port port = sys_alloc_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    bootstrap_register_server("org.nyx.vfs", port);

    uint8_t *ramdisk = get_ramdisk(charon);

    char buf[4096] = {0};

    list_files(ramdisk);

    read_file("hello.txt", ramdisk, buf);
    ichor_debug("hello.txt: %s", buf);

    sys_exit(0);
}