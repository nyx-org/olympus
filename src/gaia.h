#ifndef _GAIA_H
#define _GAIA_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PORT_SEND (0)
#define PORT_RECV (1)

#define PORT_RIGHT_RECV (1 << 0)
#define PORT_RIGHT_SEND (1 << 1)

#define PORT_QUEUE_MAX 16

#define PORT_MSG_TYPE_DEFAULT (1 << 0)
#define PORT_MSG_TYPE_RIGHT (1 << 1)

#define PORT_DEAD (~0)
#define PORT_NULL (0)

#define PORT_BOOTSTRAP 0

#define GAIA_SYS_LOG 0
#define GAIA_SYS_EXIT 1
#define GAIA_SYS_MSG 2
#define GAIA_SYS_ALLOC_PORT 3
#define GAIA_SYS_GET_PORT 4
#define GAIA_SYS_REGISTER_PORT 5
#define GAIA_SYS_VM_MAP 6
#define GAIA_SYS_CREATE_TASK 7
#define GAIA_SYS_START_TASK 8
#define GAIA_SYS_VM_WRITE 9

#define MMAP_ANONYMOUS (1 << 0)
#define MMAP_FIXED (1 << 1)
#define MMAP_SHARED (1 << 2)
#define MMAP_PRIVATE (1 << 3)

/* NOTE: this is specific to gaia */
#define MMAP_PHYS (1 << 4)

#define PROT_NONE (1 << 0)
#define PROT_READ (1 << 1)
#define PROT_WRITE (1 << 2)
#define PROT_EXEC (1 << 3)

#define PACKED __attribute__((packed))

typedef uint32_t GaiaPort;

typedef struct __attribute__((packed))
{
    uint8_t type;
    uint32_t size;
    uint32_t dest;
    uint32_t port_right;
    uint8_t port_type;
} GaiaMessageHeader;

typedef struct __attribute__((packed))
{
    void *space;
    size_t pid;
} GaiaTask;

void gaia_log(const char *str);
size_t gaia_msg(uint8_t type, GaiaPort port_to_receive, size_t bytes_to_receive, GaiaMessageHeader *msg);

GaiaPort gaia_allocate_port(uint8_t rights);
void gaia_register_common_port(uint8_t index, GaiaPort port);
GaiaPort gaia_get_common_port(uint8_t index);

void gaia_exit(int status);

void gaia_create_task(GaiaTask *out);
void gaia_start_task(GaiaTask *task, uintptr_t entry_point, uintptr_t stack_pointer, bool allocate_stack);
void *gaia_vm_map(void *space, uint16_t prot, uint16_t flags, void *addr, void *phys, size_t size);
void gaia_vm_write(void *space, uintptr_t address, void *buffer, size_t count);

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

#endif
