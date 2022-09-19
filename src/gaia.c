#include "gaia.h"

void gaia_log(const char *str)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(GAIA_SYS_LOG), "D"(str));
}

void gaia_msg(uint8_t type, GaiaPort port_to_receive, size_t bytes_to_receive, GaiaMessageHeader *msg)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(GAIA_SYS_MSG), "D"(type), "S"(port_to_receive), "d"(bytes_to_receive), "rcx"(msg));
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

void gaia_spawn_module(const char *name)
{
    __asm__ volatile("int $0x42"
                     :
                     : "a"(GAIA_SYS_SPAWN), "D"(name));
}
