#include <ichor/syscalls.h>

typedef struct __attribute__((packed))
{
    PortMessageHeader header;
    int empty;
    char character;
    char str[15];
} MyMessage;

#define PACKED __attribute__((packed))

typedef struct PACKED
{
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf64Header;

typedef struct PACKED
{
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64ProgramHeader;

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

#define PT_LOAD 0x00000001
#define PT_INTERP 0x00000003
#define PT_PHDR 0x00000006

#define ALIGN_UP(x, align) (((x) + (align)-1) & ~((align)-1))
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))
#define DIV_CEIL(x, align) (((x) + (align)-1) / (align))

void *memset(void *ptr, int c, size_t n)
{

    for (size_t i = 0; i < n; i++)
    {
        ((char *)ptr)[i] = c;
    }

    return ptr;
}

uintptr_t elf_load(void *elf_buffer, Task *task)
{
    Elf64Header *header = (Elf64Header *)elf_buffer;

    if (header->e_ident[0] != 0x7F || header->e_ident[1] != 'E' || header->e_ident[2] != 'L' || header->e_ident[3] != 'F')
    {
        sys_log("Bruh not an elf");
        return 0;
    }

    Elf64ProgramHeader *program_header = (Elf64ProgramHeader *)((uintptr_t)elf_buffer + header->e_phoff);

    for (int i = 0; i < header->e_phnum; i++)
    {
        if (program_header->p_type == PT_LOAD)
        {
            size_t misalign = program_header->p_vaddr & (4096 - 1);
            size_t page_count = DIV_CEIL(misalign + program_header->p_memsz, 4096);

            for (size_t i = 0; i < page_count; i++)
            {
                VmObject obj = sys_vm_create(4096, 0, 0);
                sys_vm_map(task->space, &obj, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC, program_header->p_vaddr, VM_MAP_FIXED);
                sys_vm_write(task->space, program_header->p_vaddr, (void *)(elf_buffer + program_header->p_offset), program_header->p_filesz);
            }
        }

        program_header = (Elf64ProgramHeader *)((uint8_t *)program_header + header->e_phentsize);
    }

    return header->e_entry;
}

void _start(Charon *charon)
{
    Port port;
    Task task;
    VmObject obj;
    uintptr_t entry;
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

    entry = elf_load(obj.buf, &task);

    sys_start_task(&task, entry);

    int bytes_received = 0;

    while (bytes_received == 0)
    {
        bytes_received = sys_msg(PORT_RECV, port, sizeof(message), &message.header);
    }

    sys_log("Got message: ");
    sys_log(message.str);

    sys_exit(0);
}
