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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int copy_in(char *fname) {
  // Get the size
  FILE* file = fopen(fname, "r");
  if (file == NULL) {
      printf("file does not exist");
      return 1;  // File does not exist error
  }
  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  fseek(file, 0, SEEK_SET);

  fsutil_create(fname, size);

  char* buffer = malloc(size + 1 * sizeof(char));
  memset(buffer, 0, size);
  fgets(buffer, size + 1, file);
  fsutil_write(fname, buffer, size + 1);
  free(buffer);

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
        int index = 1;
        block_sector_t previous = all_sectors[index];
        while (index < 100) {
            //if (index != 1) printf("| %d |", all_sectors[index]);
            index++;
            if (all_sectors[index] == '\0') {
                break;
            }
            if (all_sectors[index] - previous > 3) {
                printf("| %d || %d |\n", all_sectors[index], previous);
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
    printf("Fragmentable: %d\n", fragmentable);
    printf("Fragmented:%d\n", fragmented);
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