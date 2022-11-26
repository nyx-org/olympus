#include <ichor/elf.h>
#include <ichor/syscalls.h>
#include <stdc-shim/string.h>

typedef struct __attribute__((packed))
{
    PortMessageHeader header;
    int empty;
    char character;
    char str[15];
} MyMessage;

#define PACKED __attribute__((packed))

#define CHARON_MODULE_MAX 16
#define CHARON_MMAP_SIZE_MAX 128

typedef enum
{
    MMAP_FREE,
    MMAP_RESERVED,
    MMAP_MODULE,
    MMAP_RECLAIMABLE,
    MMAP_FRAMEBUFFER
} CharonMemoryMapEntryType;

typedef struct PACKED
{

    uintptr_t base;
    size_t size;
    CharonMemoryMapEntryType type;
} CharonMemoryMapEntry;

typedef struct PACKED
{
    uint8_t count;
    CharonMemoryMapEntry entries[CHARON_MMAP_SIZE_MAX];
} CharonMemoryMap;

typedef struct PACKED
{
    bool present;
    uintptr_t address;
    uint32_t width, height, pitch, bpp;
} CharonFramebuffer;

typedef struct PACKED
{
    uint32_t size;
    uintptr_t address;
    const char name[16];
} CharonModule;

typedef struct PACKED
{
    uint8_t count;
    CharonModule modules[CHARON_MODULE_MAX];
} CharonModules;

typedef struct PACKED
{
    uintptr_t rsdp;
    CharonFramebuffer framebuffer;
    CharonModules modules;
    CharonMemoryMap memory_map;
} Charon;

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

    task = sys_create_task();

    obj = sys_vm_create(charon->modules.modules[0].size, charon->modules.modules[0].address, VM_MEM_DMA);

    sys_vm_map(NULL, &obj, VM_PROT_READ | VM_PROT_WRITE, 0, VM_MAP_ANONYMOUS | VM_MAP_PHYS);

    ichor_exec_elf(&task, obj.buf);

    int bytes_received = 0;

    while (bytes_received == 0)
    {
        bytes_received = sys_msg(PORT_RECV, port, sizeof(message), &message.header);
    }

    sys_log("Got message: ");
    sys_log(message.str);

    sys_exit(0);
}
