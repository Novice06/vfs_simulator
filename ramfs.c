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

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "device.h"
#include "vfs.h"

#include "ramfs.h"

uint64_t get_current_time() {
    return (uint64_t)time(NULL);
}

treenode_t* ramfs_lookup(treenode_t *dir, const char *name)
{
    if (!dir || dir->meta.type != NODE_DIRECTORY)
        return NULL;
    
    // Mise à jour du temps d'accès
    dir->meta.access_time = get_current_time();
    
    treenode_t *child = dir->first_child;
    while (child)
    {
        if (strcmp(child->meta.name, name) == 0)
            return child;
        
        child = child->next_sibling;
    }
    
    return NULL;
}

treenode_t* ramfs_create_node(treenode_t *parent, const char *name, nodetype_t type)
{
    if (!parent || parent->meta.type != NODE_DIRECTORY)
        return NULL;
    
    // Vérifier si le nom existe déjà
    if (ramfs_lookup(parent, name) != NULL)
        return NULL; // Le nom existe déjà
    
    // Créer le nouveau nœud
    treenode_t *new_node = (treenode_t*)malloc(sizeof(treenode_t));
    if (!new_node)
        return NULL;
    
    // Initialiser les métadonnées
    strncpy(new_node->meta.name, name, MAX_NAME_LENGTH - 1);
    new_node->meta.name[MAX_NAME_LENGTH - 1] = '\0';
    new_node->meta.type = type;
    new_node->meta.size = 0;
    uint64_t current_time = get_current_time();
    new_node->meta.create_time = current_time;
    new_node->meta.modify_time = current_time;
    new_node->meta.access_time = current_time;
    
    // Initialiser les liens
    new_node->parent = parent;
    new_node->first_child = NULL;
    new_node->next_sibling = NULL;
    new_node->data = NULL;
    
    // Ajouter à la liste des enfants du parent
    if (!parent->first_child)
        parent->first_child = new_node;
    else
    {
        treenode_t *sibling = parent->first_child;
        while (sibling->next_sibling)
            sibling = sibling->next_sibling;

        sibling->next_sibling = new_node;
    }
    
    // Mettre à jour le temps de modification du parent
    parent->meta.modify_time = current_time;
    
    return new_node;
}

size_t ramfs_write(treenode_t *file, const uint8_t *data, uint64_t size, uint64_t offset)
{
    if (!file || file->meta.type != NODE_FILE)
        return VFS_ENOENT;
    
    // Calculer la nouvelle taille du fichier
    uint64_t new_size = (offset + size > file->meta.size) ? offset + size : file->meta.size;
    
    // Réallouer la mémoire pour le fichier si nécessaire
    if (new_size > file->meta.size || !file->data) 
    {
        uint8_t *new_data = (uint8_t*)realloc(file->data, new_size);
        if (!new_data)
            return VFS_ERROR;
        
        file->data = new_data;
    }
    
    // Copier les données
    if (data && size > 0)
        memcpy(file->data + offset, data, size);
    
    // Mettre à jour la taille du fichier
    file->meta.size = new_size;
    
    // Mettre à jour les temps
    uint64_t current_time = get_current_time();
    file->meta.modify_time = current_time;
    file->meta.access_time = current_time;
    
    return size;
}

// Fonction pour lire un fichier
size_t ramfs_read(treenode_t *file, uint8_t *buffer, uint64_t size, uint64_t offset)
{
    if (!file || file->meta.type != NODE_FILE || !buffer)
        return VFS_ENOENT;
    
    // Mettre à jour le temps d'accès
    file->meta.access_time = get_current_time();
    
    // Vérifier les limites
    if (offset >= file->meta.size)
        return 0;
    
    // Calculer le nombre d'octets à lire
    uint64_t to_read = (offset + size > file->meta.size) ? file->meta.size - offset : size;
    
    // Copier les données
    memcpy(buffer, file->data + offset, to_read);

    return to_read;
}

#define MAX_VNODE   16
typedef struct ramfs_info
{
    vnode_t* total_vnode[MAX_VNODE];
    vnode_t* root_vnode;
    treenode_t* root_node;
}fs_info_t;

int ramfs_mount(vfs_t* mountpoint, int device_id);
int ramfs_unmount(vfs_t* mountpoint);
int ramfs_get_root(vfs_t* mountpoint, vnode_t** result);

int read(vnode_t* node, void *buffer, size_t size, uint32_t offset);
int write(vnode_t* node, const void *buffer, size_t size, uint32_t offset);
int lookup(vnode_t* node, const char* name, struct vnode** result);

filesystem_t ramfs_op = {
    .get_root = ramfs_get_root,
    .vfs_mount = ramfs_mount,
    .vfs_unmount = ramfs_unmount,
};

vnodeops_t ramfs_vnode_op = {
    .read = read,
    .write = write,
    .lookup = lookup,
};

