#include "ichor/error.h"
#include <ichor/elf.h>
#include <ichor/rights.h>
#include <posix/posix.h>

#define AT_NULL 0
#define AT_IGNORE 1
#define AT_EXECFD 2
#define AT_PHDR 3
#define AT_PHENT 4
#define AT_PHNUM 5
#define AT_ENTRY 9

typedef struct
{
    uintptr_t at_entry;
    uintptr_t at_phdr;
    uintptr_t at_phent;
    uintptr_t at_phnum;
} Auxval;

static uintptr_t load_elf(Task *task, Vnode *file, Auxval *auxval, uintptr_t base, char *ld_path)
{
    Elf64Header header = {0};

    VOP_READ(file, &header, sizeof(Elf64Header), 0);

    if (header.e_ident[0] != ELFMAG0 ||
        header.e_ident[1] != ELFMAG1 ||
        header.e_ident[2] != ELFMAG2 ||
        header.e_ident[3] != ELFMAG3)
    {
        return ERR_INVALID_PARAMETERS;
    }

    auxval->at_phdr = 0;
    auxval->at_phent = header.e_phentsize;
    auxval->at_phnum = header.e_phnum;

    for (int i = 0; i < header.e_phnum; i++)
    {
        Elf64ProgramHeader phdr;

        VOP_READ(file, &phdr, header.e_phentsize, header.e_phoff + i * header.e_phentsize);

        void *buf = ichor_malloc(phdr.p_filesz);
        VOP_READ(file, buf, phdr.p_filesz, phdr.p_offset);

        switch (phdr.p_type)
        {
        case PT_LOAD:
        {
            size_t misalign = phdr.p_vaddr & (4096 - 1);
            size_t page_count = DIV_CEIL(misalign + phdr.p_memsz, 4096);

            VmObject obj = sys_vm_create(page_count * 4096, 0, 0);
            sys_vm_map(task->space, &obj, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC, base + phdr.p_vaddr, VM_MAP_FIXED);

            sys_vm_write(task->space, base + phdr.p_vaddr + misalign, (void *)(buf), phdr.p_filesz);

            ichor_free(buf);
            break;
        }
        case PT_PHDR:
            auxval->at_phdr = base + phdr.p_vaddr;
            break;

        case PT_INTERP:
        {
            if (!ld_path)
                break;

            memcpy(ld_path, buf, phdr.p_filesz);

            (ld_path)[phdr.p_filesz] = 0;

            ichor_free(buf);
        }
        }
    }

    auxval->at_entry = base + header.e_entry;
    return ERR_SUCCESS;
}

int posix_sys_fork(Proc *proc)
{
    proc->forked = true;
    return 0;
}

// kinda cursed?
#define USER_STACK_TOP 0x7fffffffe000

