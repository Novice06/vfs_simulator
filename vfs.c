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

static int count_slash_path(const char* path)
{
	char* slash;
	int num = 0;

	slash = strchr(path, '/');
	while(slash != NULL)
	{
		num++;
		slash = strchr(slash+1, '/');
	}

	return num;
}

static void add_mount_point(vfs_t *mountpoint)
{
	int mountpoint_length = count_slash_path(mountpoint->mount_point);

	if(vfs_root == NULL)
	{
		vfs_root = mountpoint;
		return;
	}

	vfs_t *current = vfs_root;

	while (current->next != NULL)
	{
		int next_length = count_slash_path(current->next->mount_point);

		if(next_length > mountpoint_length)
		{
			mountpoint->next = current->next;
			current->next = mountpoint;
			return;
		}

		current = current->next;
	}
	
	current->next = mountpoint;
}

static void remove_mount_point(vfs_t *mountpoint)
{
	vfs_t *current = vfs_root;

	while (current->next != mountpoint)
		current = current->next;

	current->next = mountpoint;
}

static vfs_t* find_mount_point_by_path(const char *path)
{
	vfs_t *current = vfs_root;

    while (current != NULL)
	{
		if(strcmp(current->mount_point, path) == 0)
			return current;
		
		current = current->next;
	}

	return NULL;
 }
 

static vfs_t* find_best_mount_point(const char *path, char* relativePathOut)
{
	if(path == NULL)
		return NULL;

	vfs_t* best_mountpoint = vfs_root;
	vfs_t* current = best_mountpoint->next;

	while (current != NULL)
	{
		if(strncmp(current->mount_point, path, strlen(current->mount_point)) == 0)
			best_mountpoint = current;

		current = current->next;
	}

	if(relativePathOut != NULL)
		strcpy(relativePathOut, &path[strlen(best_mountpoint->mount_point)]);
	
	return best_mountpoint;
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
	vnode_t* root_node = NULL;
	vnode_t* node_out = NULL;
	char newPath[VFS_MAX_PATH_LENGTH];

	if(path == NULL)
		return NULL;

	vfs_t* mountpoint = find_best_mount_point(path, newPath);

	mountpoint->vfs_op->get_root(mountpoint, &root_node);

	root_node->vnode_op->lookup(root_node, newPath, &node_out);
	// TODO: CHECK FOR ERROR !!

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
	strncpy(new_vfs->mount_point, mount_point, VFS_MAX_PATH_LENGTH);
	new_vfs->vfs_op = fs;

	if(vfs_root == NULL)	// is this the first mount point ?
	{
		new_vfs->vnodecovered = NULL;
		strcpy(new_vfs->mount_point, "/");
	}
	else
	{
		new_vfs->vnodecovered = lookup_path_name(mount_point);
		// TODO: CHECK FOR ERROR !!
	}

	new_vfs->vfs_op->vfs_mount(new_vfs, device_id);
	add_mount_point(new_vfs);

	return VFS_OK;	// ok
}

 int vfs_unmount(const char *mount_point)
 {
	vfs_t* mountpoint = find_mount_point_by_path(mount_point);

	if(mountpoint == NULL)
		return VFS_ERROR;	// it's not a mount point...

	if (mountpoint == vfs_root)
         return VFS_EACCESS;  // cannot unmount the root fs

	mountpoint->vfs_op->vfs_unmount(mountpoint);
	remove_mount_point(mountpoint);
	free(mountpoint);

    return VFS_OK;
 }

 fd_t vfs_open(const char *path, uint16_t mode)
 {
	vnode_t* root_node = NULL;
	vnode_t* file_node = NULL;
	char newPath[VFS_MAX_PATH_LENGTH];

	vfs_t* mountpoint = find_best_mount_point(path, newPath);
	if(mountpoint == NULL)
		return VFS_ERROR;

	mountpoint->vfs_op->get_root(mountpoint, &root_node);

	root_node->vnode_op->lookup(root_node, newPath, &file_node);
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