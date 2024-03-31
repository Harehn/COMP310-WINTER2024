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
      return -1;
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
  struct dir *dir;
  char name[NAME_MAX + 1];
  dir = dir_open_root();
  if (dir == NULL) {
    return;
  }

  // Loop through all files in root directory, checking for pattern in file contents
  while (dir_readdir(dir, name)) {
    struct file *file_s = get_file_by_fname(name);
    bool opened = false;
    // filesys_open will not output NULL because program loops through existing files
    if (file_s == NULL) {
      file_s = filesys_open(name);  
      opened = true;
      add_to_file_table(file_s, name);
    }

    int size = file_length(file_s);
    offset_t offset = file_tell(file_s);
    file_seek(file_s, 0);

    char* buffer = calloc(size + 1, sizeof(char));
    file_read(file_s, buffer, size);
    char* ptr = strstr(buffer, pattern);
    if (ptr != NULL) {
      printf("%s\n", name);
    }

    file_seek(file_s, offset);
    free(buffer);
    if (opened) {
      remove_from_file_table(name);
    }
  }
  dir_close(dir);
  return;
}

void fragmentation_degree() {
  // TODO
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