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

typedef enum
{
    VFS_OK         = 0,     /* Operation successful */
    VFS_ERROR      = -1,    /* Generic error */
    VFS_ENOENT     = -2,    /* File or directory not found */
    VFS_EEXIST     = -3,    /* File already exists */
    VFS_EACCESS    = -4,    /* Permission denied */
    VFS_EISDIR     = -9,    /* Is a directory */
    VFS_ENOTDIR    = -10,   /* Not a directory */
    VFS_ENFILE     = -11,   /* Too many open files */
    VFS_EBADF      = -12    /* Invalid file descriptor */
} vfs_error_t;


struct filesystem;
struct vnode;
struct vnodeops;

/*
 * Represents a mounted virtual file system.
 * This structure links together the mount point and the file system operations.
 */
typedef struct vfs
{
    struct vfs *next;           /* Pointer to the next mounted file system (used in a linked list) */
    struct filesystem *vfs_op;  /* Pointer to the file system operations (driver) associated with this mount */
    struct vnode *vnodecovered; /* The vnode that this file system is mounted over (i.e., the mount point) */
    void *vfs_data;             /* Private data used by the specific file system implementation */
} vfs_t;


/*
 * Describes a file system driver and the operations it supports.
 * Each registered file system should implement these functions.
 */
typedef struct filesystem
{
    char fs_name[VFS_MAX_FILENAME];                                 /* Name of the file system (e.g., "ramfs", "ext2") */
    int (*vfs_mount)(struct vfs* mountpoint, int device_id);        /* Function to mount the file system on a device */
    int (*vfs_unmount)(struct vfs* mountpoint);                     /* Function to unmount the file system */
    int (*get_root)(struct vfs* mountpoint, struct vnode** result); /* Get the root vnode of the mounted FS */
}filesystem_t;


typedef enum {VNON, VREG, VDIR}vtype;
/*
 * Represents a file or directory within a mounted file system.
 * Abstracts the underlying inode and links to file system operations.
 */
typedef struct vnode
{
    //uint32_t flags;
    uint32_t ref_count;             /* Reference count for this vnode (used to manage lifetime) */
    vtype vnode_type;
    struct vfs *vfs_mountedhere;    /* If another file system is mounted here, pointer to it */
    struct vnodeops *vnode_op;      /* Pointer to the vnode operations supported by this file/directory */
    struct vfs *vnode_vfs;          /* The VFS this vnode belongs to */
    void *vnode_data;               /* File-system-specific data (usually an inode or similar) */
}vnode_t;

/*
 * Defines the operations that can be performed on a vnode.
 * These must be implemented by each file system.
 */
typedef struct vnodeops
{
    int (*read)(struct vnode* node, void *buffer, size_t size, uint32_t offset);
    int (*write)(struct vnode* node, const void *buffer, size_t size, uint32_t offset);

    /* Find a file/directory by name */
    int (*lookup)(struct vnode* node_dir, const char* name, struct vnode** result);
}vnodeops_t;


/*
 * Represents an open file in the VFS layer.
 * Maintains current file position and access mode.
 */
typedef struct vfs_file
{
    struct vnode *vnode;    /* The vnode associated with this file */
    uint16_t mode;          /* Mode in which the file was opened (read, write, etc.) */
    uint32_t position;      /* Current position within the file (for reading/writing) */
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