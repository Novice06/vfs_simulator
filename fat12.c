#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "vfs.h"
#include "device.h"

#define MAX_VNODE_PER_VFS   16

typedef struct fat_BS
{
    uint8_t bootjmp[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t table_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t table_size_16;
    uint16_t sectors_per_track;
    uint16_t head_side_count;
    uint32_t hidden_sector_count;
    uint32_t total_sectors_32;

    //extended fat12 and fat16 stuff
    uint8_t bios_drive_num;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fat_type_label[8];

    uint8_t Filler[448];            //needed to make struct 512 bytes

}__attribute__((packed)) fat_BS_t;

typedef enum
{
    FAT_ATTR_REGULAR    = 0x00, // Regular file with no special attribute
    FAT_ATTR_READ_ONLY  = 0x01, // File is read-only
    FAT_ATTR_HIDDEN     = 0x02, // File is hidden
    FAT_ATTR_SYSTEM     = 0x04, // System file
    FAT_ATTR_VOLUME_ID  = 0x08, // Volume label
    FAT_ATTR_DIRECTORY  = 0x10, // Entry is a directory
    FAT_ATTR_ARCHIVE    = 0x20, // File should be archived
    FAT_ATTR_LFN        = 0x0F  // Long file name entry (special combination)
} fat_attributes_t;

typedef struct fat_dir_entry
{
    char filename[11];            // File name (8 characters) and File extension (3 characters)
    uint8_t attributes;          // File attributes (read-only, hidden, system, etc.)
    uint8_t reserved;            // Reserved (usually zero)
    uint8_t creationTimeTenth;   // Creation time in tenths of a second
    uint16_t creationTime;       // Creation time (HHMMSS format)
    uint16_t creationDate;       // Creation date (YYMMDD format)
    uint16_t lastAccessDate;     // Last access date
    uint16_t firstClusterHigh;   // High word of the first cluster number (FAT32 only)
    uint16_t writeTime;          // Last write time
    uint16_t writeDate;          // Last write date
    uint16_t firstClusterLow;    // Low word of the first cluster number
    uint32_t fileSize;           // File size in bytes
} __attribute__((packed)) fat_dir_entry_t;

/* Important information for the file system ! */
typedef struct ramfs_info
{
    vnode_t* total_vnode[MAX_VNODE_PER_VFS];
    vnode_t* root_vnode;
    fat_BS_t *bootSector;
    void* file_allocation_table;
    void* fat_buffer;
}fs_info_t;

int fat12_mount(vfs_t* mountpoint, int device_id);
int fat12_unmount(vfs_t* mountpoint);
int fat12_get_root(vfs_t* mountpoint, vnode_t** result);

int fat12_read(vnode_t* node, void *buffer, size_t size, uint32_t offset);
int fat12_write(vnode_t* node, const void *buffer, size_t size, uint32_t offset);
int fat12_lookup(vnode_t* node, const char* name, struct vnode** result);

filesystem_t fat12_op = {
    // fs_name will be filled later
    .get_root = fat12_get_root,
    .vfs_mount = fat12_mount,
    .vfs_unmount = fat12_unmount,
};

// vnode operation !!
vnodeops_t fat12_vnode_op = {
    .read = fat12_read,
    .write = fat12_write,
    .lookup = fat12_lookup,
};

void fat12_init()
{
    strcpy(fat12_op.fs_name, "fat12");
    vfs_register_new_filesystem(&fat12_op);
}

int fat12_mount(vfs_t* mountpoint, int device_id)
{
    fs_info_t* fs_info = malloc(sizeof(fs_info_t));

    if(fs_info == NULL)
        return -1; // error

    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
        fs_info->total_vnode[i] = NULL;
    
    fat_BS_t *bootSector = malloc(sizeof(fat_BS_t));
    if(bootSector == NULL)
        return -1; // error
    
    device_list[device_id]->read((void*)bootSector, 0, 1, device_list[device_id]->priv);

    void* file_allocation_table = malloc(sizeof(uint8_t) * bootSector->table_size_16 * bootSector->bytes_per_sector);
    if(file_allocation_table == NULL)
        return -1; // error
    
    device_list[device_id]->read(file_allocation_table, bootSector->reserved_sector_count, bootSector->table_size_16, device_list[device_id]->priv);

    void *fat_buffer = malloc(bootSector->sectors_per_cluster * bootSector->bytes_per_sector);
    if(fat_buffer == NULL)
        return -1; // error

    fs_info->bootSector = bootSector;
    fs_info->fat_buffer = fat_buffer;
    fs_info->file_allocation_table = file_allocation_table;

    fs_info->root_vnode = malloc(sizeof(vnode_t));
    if(fs_info->root_vnode == NULL)
        return -1; // error

    fs_info->root_vnode->ref_count = 0;
    fs_info->root_vnode->flags = VNODE_ROOT;
    fs_info->root_vnode->vnode_type = VDIR;
    fs_info->root_vnode->vfs_mountedhere = NULL;
    fs_info->root_vnode->vnode_op = &fat12_vnode_op;
    fs_info->root_vnode->vnode_vfs = mountpoint;
    fs_info->root_vnode->vnode_data = NULL; // its the root !!

    // here we need to fill specific filesystem info !
    mountpoint->vfs_data = fs_info;

    return VFS_OK;
}

int fat12_unmount(vfs_t* mountpoint)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
        if(fs_info->total_vnode[i] != NULL)
            free(fs_info->total_vnode[i]);
    
    free(fs_info->root_vnode);
    free(fs_info->bootSector);
    free(fs_info->fat_buffer);
    free(fs_info->file_allocation_table);
    
    return VFS_OK;
}

