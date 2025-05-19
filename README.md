# VFS Simulator

## A Virtual File System Simulation for Hobby OS

This project is an educational implementation of a simple Virtual File System (VFS), It aims to simulate how operating systems abstract access to multiple file systems using a unified interface.

The core goals of this project are:
- To mimic POSIX/Unix-like filesystem behavior.
- To simulate a basic VFS layer that communicates with multiple backend filesystems.
- To support mountable in-memory filesystems (RAMFS).
- To prepare the architecture for future integration of real disk-based filesystems (e.g., FAT12).

The system introduces basic device abstraction, enabling virtual devices (like RAMFS or .img files) to be mounted and accessed via a common interface. This setup is ideal for experimentation and portability in hobby OS development.

[the test](/test.png)

---

## Design Philosophy Inspired by Kleiman’s Vnode Architecture

This VFS simulation project is inspired by the design principles outlined in [Kleiman’s Vnode Paper (1986)](https://www.cs.fsu.edu/~awang/courses/cop5611_s2024/vnode.pdf). The goal was to implement a modular and Unix-like virtual file system architecture that separates file system-independent logic (VFS layer) from file system-specific implementations (FS layer), following the vnode abstraction model.

According to my understanding of the Vnode Paper, the responsibility for managing vnode object such as creation, deletion, and internal tracking lies with the file system-dependent code.
The VFS layer itself does not directly manage vnode lifecycles; instead, it acts as a standardized interface that facilitates communication between user-level requests and the underlying file systems.

```
┌─────────────────────┐
│    User Interface   │
└─────────┬───────────┘
          │
┌─────────▼───────────┐
│      VFS Layer      │
└─────────┬───────────┘
          │
┌─────────▼───────────┐
│   Device Abstraction│
└──┬──────────┬───────┘
   │          │
┌──▼──┐    ┌──▼──┐
│RAMFS│    │FAT12│  ...
└─────┘    └─────┘
```

---

## Current Implementation: RAM-Based File System (ramfs)

Currently, the system does not support real on-disk file systems. Instead, it uses a memory-based file system (ramfs) for simulation purposes.  
This file system stores all its vnode objects in a static array, along with other related filesystem data. These are encapsulated in the vfs_data field of the mounted file system structure.

This approach allows the file system to efficiently keep track of all allocated vnode instances, reuse them when needed, or delete them as appropriate.

---

## Device Abstraction for Filesystem Integration

To maintain consistency and ensure that the code remains easily portable into a hobby OS (specifically, mine), I introduced the notion of devices into the design even though I don’t yet fully understand all the nuances.

This idea was inspired by its frequent presence in many OS codebases.  
Since mounting a file system typically requires a device, I decided that anything related to a file system whether it's a real hardware device or an in-memory file system like ramfs would be treated uniformly as a device.

---

## Virtual Disk Images and Future Filesystem Support

Inside the disks/ directory, I’ve added several .img files that simulate real storage devices. These are raw disk images preformatted with the FAT12 file system.  
Thanks to device abstraction, these images are treated just like any other device.  
The next step in the evolution of this simulation will be to implement support for reading and writing to these formatted images bringing the project closer to supporting real world file systems.

---

## Project Structure Overview

- *disks/*  
  This directory contains raw .img files representing virtual storage devices. These images are intended to simulate real-world file systems, such as FAT12.

- *device.c / device.h*  
  Provides the abstraction for handling all file systems as generic devices. This layer allows for clean separation and portability across different environments, especially useful in hobby OS development.

- *disk.c / disk.h*  
  Responsible for detecting virtual disk images and registering them as usable devices in the system. This module simulates physical disk detection and setup.

- *ramfs.c / ramfs.h*  
  Implements a RAM-based file system and acts as the file system-dependent driver. It handles vnode creation, lookup, and file operations for files stored in memory.

- *vfs.c / vfs.h*  
  This is the core Virtual File System layer. It abstracts interactions with various file systems, providing a unified interface for mounting, file access, and directory traversal, inspired by the Kleiman vnode architecture.

- *main.c*  
  A simple test driver. It initializes the system, mounts various file systems, and tests file operations like opening, reading, writing, and navigating file structures using the VFS interface.
