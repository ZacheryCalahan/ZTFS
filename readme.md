# Zac's Tormenting File System (ZTFS)
ZTFS is an EXT based file system designed to be simple to implement for my simple hobby operating system. It lacks many features one would expect from a good file system, but is a great stepping stone from a simple file system such as FAT to more complex file systems.

I do not recommend this be used for any serious projects, nor anything you would expect any reliability.

# What is this project
This project contains utilities for creating, editing, and repairing ZTFS file systems that exist within a .img file. This also will include documentation for the file system itself.

# What makes this different from EXT?
- ZTFS does not differentiate between an file, directory, nor a directory entry. All of these are implemented in a fixed size data structure `Entry`. ZTFS uses 3 main structures, including the `Blueprint`, `Block Group Descriptors`, and `Entry`. These are defined in "ZTFS.h".
- No concepts of time required, meaning a file does not require a time stamp.
- No journaling, no redundancy, no extra steps. Though this does create an (inevitable) amount of instability, it does make it simple enough to implement and test knowledge of disk concepts before jumping into more mature file systems.
- Hard cap of (block_size / 4) subentries in a given entry.

# Build Instructions
- Clone the repository on a Linux device
- Run `make build`
- Executable and dependencies are located in the `build/` folder.