int fat12_get_root(vfs_t* mountpoint, vnode_t** result)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    *result = fs_info->root_vnode;
    
    return VFS_OK;
}

uint32_t get_next_cluster(uint32_t currentCluster, void* fat_table)
{    
    uint32_t fatIndex = currentCluster * 3 / 2;

    if (currentCluster % 2 == 0)
        return (*(uint16_t*)(fat_table + fatIndex)) & 0x0FFF;
    else
        return (*(uint16_t*)(fat_table + fatIndex)) >> 4;
}

uint32_t cluster_to_Lba(uint32_t cluster, fat_BS_t* bootSector)
{
    uint16_t root_dir_size = (bootSector->root_entry_count * 32) / bootSector->bytes_per_sector;
    uint16_t fat_total_size = (bootSector->table_size_16 * bootSector->table_count);

    return (bootSector->reserved_sector_count + fat_total_size + root_dir_size) + (cluster - 2) * bootSector->sectors_per_cluster;
}

int fat12_read(vnode_t* node, void *buffer, size_t size, uint32_t offset)
{
    if(node->vnode_type != VREG)
        return VFS_EISDIR;
    
    fat_dir_entry_t* inode = node->vnode_data;
    fs_info_t* fs_info = node->vnode_vfs->vfs_data;

    // EOF ?
    if (offset >= inode->fileSize)
        return 0;

    uint16_t currentCluster = inode->firstClusterLow;
    uint16_t skippedClusters = (uint16_t)(offset / (fs_info->bootSector->sectors_per_cluster * fs_info->bootSector->bytes_per_sector));
    
    for(int i = 0; i < skippedClusters; i++)
        currentCluster = get_next_cluster(currentCluster, fs_info->file_allocation_table);

    uint32_t hypothetical_offset = offset - (skippedClusters * fs_info->bootSector->sectors_per_cluster * fs_info->bootSector->bytes_per_sector);
    size_t to_read = 0;
    while (currentCluster < 0xFF8 && to_read < size)
    {
        device_list[node->vnode_vfs->device_id]->read(fs_info->fat_buffer, cluster_to_Lba(currentCluster, fs_info->bootSector), fs_info->bootSector->sectors_per_cluster, device_list[node->vnode_vfs->device_id]->priv);

        uint16_t byte_to_read = (fs_info->bootSector->sectors_per_cluster * fs_info->bootSector->bytes_per_sector) - hypothetical_offset;
        byte_to_read = ((byte_to_read + to_read) > size) ? (size - to_read) : byte_to_read;

        memcpy(buffer + to_read, fs_info->fat_buffer + hypothetical_offset, byte_to_read);

        to_read += byte_to_read;
        hypothetical_offset = 0;
        currentCluster = get_next_cluster(currentCluster, fs_info->file_allocation_table);
    }
    
    return to_read;
}

int fat12_write(vnode_t* node, const void *buffer, size_t size, uint32_t offset)
{
    /* I hate warnings !*/
    node = node;
    buffer = buffer;
    size = size;
    offset = offset;
    return 0;   // unfortunately I haven't implemented this yet !
}