void ramfs_init()
{
    strcpy(ramfs_op.fs_name, "ramfs");
    vfs_register_new_filesystem(&ramfs_op);

    treenode_t* root0_fs = malloc(sizeof(treenode_t));
    root0_fs->meta.type = NODE_DIRECTORY;
    root0_fs->parent = NULL;
    root0_fs->first_child = NULL;
    root0_fs->next_sibling = NULL;

    treenode_t* doc = ramfs_create_node(root0_fs, "doc", NODE_DIRECTORY);
    treenode_t* hello = ramfs_create_node(doc, "hello.txt", NODE_FILE);

    ramfs_write(hello, (const uint8_t*)"hello world !", 13, 0);
    ramfs_create_node(root0_fs, "mnt", NODE_DIRECTORY);

    device_t* device_1 = malloc(sizeof(device_t));
    strcpy(device_1->name, "root0_fs");
    device_1->priv = root0_fs;
    device_1->read = NULL;
    device_1->write = NULL;
    add_device(device_1);

    treenode_t* root1_fs = malloc(sizeof(treenode_t));
    root1_fs->meta.type = NODE_DIRECTORY;
    root1_fs->parent = NULL;
    root1_fs->first_child = NULL;
    root1_fs->next_sibling = NULL;

    treenode_t* hi = ramfs_create_node(root1_fs, "hi.txt", NODE_FILE);
    ramfs_write(hi, (const uint8_t*)"hi from root1_fs !", 18, 0);

    device_t* device_2 = malloc(sizeof(device_t));
    strcpy(device_2->name, "root1_fs");
    device_2->priv = root1_fs;
    device_2->read = NULL;
    device_2->write = NULL;
    add_device(device_2);
}

int ramfs_mount(vfs_t* mountpoint, int device_id)
{
    fs_info_t* fs_info = malloc(sizeof(fs_info_t));

    for(int i = 0; i < MAX_VNODE; i++)
        fs_info->total_vnode[i] = NULL;

    fs_info->root_vnode = malloc(sizeof(vnode_t));
    fs_info->root_vnode->ref_count = 0;
    fs_info->root_vnode->vfs_mountedhere = NULL;
    fs_info->root_vnode->vnode_op = &ramfs_vnode_op;
    fs_info->root_vnode->vnode_vfs = mountpoint;

    fs_info->root_node = (treenode_t*)device_list[device_id]->priv;
    fs_info->root_vnode->vnode_data = fs_info->root_node;

    // here we need to fill specific filesystem info !
    mountpoint->vfs_data = fs_info;

    return VFS_OK;
}

int ramfs_unmount(vfs_t* mountpoint)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    for(int i = 0; i < MAX_VNODE; i++)
        if(fs_info->total_vnode[i] != NULL)
            free(fs_info->total_vnode[i]);

    free(fs_info->root_vnode);

    free(fs_info);

    return VFS_OK;
}

int ramfs_get_root(vfs_t* mountpoint, vnode_t** result)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    *result = fs_info->root_vnode;
    
    return VFS_OK;
}

static vnode_t* create_vnode(vfs_t* mountpoint, treenode_t* node)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    for(int i = 0; i < MAX_VNODE; i++)
        if(fs_info->total_vnode[i] != NULL && fs_info->total_vnode[i]->vnode_data == (void*)node)
            return fs_info->total_vnode[i];    // if the vnode already exist in the vnode table

    vnode_t* newVnode = malloc(sizeof(vnode_t));
    newVnode->ref_count = 0;
    newVnode->vfs_mountedhere = NULL;
    newVnode->vnode_data = node;
    newVnode->vnode_op = &ramfs_vnode_op;
    newVnode->vnode_vfs = mountpoint;
   
    // otherwise we'll search a free place
    for(int i = 0; i < MAX_VNODE; i++)
    {   
        if(fs_info->total_vnode[i] == NULL)
        {
            fs_info->total_vnode[i] = newVnode;
            return newVnode;
        } 

        if(fs_info->total_vnode[i]->ref_count <= 0)
        {
            free(fs_info->total_vnode[i]);
            fs_info->total_vnode[i] = newVnode;
            return newVnode;
        }
    }

    free(newVnode);
    return NULL;
}

int lookup(vnode_t* node_dir, const char* name, struct vnode** result)
{
    treenode_t* node = NULL;
    treenode_t* parent = (treenode_t*)node_dir->vnode_data;

    node = ramfs_lookup(parent, name);
    if(node == NULL)
        return VFS_ENOENT;

    *result = create_vnode(node_dir->vnode_vfs, node);

    if(*result == NULL)
        return VFS_ERROR;
    else
        return VFS_OK;
}

int read(vnode_t* node, void *buffer, size_t size, uint32_t offset)
{
    treenode_t* file_node = (treenode_t*)node->vnode_data;

    return ramfs_read(file_node, buffer, size, offset);
}

int write(vnode_t* node, const void *buffer, size_t size, uint32_t offset)
{
    treenode_t* file_node = (treenode_t*)node->vnode_data;

    return ramfs_write(file_node, buffer, size, offset);
}