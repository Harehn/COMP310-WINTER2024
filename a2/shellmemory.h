#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H
void mem_init();
void mem_reset();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file(FILE* fp, int* pEnd, int* pOffset_End, int* page_table, char* fileID);
char * mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
#endif