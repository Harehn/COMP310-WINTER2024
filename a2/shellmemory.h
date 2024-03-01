#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H
#include <stdbool.h>
#include "pcb.h"
void mem_init();
void mem_reset();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file(PCB* pcb);
void replace_page(PCB* pcb, int page);
char * mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void printShellMemory();
#endif