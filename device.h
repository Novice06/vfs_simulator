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

#define MAX_NAME_LENGTH 64

/*
 * Structure to Simulate a Device in an OS-like Environment  
 *
 * At this point, I honestly don’t know exactly why we need to implement this kind of structure.
 * All I know is that I’ve seen similar ones used in many codebases. In this case, 
 * it helps simplify things when mounting a VFS that expects a device ID field.
*/
typedef struct device
{
	char name[MAX_NAME_LENGTH];
	uint32_t id;	// unique id

	// functions to interact with the device
	void (*read)(uint8_t* buffer, uint32_t offset , uint32_t len, void* dev);
	void (*write)(const uint8_t *buffer, uint32_t offset, uint32_t len, void* dev);

	void *priv;	// private data of the device ...
} device_t;


/*
 * Why These Variables Are Declared as extern:  
 *
 * I declared these variables as extern in this header because 
 * I didn’t want to create getter functions. I’m not sure if that’s the best idea, 
 * but it keeps things simple for now.
*/
extern device_t **device_list;
extern int device_num;

void add_device(device_t* device);