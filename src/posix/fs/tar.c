#include "posix/errno.h"
#include "posix/posix.h"
#include <fs/tar.h>
#include <fs/vfs.h>
#include <ichor/base.h>
#include <ichor/debug.h>
#include <ichor/error.h>
#include <posix/stat.h>
#include <stdc-shim/string.h>
#include <stdint.h>

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

void tar_write_on_tmpfs(void *archive)
{
    TarHeader *current_file = (TarHeader *)archive;

    while (strncmp(current_file->magic, "ustar", 5) == 0)
    {
        char *name = current_file->name;
        Vattr attr = {0};

        attr.mode = oct2int(current_file->mode, sizeof(current_file->mode) - 1) & ~(S_IFMT);

        uint64_t size = oct2int(current_file->size, sizeof(current_file->size));

        switch (current_file->type)
        {
        case TAR_NORMAL_FILE:
        {
            int ret = 0;
            Vnode *vn;

            ret = vfs_find_and(NULL, &vn, name, VFS_FIND_AND_CREATE, &attr);

            if (ret < 0)
            {
                ichor_debug("Failed making file %s, error is %s", name, ret == -ENOTDIR ? "enodir" : "dunno");
            }

            vfs_write(vn, (void *)current_file + 512, size, 0);
            break;
        }

        case TAR_DIRECTORY:
        {
            int ret = 0;
            Vnode *vn;

            ret = vfs_find_and(NULL, &vn, name, VFS_FIND_AND_MKDIR, &attr);

            if (ret < 0)
            {
                ichor_debug("Failed making directory %s", name);
            }
            break;
        }
        }

        current_file = (void *)current_file + 512 + ALIGN_UP(size, 512);
    }
}