// Danlin Luo 261040021
// Nitin Kaundun 260786113

#include "fsutil2.h"
#include "bitmap.h"
#include "cache.h"
#include "debug.h"
#include "directory.h"
#include "file.h"
#include "filesys.h"
#include "free-map.h"
#include "fsutil.h"
#include "inode.h"
#include "off_t.h"
#include "partition.h"
#include "round.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int copyable_size_of_file(int size);

int copy_in(char *fname) {
  // Get the size
  FILE* file = fopen(fname, "r");
  if (file == NULL) {
      return 1;  // File does not exist error
  }
  fseek(file, 0, SEEK_END);
  long int size = ftell(file);
  fseek(file, 0, SEEK_SET);

  // Allocates an extra block if null terminating character causes overflow
  int copy_size = size % BLOCK_SECTOR_SIZE == 0 ? 
      copyable_size_of_file(size+1) : copyable_size_of_file(size);
  if (copy_size <= 0) {
    return 2;  // No memory space error
  }

  fsutil_create(fname, copy_size);

  char* buffer = malloc(copy_size + 1 * sizeof(char));
  memset(buffer, 0, copy_size);
  fread(buffer, sizeof(char), copy_size, file);
  fsutil_write(fname, buffer, copy_size + 1);
  free(buffer);

  if (size != copy_size) {
    printf("Warning: could only write %d out of %ld bytes (reached end of file\n", copy_size, size);
  }

  struct file* file_s = get_file_by_fname(fname);
  file_seek(file_s, 0);


  //char* buffer = malloc(11 * sizeof(char));
  //memset(buffer, 0, 10);
  //while (fgets(buffer, 10, file)) {
  //    fsutil_write(fname, buffer, 10);
  //    memset(buffer, 0, 10);
  //}
  //memset(buffer, 0, 10);


  fclose(file);
  return 0;
}

int copy_out(char *fname) {
    int size = fsutil_size(fname);
    if (size == -1) {
      return 1;  // File does not exist error
    }
    struct file *file_s = get_file_by_fname(fname);
    offset_t offset = file_s->pos;
    file_seek(file_s, 0);
    
    char* buffer = malloc((size + 1) * sizeof(char));
    memset(buffer, 0, size + 1);
    fsutil_read(fname, buffer, size);
    //printf(buffer);
    FILE* file = fopen(fname, "w");
    if (file == NULL) {
        printf("file does not exist");
        return 12;  // File write error
    }
    fputs(buffer, file);
    fclose(file);
    
    free(buffer);
    file_seek(file_s, offset);
  return 0;
}

void find_file(char *pattern) {
  //printf("SIZE: %d", length);
  struct dir* dir;
  char name[NAME_MAX + 1];
  dir = dir_open_root();
  if (dir == NULL)
      return;
  while (dir_readdir(dir, name)) {
      //printf("%s\n", name);
      int size = fsutil_size(name);
      if (size == -1) {
          return;  // File does not exist error
      }
      struct file* file_s = get_file_by_fname(name);
      offset_t offset = file_s->pos;
      file_seek(file_s, 0);

      // Search file contents for pattern
      char* buffer = malloc((size + 1) * sizeof(char));
      memset(buffer, 0, size + 1);
      fsutil_read(name, buffer, size);
      char* ptr = strstr(buffer, pattern);
      if (ptr != NULL) {
        printf("%s\n", name);
      }

      file_seek(file_s, offset);
      free(buffer);
  }
      
  dir_close(dir);
  return;
}

