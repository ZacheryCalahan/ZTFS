/*
    Handler for any data to and from disk.
*/

#ifndef ZTFS_BLOCK_H
#define ZTFS_BLOCK_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "ztfs.h"

/**
 * @brief Read from `image_file` `size` bytes `n` times starting from absolute address `offset`.
 * 
 * @param image_file File of image
 * @param buffer Buffer for data read in, must be `size * n` bytes long.
 * @param size Bytes to read in
 * @param n Times to read in `size` bytes
 * @param offset Absolute address on disk to read
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_read(FILE *image_file, void *buffer, size_t size, size_t n, size_t offset);

/**
 * Write to `image_file` `size` bytes `n` times starting from absolute address `offset`.
 * @param image_file File of image
 * @param buffer Buffer of data to write, must be `size` bytes long.
 * @param size Bytes to write
 * @param n Times to write `size` bytes
 * @param offset Absolute address on disk to write to 
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_write(FILE *image_file, void *buffer, size_t size, size_t n, size_t offset);

/**
 * @brief Read block group `bg_num` into `desc` from `image_file`.
 * 
 * @param image_file File of image
 * @param desc Buffer to read in to
 * @param bg_num Block group number of the descriptor
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_read_bgd(FILE *image_file, struct ztfs_block_group_descriptor *desc, uint32_t bg_num);

/**
 * @brief Write `desc` into block group `bg_num` from `image_file`.
 * 
 * @param image_file File of image
 * @param desc Buffer of data to write
 * @param bg_num Block group number of the descriptor
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_write_bgd(FILE *image_file, struct ztfs_block_group_descriptor *desc, uint32_t bg_num);

/**
 * @brief Read from `image_file` the `blueprint`
 * 
 * @param image_file File of image
 * @param blueprint Buffer to read data in to
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_read_blueprint(FILE *image_file, struct ztfs_blueprint *blueprint);

/**
 * @brief Write to `image_file` `blueprint`
 * 
 * @param image_file File of image
 * @param blueprint Buffer holding `blueprint` data to write
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_write_blueprint(FILE *image_file, struct ztfs_blueprint *blueprint);

/**
 * @brief Read in a full block bitmap from `image_file` into `bitmap_block` from group `bg_num.
 * 
 * @param image_file File of image
 * @param bitmap_block Buffer to hold data read in
 * @param bg_num Number of the block group to get bitmap from
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_read_bitmap(FILE *image_file, void* bitmap_block, uint32_t bg_num);

/**
 * @brief Mark `baddr_block` block used in `image_file`.
 * 
 * @param image_file File of image
 * @param baddr_block Block address to mark as used
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_mark_block_used_bitmap(FILE *image_file, baddr_t baddr_block);

/**
 * @brief Mark `baddr_block` block as a free block in `image_file`.
 * 
 * @param image_file File of image
 * @param baddr_block Block address to mark as free
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_free_block_bitmap(FILE *image_file, baddr_t baddr_block);

/**
 * @brief Find a free block in `image_file`
 * 
 * @param image_file File of image
 * 
 * @returns Block address of a free block, or 0 if not found.
 */
baddr_t ztfs_find_unused_block(FILE *image_file);

/**
 * @brief Find an `entry` from absolute `path`.
 * 
 * @param image_file File of image
 * @param entry Buffer to read in entry found
 * @param path Absolute path to entry
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_find_entry_via_path(FILE *image_file, struct ztfs_entry *entry, char *path);

/**
 * @brief Find a free entry in `parent` and mark location in `data_block_idx` and `entry_idx` from `image_file`
 * 
 * @param image_file File of image
 * @param parent Parent entry to search for free entry
 * @param data_block_idx Buffer for index into indirect block the free entry is in
 * @param entry_idx Buffer for index into entry array the free entry is in
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_find_free_entry(FILE *image_file, struct ztfs_entry *parent, uint32_t *data_block_idx, uint32_t *entry_idx);

/**
 * @brief Insert `child` entry into `parent` entry in `image_file`.
 * 
 * @param image_file File of image
 * @param parent Parent entry
 * @param child Entry to insert into parent
 * 
 * @returns 0 on success, and -1 on failure.
 */
int ztfs_place_entry(FILE *image_file, struct ztfs_entry *parent, struct ztfs_entry *child);

/**
 * @brief Find the block group that `baddr_block` resides in
 * 
 * @param image_file File of image
 * @param baddr_block Block address to find the block group of
 * 
 * @returns -1 on failure, or number of the block group on success.
 */
uint32_t ztfs_find_block_group_from_baddr(FILE *image_file, baddr_t baddr_block);

#endif