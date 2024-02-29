#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1;

int generatePID(){
    return pid_counter++;
}

int getPID() {
    return pid_counter;
}

//In this implementation, Pid is the same as file ID 
PCB* makePCB(int end, int offset_end, int* table, int* table_end){
    PCB * newPCB = malloc(sizeof(PCB)+sizeof(table));
    newPCB->pid = generatePID();
    newPCB->PC = 0;
    newPCB->end = end;
    newPCB->offset_end = offset_end;
    newPCB->page_table = table; 
    newPCB->offset = 0;
    newPCB->table_end = table_end;
    // newPCB->job_length_score = 1+end-start;
    newPCB->priority = false;
    return newPCB;
}