void fragmentation_degree() {
    struct dir* dir;
    char name[NAME_MAX + 1];
    dir = dir_open_root();
    if (dir == NULL)
        return;
    int fragmentable = 0;
    int fragmented = 0;
    while (dir_readdir(dir, name)) {
        int size = fsutil_size(name);
        if (size == -1) {
            return;  // File does not exist error
        }
        struct file* file_s = get_file_by_fname(name);
        offset_t offset = file_s->pos;
        file_seek(file_s, 0);

        //printf(name);
        //printf("\nMain Sector: %d\n",file_s->inode->sector);
        struct inode *main = file_s->inode;
        block_sector_t* all_sectors = get_inode_data_sectors(main);
        //int length = sizeof(all_sectors) / sizeof(file_s->inode->sector);
        //printf("Number of sectors: %d", length);
        //printf("FIRST: %d ",all_sectors[1]);

        int file_size = fsutil_size(name);
        int inode_number = file_size / 512;
        if (file_size % 512) {
            inode_number += 1;
        }

        int index = 0;
        block_sector_t previous = all_sectors[index];
        for (index = 1; index < inode_number; index++) {
            //if (index != 1) printf("| %d |", all_sectors[index]);
            /*
            index++;
            if (all_sectors[index] == '\0') {
                break;
            }
            */
            if (all_sectors[index] - previous > 3) {
                //printf("| %d || %d |\n", all_sectors[index], previous);
                fragmented += 1;
                break;
            }
            previous = all_sectors[index];
        }
        if (index == 2) {
            //printf("Not Fragmented\n");
        }
        else {
            //printf("Fragmented\n");
            fragmentable += 1;
        }
        file_seek(file_s, offset);
    }
    printf("Num fragmentable files: %d\n", fragmentable);
    printf("Num fragmented files: %d\n", fragmented);
    printf("Fragmentation pct: %f\n", (float)fragmented / (float)fragmentable);
    dir_close(dir);
    return;
}

int defragment() {
  // TODO
  return 0;
}

void recover(int flag) {
  if (flag == 0) { // recover deleted inodes

    // TODO
  } else if (flag == 1) { // recover all non-empty sectors

    // TODO
  } else if (flag == 2) { // data past end of file.

    // TODO
  }
}


/**
 * Returns the size of the file that can be written into the drive
 * Output is limited based on the freespace in drive
 * Subtract 1 from the upper limit of available blocks to fit the null terminating character
*/
int copyable_size_of_file(int size) {
  int sectors_needed = bytes_to_sectors(size);
  int free_sectors = num_free_sectors();
  int total_indirect = DIRECT_BLOCKS_COUNT + INDIRECT_BLOCKS_PER_SECTOR + 2;
  int total_double_indirect = total_indirect + 1 + INDIRECT_BLOCKS_PER_SECTOR +
          (INDIRECT_BLOCKS_PER_SECTOR * INDIRECT_BLOCKS_PER_SECTOR);

  // Direct blocks + inode: 1 + (0-123) blocks
  if (free_sectors <= DIRECT_BLOCKS_COUNT + 1) {
    if (sectors_needed <= free_sectors-1) {
      return size;
    }
    return BLOCK_SECTOR_SIZE * (free_sectors-1) - 1;
  }
  // 1 indirect block: 2 + (123-251) blocks
  else if (free_sectors <= DIRECT_BLOCKS_COUNT + INDIRECT_BLOCKS_PER_SECTOR + 2) {
    if (sectors_needed <= free_sectors-2) {
      return size;
    }
    return  BLOCK_SECTOR_SIZE * (free_sectors-2) - 1;
  }
  // 1 double indirect block: > 2 + 251 blocks
  else {
    // check for max capacity of inode
    if (free_sectors >= total_double_indirect) {
      if (sectors_needed <= total_double_indirect-131) {
        return size;
      }
      return BLOCK_SECTOR_SIZE * (total_double_indirect - 131) - 1;
    }

    // metadata = 1 inode + 1 indirect + 1 double indirect + x indirect from double indirect
    int metadata_blocks = 3 + DIV_ROUND_UP((free_sectors-(total_indirect+1)),129);
    printf("%d metadata blocks\n", metadata_blocks);
    if (sectors_needed <= free_sectors-metadata_blocks) {
      return size;
    }
    return BLOCK_SECTOR_SIZE * (free_sectors-metadata_blocks) - 1;
  }
  /** calculations for # of indirect blocks in double indirect
   *    1 inode
   *    123 direct
   *    1 indirect
   *    128 pointers
   *    1 double indirect
   *    x indicrect             (x can be a max of 128)
   *    x * 128 double indirect (may be less than 128 given space)
   *    total: 254 + 129x
   *  round up x, last indirect block will point to less than 128 sectors
  */
  return 0;  // error?
}