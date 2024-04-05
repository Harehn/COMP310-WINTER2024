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

struct file_elem {
  struct list_elem elem;
  char* fname;
  char* contents;
  int size;
};

int copyable_size_of_file(int size);

/*
* Get the size of the input file
* Open a 'file' on the 'hard disk' of that size
* Write the contents of the input file into the output file using a buffer
*/
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

  fsutil_create(fname, copy_size); //Create file

  char* buffer = malloc(copy_size + 1 * sizeof(char));
  memset(buffer, 0, copy_size);
  fread(buffer, sizeof(char), copy_size, file);
  fsutil_write(fname, buffer, copy_size + 1);
  free(buffer);

  if (size != copy_size) {
    printf("Warning: could only write %d out of %ld bytes (reached end of file\n", copy_size, size);
  }

  //Reset pointer of the file to the start
  struct file* file_s = get_file_by_fname(fname);
  file_seek(file_s, 0);
  fclose(file);
  return 0;
}

/*
* Get the size of the 'file' on the 'hard disk'
* Write the contents of the 'file' in a buffer
* Create the output file and write the content 
*/
int copy_out(char *fname) {
    //Get the size and check if everything is okay
    int size = fsutil_size(fname);
    if (size == -1) {
      return 1;  // File does not exist error
    }
    // keep track of the original position of the pointer
    struct file *file_s = get_file_by_fname(fname);
    offset_t offset = file_s->pos;
    file_seek(file_s, 0);
    
    //Create a buffer and write the content into the buffer
    char* buffer = malloc((size + 1) * sizeof(char));
    memset(buffer, 0, size + 1);
    fsutil_read(fname, buffer, size);
    //Open and write content into output file
    FILE* file = fopen(fname, "w");
    if (file == NULL) {
        printf("file does not exist");
        return 12;  // File write error
    }
    fputs(buffer, file);

    //Clear out resources
    fclose(file);
    free(buffer); 

    //Reset pointer
    file_seek(file_s, offset);
  return 0;
}

/*
* Open root directory
* Iterate through each file and open the content
* Math the pattern and print the filename
*/
void find_file(char *pattern) {
  //Open root directory
  struct dir* dir;
  char name[NAME_MAX + 1];
  dir = dir_open_root();
  if (dir == NULL)
      return;
  // Iterate through directory
  while (dir_readdir(dir, name)) {
      int size = fsutil_size(name);
      if (size == -1) {
          return;  // File does not exist error
      }
      // keep track of the original position of the pointer
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

      // Reset pointer
      file_seek(file_s, offset);
      free(buffer);
  }
      
  dir_close(dir);
  return;
}

/*
* Open and iterate through the root directory
* If file has more than 2 data block, increase number of fragmentable files
* Go through the data blocks in order. If the difference between 2 consecutive blocks' index >3, the file is fragmented
*/
void fragmentation_degree() {
    //Open root directory
    struct dir* dir;
    char name[NAME_MAX + 1];
    dir = dir_open_root();
    if (dir == NULL)
        return;
    int fragmentable = 0;
    int fragmented = 0;
    //Iterate through directory
    while (dir_readdir(dir, name)) {
        int size = fsutil_size(name);
        if (size == -1) {
            return;  // File does not exist error
        }
        // keep track of the original position of the pointer
        struct file* file_s = get_file_by_fname(name);
        offset_t offset = file_s->pos;
        file_seek(file_s, 0);

        // Get the data Sectors
        struct inode *main = file_s->inode;
        block_sector_t* all_sectors = get_inode_data_sectors(main);
        int file_size = fsutil_size(name);
        int inode_number = bytes_to_sectors(file_size);

        //Iterate through the data sectors
        int index = 0;
        block_sector_t previous = all_sectors[index];
        for (index = 1; index < inode_number; index++) {
            if (all_sectors[index] - previous > 3) { //File is fragmented if the difference >3
                fragmented += 1;
                break;
            }
            previous = all_sectors[index];
        }

        if (inode_number > 1) {//2 more data blocks mean that file is fragmentable
            fragmentable += 1;
        }
        
        free(all_sectors);
        file_seek(file_s, offset); // Reset file pointer
    }
    printf("Num fragmentable files: %d\n", fragmentable);
    printf("Num fragmented files: %d\n", fragmented);
    printf("Fragmentation pct: %f\n", (float)fragmented / (float)fragmentable);
    dir_close(dir);
    return;
}

