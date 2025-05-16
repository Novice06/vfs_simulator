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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vfs.h"

#define VFS_MAX_FS 10
#define MAX_OPEN_FILES 24

vfs_t *vfs_root;
filesystem_t *registered_fs[VFS_MAX_FS];
int num_registered_fs;
vfs_file_t vfs_open_files[MAX_OPEN_FILES];

static void add_mount_point(vfs_t *mountpoint)
{
	if(vfs_root == NULL)
	{
		vfs_root = mountpoint;
		return;
	}

	vfs_t *current = vfs_root;
	while (current->next != NULL)
		current = current->next;

	current->next = mountpoint;
}

static void remove_mount_point(vfs_t *mountpoint)
{
	vfs_t *current = vfs_root;

	while (current->next != mountpoint)
		current = current->next;

	current->next = mountpoint;
}

 static filesystem_t *find_filesystem_by_name(const char *name)
 {
	for (int i = 0; i < num_registered_fs; i++)
	{
		if (strcmp(registered_fs[i]->fs_name, name) == 0)
		{
			return registered_fs[i];
		}
	}
	return NULL;
 }

static fd_t find_free_fd(void)
{
	for (fd_t i = 0; i < MAX_OPEN_FILES; i++)
	{
		if (vfs_open_files[i].vnode == NULL)
			return i;
	}
	return VFS_ENFILE;
}

static int is_fd_valid(fd_t fd)
{
	if(fd < 0 || fd >= MAX_OPEN_FILES)
		return 0;

	if (vfs_open_files[fd].vnode == NULL)
		return 0;

	return 1;
}

void vfs_init()
{
	vfs_root = NULL;
	num_registered_fs = 0;

	for(int i = 0; i < VFS_MAX_FS; i++)
		registered_fs[i] = NULL;

	for(int i = 0; i < MAX_OPEN_FILES; i++)
		vfs_open_files[i].vnode = NULL;
}

vnode_t* lookup_path_name(const char* path)
{
	/*vnode_t* root_node = NULL;
	vnode_t* node_out = NULL;
	char newPath[VFS_MAX_PATH_LENGTH];

	if(path == NULL)
		return NULL;

	vfs_t* mountpoint = find_best_mount_point(path, newPath);

	mountpoint->vfs_op->get_root(mountpoint, &root_node);

	root_node->vnode_op->lookup(root_node, newPath, &node_out);
	// TODO: CHECK FOR ERROR !!

	return node_out;*/

	vnode_t* node_out = NULL;
	char parsed_path[VFS_MAX_PATH_LENGTH];
	char* name;

	if(path == NULL || path[0] != '/')
		return NULL;

	strcpy(parsed_path, path);

	vfs_root->vfs_op->get_root(vfs_root, &node_out);
	name = strtok(parsed_path, "/");
	
	while(node_out != NULL && name != NULL)
	{
		if(node_out->vfs_mountedhere != NULL) // if this is a mountpoint
			node_out->vfs_mountedhere->vfs_op->get_root(node_out->vfs_mountedhere, &node_out);

		node_out->vnode_op->lookup(node_out, name, &node_out);
		name = strtok(NULL, "/");
	}

	//if(name != NULL)	// this means the path has not been enterely parsed, this code has been commented out
	//	return NULL;	// because it's useless the lookup vnode operation returns null as result
	
	return node_out;
}

int vfs_mount(const char *fs_name, const char *mount_point, int device_id)
{
	vfs_t *new_vfs;
	filesystem_t *fs;

	fs = find_filesystem_by_name(fs_name);

	if(fs == NULL)
		return VFS_ERROR; // error code !

	new_vfs = malloc(sizeof(vfs_t));

	new_vfs->next = NULL;
	new_vfs->vfs_op = fs;

	if(vfs_root == NULL)	// is this the first mount point ?
		new_vfs->vnodecovered = NULL;
	else
	{
		new_vfs->vnodecovered = lookup_path_name(mount_point);
		if(new_vfs->vnodecovered == NULL)
		{
			free(new_vfs);
			return VFS_ENOENT;
		}

		new_vfs->vnodecovered->ref_count++;
		new_vfs->vnodecovered->vfs_mountedhere = new_vfs;
	}

	new_vfs->vfs_op->vfs_mount(new_vfs, device_id);
	add_mount_point(new_vfs);

	return VFS_OK;	// ok
}

 int vfs_unmount(const char *mount_point)
 {
	//vfs_t* mountpoint = find_mount_point_by_path(mount_point);

	vnode_t* vnode_covered = lookup_path_name(mount_point);
	if(vnode_covered == NULL)
		return VFS_ENOENT;

	vfs_t* mountpoint = vnode_covered->vfs_mountedhere;

	if(mountpoint == NULL)
		return VFS_ERROR;	// it's not a mount point...

	if (mountpoint == vfs_root)
         return VFS_EACCESS;  // cannot unmount the root fs

	// TODO: implemente a mechanism to prevent umounting a filesystem
	// as long as there are other filesystems mounted on top of it

	mountpoint->vfs_op->vfs_unmount(mountpoint);
	remove_mount_point(mountpoint);
	free(mountpoint);

    return VFS_OK;
 }

 fd_t vfs_open(const char *path, uint16_t mode)
 {
	vnode_t* file_node = lookup_path_name(path);

	if(file_node == NULL)
		return VFS_ENOENT;

	fd_t descriptor = find_free_fd();
	if(descriptor == VFS_ENFILE)
		return VFS_ENFILE;

	file_node->ref_count++;

	vfs_open_files[descriptor].mode = mode;
	vfs_open_files[descriptor].position = 0;
	vfs_open_files[descriptor].vnode = file_node;

	return descriptor;
 }

int vfs_close(fd_t descriptor)
{
	if(!is_fd_valid(descriptor))
		return VFS_EBADF;

	vfs_open_files[descriptor].vnode->ref_count--;
	vfs_open_files[descriptor].vnode = NULL;

    return VFS_OK;
}

size_t vfs_read(fd_t fd, void *buffer, size_t size)
{
	if(!is_fd_valid(fd))
		return VFS_EBADF;

	if(vfs_open_files[fd].mode != VFS_O_RDONLY && vfs_open_files[fd].mode != VFS_O_RDWR)
		return VFS_EACCESS;

	return vfs_open_files[fd].vnode->vnode_op->read(vfs_open_files[fd].vnode, buffer, size, vfs_open_files[fd].position);
}

size_t vfs_write(fd_t fd, const void *buffer, size_t size)
{
	if(!is_fd_valid(fd))
		return VFS_EBADF;

	if(vfs_open_files[fd].mode != VFS_O_WRONLY && vfs_open_files[fd].mode != VFS_O_RDWR)
		return VFS_EACCESS;

	return vfs_open_files[fd].vnode->vnode_op->write(vfs_open_files[fd].vnode, buffer, size, vfs_open_files[fd].position);
}

void vfs_register_new_filesystem(filesystem_t* fs)
{
	if(num_registered_fs >= VFS_MAX_FS)
		return;

	registered_fs[num_registered_fs] = fs;
	num_registered_fs++;
}