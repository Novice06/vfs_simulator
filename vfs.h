/*
 * MIT License
 *
 * Copyright (c) 2025 Novice
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#pragma once
#include <stdint.h>
#include <stddef.h>

#define VFS_MAX_PATH_LENGTH 256
#define VFS_MAX_FILENAME 64

typedef enum
{
    VFS_O_RDONLY = 0x0001,   // read only
    VFS_O_WRONLY = 0x0002,   // write only
    VFS_O_RDWR   = 0x0003,   // read / write
} vfs_open_mode_t;

typedef enum {
    VFS_OK         = 0,     /* Opération réussie */
    VFS_ERROR      = -1,    /* Erreur générique */
    VFS_ENOENT     = -2,    /* Fichier ou répertoire non trouvé */
    VFS_EEXIST     = -3,    /* Le fichier existe déjà */
    VFS_EACCESS    = -4,    /* Permission refusée */
    VFS_EISDIR     = -9,    /* Est un répertoire */
    VFS_ENOTDIR    = -10,   /* N'est pas un répertoire */
    VFS_ENFILE     = -11,   /* Trop de fichiers ouverts */
    VFS_EBADF      = -12    /* Descripteur de fichier invalide */
} vfs_error_t;

struct filesystem;
struct vnode;
struct vnodeops;

typedef struct vfs
{
    struct vfs *next;
    struct filesystem *vfs_op;
    struct vnode *vnodecovered;
    void *vfs_data;
}vfs_t;

typedef struct filesystem
{
    char fs_name[VFS_MAX_FILENAME];
    int (*vfs_mount)(struct vfs* mountpoint, int device_id);
    int (*vfs_unmount)(struct vfs* mountpoint);
    int (*get_root)(struct vfs* mountpoint, struct vnode** result);
}filesystem_t;

typedef struct vnode
{
    //uint32_t flags;
    uint32_t ref_count;
    struct vfs *vfs_mountedhere;
    struct vnodeops *vnode_op;
    struct vfs *vnode_vfs;
    void *vnode_data;
}vnode_t;

typedef struct vnodeops
{
    int (*read)(struct vnode* node, void *buffer, size_t size, uint32_t offset);
    int (*write)(struct vnode* node, const void *buffer, size_t size, uint32_t offset);
    int (*lookup)(struct vnode* node_dir, const char* name, struct vnode** result);
}vnodeops_t;

typedef struct vfs_file
{
    struct vnode *vnode;
    uint16_t mode;
    uint32_t position;
} vfs_file_t;

typedef int fd_t;   // file descriptor

void vfs_init();
void vfs_register_new_filesystem(filesystem_t* fs);

int vfs_mount(const char *fs_name, const char *mount_point, int device_id);
int vfs_unmount(const char *mount_point);

fd_t vfs_open(const char *path, uint16_t mode);
int vfs_close(fd_t descriptor);

size_t vfs_read(fd_t fd, void *buffer, size_t size);
size_t vfs_write(fd_t fd, const void *buffer, size_t size);