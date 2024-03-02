#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "pcb.h"

int pid_counter = 1;

//Increases the PID and returns it
int generatePID(){
    return pid_counter++;
}

//Allows access to the PID
int getPID() {
    return pid_counter;
}

//In this implementation, Pid is the same as file ID 
PCB* makePCB(int page_count, int offset_end, int* table, bool* valid, char* name, FILE* fp){
    PCB * newPCB = malloc(sizeof(PCB)+sizeof(table)+sizeof(valid)+strlen(name)+sizeof(fp)+1);
    newPCB->pid = generatePID();
    newPCB->PC = 0;
    newPCB->page_count = page_count;
    newPCB->offset_end = offset_end;
    newPCB->page_table = table; 
    newPCB->offset = 0;
    newPCB->valid_page = valid;
    newPCB->priority = false;
    newPCB->filename = (char*)malloc(strlen(name)+1);
    strcpy(newPCB->filename, name);
    newPCB->fp = fp;
    return newPCB;
}