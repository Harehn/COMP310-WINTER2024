#ifndef PCB_H
#define PCB_H
#include <stdbool.h>
/*
 * Struct:  PCB 
 * --------------------
 * pid: process(task) id
 * PC: program counter, stores the index of line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 */
typedef struct
{
    bool priority;
    int pid;
    int PC;
    int page_count;
    int offset_end;
    int* page_table;
    int offset;
    bool* valid_page;
    int job_length_score;
    char* filename;
    FILE* fp;
}PCB;

int generatePID();
int getPID();
PCB * makePCB(int page_count, int offset_end, int* table, bool* valid, char* name, FILE* fp);

#endif