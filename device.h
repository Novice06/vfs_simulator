#pragma once

#include <stdint.h>

#define MAX_NAME_LENGTH 64

typedef struct device
{
	char name[MAX_NAME_LENGTH];
	uint32_t id;
	uint8_t (*read)(uint8_t* buffer, uint32_t offset , uint32_t len, void* dev);
	uint8_t (*write)(uint8_t *buffer, uint32_t offset, uint32_t len, void* dev);
	void *priv;
} device_t;

extern device_t **device_list;
extern int device_num;

void add_device(device_t* device);