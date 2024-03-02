#ifndef PCB_H
#define PCB_H
#include <stdbool.h>
/*
 * Struct:  PCB 
 * --------------------
 * priority: priority flag
 * pid: process(task) id
 * PC: program counter, stores the index of line that the task is executing
 * page_count: 
 * offset_end:
 * page_table: array to hold the indices of the pages
 * offset:s
 * valid_page:
 * job_length_score: for EXEC AGING use only, stores the job length score
 * filename: name of the file from which commands are read from
 * fp: Filestream object to read from
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

//Helper function to get the PID
int generatePID();
//Function to get the PID
int getPID();
// Create PCB
PCB * makePCB(int page_count, int offset_end, int* table, bool* valid, char* name, FILE* fp);

#endif