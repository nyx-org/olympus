#include <gaia.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct __attribute__((packed))
{
    GaiaMessageHeader header;
    int empty;
    char character;
    char str[15];
} MyMessage;

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

uintptr_t elf_load(void *elf_buffer, GaiaTask *task)
{
    Elf64Header *header = (Elf64Header *)elf_buffer;
    Elf64ProgramHeader *program_header = (Elf64ProgramHeader *)((uintptr_t)elf_buffer + header->e_phoff);

    for (int i = 0; i < header->e_phnum; i++)
    {
        if (program_header->p_type == PT_LOAD)
        {
            size_t misalign = program_header->p_vaddr & (4096 - 1);
            size_t page_count = DIV_CEIL(misalign + program_header->p_memsz, 4096);

            for (size_t i = 0; i < page_count; i++)
            {
                gaia_vm_map(task->space, PROT_READ | PROT_WRITE | PROT_EXEC, MMAP_FIXED, (void *)program_header->p_vaddr, NULL, 4096);
                gaia_vm_write(task->space, program_header->p_vaddr, (void *)(elf_buffer + program_header->p_offset), program_header->p_filesz);
            }
        }

        program_header = (Elf64ProgramHeader *)((uint8_t *)program_header + header->e_phentsize);
    }

    return header->e_entry;
}

#define USER_STACK_TOP 0x7fffffffe000
void _start(Charon *charon)
{

    // We can receive and send from/to this port
    GaiaPort name = gaia_allocate_port(PORT_RIGHT_RECV | PORT_RIGHT_SEND);

    gaia_register_common_port(PORT_BOOTSTRAP, name);

    gaia_log("Spawning hello...");

    for (size_t i = 0; i < charon->modules.count; i++)
    {
        gaia_log(charon->modules.modules[i].name);
    }

    GaiaTask task;

    gaia_create_task(&task);

    void *elf = gaia_vm_map(NULL, PROT_READ | PROT_WRITE, MMAP_PHYS, NULL, (void *)charon->modules.modules[0].address, charon->modules.modules[0].size);

    uintptr_t entry = elf_load(elf, &task);

    gaia_start_task(&task, entry, USER_STACK_TOP, true);

    gaia_log("spawned first task");

    MyMessage message = {0};

    int bytes_received = 0;

    while (bytes_received == 0)
    {
        bytes_received = gaia_msg(PORT_RECV, name, sizeof(message), &message.header);
    }

    gaia_log("Got message: ");
    gaia_log(message.str);

    gaia_exit(0);
}