int posix_sys_execve(Proc *proc, const char *path, char const *argv[], char const *envp[])
{
    if (!proc->forked)
    {
        POSIX_PANIC("Warning: exec() without fork() is not suppported");
    }

    Task new_task = sys_create_task(RIGHT_NULL);

    Vnode *vn;
    Vattr attr;

    int r = vfs_find_and(proc->cwd, &vn, path, VFS_FIND_OR_ERROR, &attr);
    uintptr_t stack_top = USER_STACK_TOP;
    size_t *stack_ptr = (size_t *)stack_top;
    Auxval auxval;
    char ld_path[255];
    size_t nargs = 0, nenv = 0;

    if (r < 0)
    {
        ichor_debug("Error: %d", r);
        return r;
    }

    load_elf(&new_task, vn, &auxval, 0, (char *)ld_path);

    for (char **elem = (char **)argv; *elem; elem++)
    {
        stack_ptr = (void *)stack_ptr - (strlen(*elem) + 1);
        nargs++;
    }

    for (char **elem = (char **)envp; *elem; elem++)
    {
        stack_ptr = (void *)stack_ptr - (strlen(*elem) + 1);
        nenv++;
    }

    stack_ptr = (void *)stack_ptr - ((uintptr_t)stack_ptr & 0xf);
    if ((nargs + nenv + 1) & 1)
        stack_ptr--;

    --stack_ptr;
    --stack_ptr;

    stack_ptr -= 2;
    stack_ptr -= 2;
    stack_ptr -= 2;
    stack_ptr -= 2;

    stack_ptr--;
    stack_ptr -= nenv;
    stack_ptr--;
    stack_ptr -= nargs;
    stack_ptr--;

    uintptr_t required_size = (stack_top - (uintptr_t)stack_ptr);

    size_t *stack = ichor_malloc(required_size);

    for (char **elem = (char **)envp; *elem; elem++)
    {
        stack = (void *)stack - (strlen(*elem) + 1);
        strncpy((char *)stack, *elem, strlen(*elem));
    }

    for (char **elem = (char **)argv; *elem; elem++)
    {
        stack = (void *)stack - (strlen(*elem) + 1);
        strncpy((char *)stack, *elem, strlen(*elem));
    }

    /* Align strp to 16-byte so that the following calculation becomes easier. */
    stack = (void *)stack - ((uintptr_t)stack & 0xf);

    /* Make sure the *final* stack pointer is 16-byte aligned.
            - The auxv takes a multiple of 16-bytes; ignore that.
            - There are 2 markers that each take 8-byte; ignore that, too.
            - Then, there is argc and (nargs + nenv)-many pointers to args/environ.
              Those are what we *really* care about. */
    if ((nargs + nenv + 1) & 1)
        stack--;

    // clang-format off
    *(--stack) = 0; *(--stack) = 0; /* Zero auxiliary vector entry */
    stack -= 2; *stack = AT_ENTRY; *(stack + 1) = auxval.at_entry;
    stack -= 2; *stack = AT_PHDR; *(stack + 1) = auxval.at_phdr;
    stack -= 2; *stack = AT_PHENT; *(stack + 1) = auxval.at_phent;
    stack -= 2; *stack = AT_PHNUM; *(stack + 1) = auxval.at_phnum;
    // clang-format on

    uintptr_t sa = USER_STACK_TOP;

    *(--stack) = 0; /* Marker for end of environ */
    stack -= nenv;
    for (size_t i = 0; i < nenv; i++)
    {
        sa -= strlen(envp[i]) + 1;
        stack[i] = sa;
    }

    *(--stack) = 0; /* Marker for end of argv */

    stack -= nargs;
    for (size_t i = 0; i < nargs; i++)
    {
        sa -= strlen(argv[i]) + 1;
        stack[i] = sa;
    }

    *(--stack) = nargs; /* argc */

    ichor_debug("Size: %p", required_size);

    VmObject stack_obj = sys_vm_create(8 * 1024 * 1024, 0, 0);

    sys_vm_map(new_task.space, &stack_obj, VM_PROT_READ | VM_PROT_WRITE, USER_STACK_TOP - 8 * 1024 * 1024, VM_MAP_FIXED);

    sys_vm_write(new_task.space, stack_top - required_size, stack, required_size);

    if (ld_path[0])
    {
        Vnode *ldn;
        Vattr ld_attr = {0};

        r = vfs_find_and(NULL, &ldn, ld_path, VFS_FIND_OR_ERROR, &ld_attr);

        if (r < 0)
            POSIX_PANIC("error finding ld path");

        Auxval ld_aux = {0};
        r = load_elf(&new_task, ldn, &ld_aux, 0x40000000, NULL);

        if (r != ERR_SUCCESS)
        {
            ichor_debug("error: %d", r);
            POSIX_PANIC("what");
        }
        ichor_debug("at_entry: %p", ld_aux.at_entry);
        sys_start_task_stack(&new_task, ld_aux.at_entry, stack_top - required_size, false);
    }
    return 0;
}