/**
 * Defragments by iterating through files and writing contents into memory
 * The disk is cleared and then the files are sequentially rewritten into the disk
*/
int defragment() {
  struct dir* dir;
  char name[NAME_MAX + 1];
  dir = dir_open_root();
  if (dir == NULL)
    return 11;  // Filesystem error

  struct list file_list = LIST_INITIALIZER (file_list);

  // Store files in memory through a linked list
  while (dir_readdir(dir, name)) {
    int size = fsutil_size(name);
    if (size == -1) {
        return 1;  // File does not exist error
    }

    struct file* file_s = get_file_by_fname(name);
    offset_t offset = file_s->pos;
    file_seek(file_s, 0);

    // Copy contents into a list node
    char* buffer = calloc(sizeof(char), size+1);
    if (fsutil_read(name, buffer, size) == -1)
      return 13;  // File read error

    // Copy name to put into a file_elem
    char* name_cpy = calloc(sizeof(char), strlen(name)+1);
    strcpy(name_cpy, name);
    name_cpy[strlen(name)] = '\0';

    struct file_elem* f = (struct file_elem*) malloc(sizeof(struct file_elem));
    f->elem = (struct list_elem) {NULL, NULL};
    f->contents = buffer;
    f->fname = name_cpy;
    f->size = size;
    list_push_back(&file_list, &f->elem);

    file_seek(file_s, offset);
  }  
  dir_close(dir);

  // Iterate over llist and clear the disk
  struct list_elem* e;
  for (e = list_begin(&file_list); e != list_end(&file_list); e = list_next(e)) {
    struct file_elem *f = list_entry (e, struct file_elem, elem);
    remove_from_file_table(f->fname);
    filesys_remove(f->fname);
  }

  // Iterate over llist and write files back into disk
  while (!list_empty(&file_list)) {
    e = list_pop_front(&file_list);
    struct file_elem *f = list_entry (e, struct file_elem, elem);
    fsutil_create(f->fname, f->size);

    fsutil_write(f->fname, f->contents, f->size);
    fsutil_seek(f->fname, 0);
    
    // Free the node contents
    free(f->fname);
    free(f->contents);
    free(f);
  }
  return 0;
}

void recover(int flag) {
  if (flag == 0) { // recover deleted inodes
      /*
      * We look at the free sectors on the Hard disk. If the corresponding inode points to data and it is not a
      * directory, we restore the inode ie add it to the root directory.
      */
      int count = bitmap_size(free_map);
      // We iterate through the bitmap and look at the free sectors on the bitmap
      for (int sector = 0; sector < count; sector++) {
          int free = bitmap_test(free_map, sector);
          struct inode* curr_inode;
          if (!free) {
              curr_inode = inode_open(sector);
              int has_data = inode_length(curr_inode);
              if (has_data > 0) { //If the inode points to data
                  if (! inode_is_directory(curr_inode)) { //If the data is not a directory
                      char filename[30];
                      sprintf(filename, "recovered0-%d", sector);
                      struct dir* root = dir_open_root();
                      bool is_dir = false; // Set to false as we are adding a file and not a directory
                      dir_add(root, filename, sector, is_dir);
                      dir_close(root);
                  }
              }
          }
      }
  } else if (flag == 1) { // recover all non-empty sectors
    for (int i = 4; i < bitmap_size(free_map); i++) {
      char buffer[BLOCK_SECTOR_SIZE] = {0};
      buffer_cache_read(i, buffer);
      // Write into the recovered file if a nonzero character is found
      for (int j = 0; j < BLOCK_SECTOR_SIZE; j++) {
        if (buffer[j] != 0) {
          char name[30];
          sprintf(name, "recovered1-%d.txt", i);
          FILE* file = fopen(name, "w");
          fputs(buffer, file);
          fclose(file);
          break;
        }
      }
    }
  } else if (flag == 2) { // data past end of file.
    struct dir* dir;
    dir = dir_open_root();
    char name[NAME_MAX + 1];
    
    while (dir_readdir(dir, name)) {
      int size = fsutil_size(name);
      if (size == -1)
        continue;
      // If file takes up the entire block, there is nothing to read
      int eof = size % BLOCK_SECTOR_SIZE;
      if (eof == 0)
        continue;

      struct file* file_s = get_file_by_fname(name);
      char buffer[BLOCK_SECTOR_SIZE] = {0};
      block_sector_t* sectors = get_inode_data_sectors(file_s->inode);
      int last_sector = sectors[bytes_to_sectors(size)-1];
      // Only get the contents of the last sector
      buffer_cache_read(last_sector, buffer);
      for (int i = eof; i < BLOCK_SECTOR_SIZE; i++) {
        if (buffer[i] != 0) {
          printf("entire: %s\nstart at %d: %s\n", buffer, i, &buffer[i]);
          char fname[30+strlen(name)];
          sprintf(fname, "recovered2-%s.txt", name);
          FILE* f = fopen(fname, "w");
          fputs(&buffer[i], f);
          fclose(f);
          break;
        }
      }
      free(sectors);
    }
    dir_close(dir);
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