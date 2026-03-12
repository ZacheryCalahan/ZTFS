# Zac's Tormenting File System (ZTFS)
ZTFS is an EXT based file system designed to be simple to implement for my simple hobby operating system. It lacks many features one would expect from a good file system, but is a great stepping stone from a simple file system such as FAT to more complex file systems.

I do not recommend this be used for any serious projects, nor anything you would expect any reliability.

# What is this project
This project contains utilities for creating, editing, and repairing ZTFS filesystems that exist within a .img file. This also will include documentation for the filesystem itself.

# What makes this different from EXT?
- ZTFS does not differentiate between an file, directory, nor a directory entry. All of these are implemented in a fixed size data structure `Entry`. ZTFS uses 3 main structures, including the `Blueprint`, `Block Group Descriptors`, and `Entry`. These are defined in "ZTFS.h".
- No concepts of time required, meaning a file does not require a timestamp.
- No journaling, no redundancy, no extra steps. Though this does create an (inevitable) amount of instability, it does make it simple enough to implement and test knowledge of disk concepts before jumping into more mature file systems.

### `Blueprint`
Includes all data required to load and properly read/write to the filesystem.

### `Block Group Descriptor`
Similar to its EXT2 counterpart, this gives all information about a given block group.

### `Entry`
Data structure that hold any information about a given file, directory, or other types of data that the disk can hold. Any data on disk MUST be represented by an `Entry`.