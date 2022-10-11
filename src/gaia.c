#include "gaia.h"

void gaia_log(const char *str)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(GAIA_SYS_LOG), "D"(str));
}

size_t gaia_msg(uint8_t type, GaiaPort port_to_receive, size_t bytes_to_receive, GaiaMessageHeader *msg)
{
    size_t ret;
    __asm__ volatile("int $0x42"
                     : "=a"(ret)
                     : "a"(GAIA_SYS_MSG), "D"(type), "S"(port_to_receive), "d"(bytes_to_receive), "rcx"(msg));
    return ret;
}

GaiaPort gaia_allocate_port(uint8_t rights)
{
    GaiaPort ret;
    __asm__ volatile("int $0x42"
                     : "=a"(ret)
                     : "a"(GAIA_SYS_ALLOC_PORT), "D"(rights));
    return ret;
}

void gaia_register_common_port(uint8_t index, GaiaPort port)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(GAIA_SYS_REGISTER_PORT), "D"(index), "S"(port));
}

GaiaPort gaia_get_common_port(uint8_t index)
{
    GaiaPort ret;
    __asm__ volatile("int $0x42"
                     : "=a"(ret)
                     : "a"(GAIA_SYS_GET_PORT), "D"(index));

    return ret;
}

void gaia_exit(int status)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(GAIA_SYS_EXIT), "D"(status));
}

void gaia_create_task(GaiaTask *out)
{
    __asm__ volatile("int $0x42"
                     : "=a"(out)
                     : "a"(GAIA_SYS_CREATE_TASK));
}

void gaia_start_task(GaiaTask *task, uintptr_t entry_point, uintptr_t stack_pointer, bool allocate_stack)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(GAIA_SYS_START_TASK), "D"(task), "S"(entry_point), "d"(stack_pointer), "rcx"(allocate_stack));
}

struct __attribute__((packed)) mmap_params
{
    void *space;
    uint16_t prot;
    uint16_t flags;
    void *virt, *phys;
    size_t size;
};

void *gaia_vm_map(void *space, uint16_t prot, uint16_t flags, void *addr, void *phys, size_t size)
{
    struct mmap_params params = {space, prot, flags, addr, phys, size};
    void *ret;

    __asm__ volatile("int $0x42"
                     : "=a"(ret)
                     : "a"(GAIA_SYS_VM_MAP), "D"(&params));
    return ret;
}

void gaia_vm_write(void *space, uintptr_t address, void *buffer, size_t count)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(GAIA_SYS_VM_WRITE), "D"(space), "S"(address), "d"(buffer), "rcx"(count));
}