static vnode_t* create_vnode(vfs_t* mountpoint, fat_dir_entry_t* inode_info)
{
    fs_info_t* fs_info = (fs_info_t*)mountpoint->vfs_data;

    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
    {
        if(fs_info->total_vnode[i] != NULL)
        {
            fat_dir_entry_t* existing_inode = (fat_dir_entry_t*)fs_info->total_vnode[i]->vnode_data;

            if(strncmp(existing_inode->filename, inode_info->filename, 11) == 0)
                return fs_info->total_vnode[i];    // if the vnode already exist in the vnode table
        }
    }

    fat_dir_entry_t* file_inode = malloc(sizeof(fat_dir_entry_t));
    memcpy(file_inode, inode_info, sizeof(fat_dir_entry_t));

    vnode_t* newVnode = malloc(sizeof(vnode_t));
    newVnode->ref_count = 0;
    newVnode->vfs_mountedhere = NULL;
    newVnode->vnode_data = file_inode;    // we store the inode here !!
    newVnode->vnode_op = &fat12_vnode_op;
    newVnode->vnode_vfs = mountpoint;

    if((file_inode->attributes & FAT_ATTR_DIRECTORY) == FAT_ATTR_DIRECTORY)
        newVnode->vnode_type = VDIR;
    else
        newVnode->vnode_type = VREG;

    // otherwise we'll search a free place
    for(int i = 0; i < MAX_VNODE_PER_VFS; i++)
    {   
        if(fs_info->total_vnode[i] == NULL)
        {
            fs_info->total_vnode[i] = newVnode;
            return newVnode;
        } 

        // if the vnode is unused
        if(fs_info->total_vnode[i]->ref_count <= 0)
        {
            free(fs_info->total_vnode[i]->vnode_data);  // free the inode !
            free(fs_info->total_vnode[i]);

            fs_info->total_vnode[i] = newVnode;
            return newVnode;
        }
    }

    free(newVnode->vnode_data); // free the inode !
    free(newVnode);
    return NULL;    // cannot create vnode
}

void string_to_fatname(const char* name, char* nameOut)
{
    char fatName[12];

    // convert from name to fat name
    memset(fatName, ' ', sizeof(fatName));
    fatName[11] = '\0';

    const char* ext = strchr(name, '.');
    if (ext == NULL)
        ext = name + 11;

    for (int i = 0; i < 8 && name[i] && name + i < ext; i++)
        fatName[i] = toupper(name[i]);

    if (ext != name + 11)
        for (int i = 0; i < 3 && ext[i + 1]; i++)
            fatName[i + 8] = toupper(ext[i + 1]);

    memcpy(nameOut, fatName, 12);
}

fat_dir_entry_t* fat12_lookup_in_dir(uint32_t* dir, char* fatname, int dirEntryCount)
{
    fat_dir_entry_t* current_entry;

    for(int i = 0; i < dirEntryCount; i++)
    {
        current_entry = (fat_dir_entry_t*)(dir + i);

        if(strncmp(current_entry->filename, fatname, 11) == 0)
            return current_entry;
    }

    return NULL;
}

int fat12_lookup(vnode_t* node, const char* name, struct vnode** result)
{
    if(node->vnode_type != VDIR)
    {
        *result = NULL;
        return VFS_ENOTDIR;
    }
    
    fat_dir_entry_t* inode = node->vnode_data;
    fs_info_t* fs_info = node->vnode_vfs->vfs_data;
    
    char fatName[12];
    string_to_fatname(name, fatName);

    // here we need to look either on the root directory or another directory

    
    if((node->flags & VNODE_ROOT) == VNODE_ROOT)
    {
        inode = NULL;
        
        uint16_t root_dir_size = (fs_info->bootSector->root_entry_count * 32) / fs_info->bootSector->bytes_per_sector;
        uint16_t root_dir_offset = fs_info->bootSector->reserved_sector_count + (fs_info->bootSector->table_size_16 * fs_info->bootSector->table_count);
        
        int dirEntryCount = fs_info->bootSector->bytes_per_sector / 32;
        for(int i = 0; i < root_dir_size && inode == NULL; i++)
        {
            device_list[node->vnode_vfs->device_id]->read(fs_info->fat_buffer, root_dir_offset + i, 1, device_list[node->vnode_vfs->device_id]->priv);
            inode = fat12_lookup_in_dir(fs_info->fat_buffer, fatName, dirEntryCount);
        }
        
    }
    else
    {
        uint16_t currentCluster = inode->firstClusterLow;
        inode = NULL;
        int dirEntryCount = (fs_info->bootSector->sectors_per_cluster * fs_info->bootSector->bytes_per_sector) / 32;
        while (currentCluster < 0xFF8 && inode == NULL)
        {
            device_list[node->vnode_vfs->device_id]->read(fs_info->fat_buffer, cluster_to_Lba(currentCluster, fs_info->bootSector), fs_info->bootSector->sectors_per_cluster, device_list[node->vnode_vfs->device_id]->priv);
            inode = fat12_lookup_in_dir(fs_info->fat_buffer, fatName, dirEntryCount);

            currentCluster = get_next_cluster(currentCluster, fs_info->file_allocation_table);
        }
    }

    if(inode != NULL)
    {
        *result = create_vnode(node->vnode_vfs, inode);
        return VFS_OK;
    }

    *result = NULL;
    return VFS_ENOENT;
}