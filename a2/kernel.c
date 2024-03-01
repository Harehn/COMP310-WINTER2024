#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"
#include "interpreter.h"

bool active = false;
bool debug = false;
bool in_background = false;

int process_initialize(char *filename){
    FILE *fp, *cp;
    
    // Check if file exists
    fp = fopen(filename, "rt");
    if(fp == NULL){
		return FILE_DOES_NOT_EXIST;
    }

    char pid[3];
    sprintf(pid, "%d", getPID());

    // Copy file into backing store, using PID as the unique identifier
    char* newfile = (char*)malloc(strlen("./backing_store")+strlen(filename)+3*sizeof(int));
    strcpy(newfile, "./backing_store/");
    strcat(newfile, filename);
    strcat(newfile, pid);
    cp = fopen(newfile, "w");
    fp = fopen(filename, "rt");
    char c;
    c = fgetc(fp);
    int line_count = 1;
    while (c != EOF)
    {
        fputc(c, (FILE *)cp);
        if (c == '\n') {
            line_count += 1;
        }
        c = fgetc(fp);
    }
    fclose(cp);
    
    // Get size of page table and end marker based on line count
    int offset_end = (line_count%3 == 0) ? 3 : line_count%3;
    int page_count = line_count/3;
    if (offset_end != 3) {
        page_count += 1;
    }
    int* page_table = (int*)(malloc(sizeof(int)*(page_count)));
    // Allocate default false value to valid bits
    bool* valid_page = (bool*)calloc(page_count, sizeof(bool));

    // Load backing store copy into frame store
    cp = fopen(newfile, "rt");

    PCB* newPCB = makePCB(page_count, offset_end, page_table, valid_page, newfile, cp);

    int error_code = load_file(newPCB);
    if(error_code != 0){
        fclose(cp);
        return FILE_ERROR;
    }
    // Leave cp open to access later when paging

    QueueNode *node = malloc(sizeof(QueueNode)+sizeof(PCB));
    node->pcb = newPCB;

    ready_queue_add_to_tail(node);

    free(newfile);
    fclose(fp);
    return 0;
}

// int shell_process_initialize(){
//     //Note that "You can assume that the # option will only be used in batch mode."
//     //So we know that the input is a file, we can directly load the file into ram
//     int* start = (int*)malloc(sizeof(int));
//     int* end = (int*)malloc(sizeof(int));
//     int error_code = 0;
//     error_code = load_file(stdin, start, end, "_SHELL");
//     if(error_code != 0){
//         return error_code;
//     }
//     PCB* newPCB = makePCB(*start,*end);
//     newPCB->priority = true;
//     QueueNode *node = malloc(sizeof(QueueNode));
//     node->pcb = newPCB;

//     ready_queue_add_to_head(node);

//     freopen("/dev/tty", "r", stdin);
//     return 0;
// }

bool execute_process(QueueNode *node, int quanta){
    char *line = NULL;
    PCB *pcb = node->pcb;
    for(int i=0; i<quanta; i++){
        // If page cannot be accessed, send to back of the queue and find the page
        if (!pcb->valid_page[pcb->PC]) {
            replace_page(pcb, pcb->PC);
            ready_queue_add_to_tail(node);
            return true;  // To avoid RR adding the node again
        }
    
        line = mem_get_value_at_line(pcb->page_table[pcb->PC] + pcb->offset++);

        // Check that there are no more pages or the current page does not have more lines
        if(pcb->PC==pcb->page_count-1 && pcb->offset==pcb->offset_end){
            parseInput(line);
            terminate_process(node);
            in_background = false;
            return true;
        }

        // Only move to the next page once the current one is done loading
        if (pcb->offset == 3) {
            pcb->offset = 0;
            pcb->PC++;
        }
        in_background = true;
        if(pcb->priority) {
            pcb->priority = false;
        }
        
        parseInput(line);
        in_background = false;
    }
    return false;
}

void *scheduler_FCFS(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;   
        }
        cur = ready_queue_pop_head();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_SJF(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_AGING_alternative(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        if(!execute_process(cur, 1)) {
            ready_queue_add_to_head(cur);
        }   
    }
    return 0;
}

void *scheduler_AGING(){
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if(shortest < cur->pcb->job_length_score){
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        if(!execute_process(cur, 1)) {
            ready_queue_add_to_head(cur);
        }
    }
    return 0;
}

void *scheduler_RR(void *arg){
    int quanta = ((int *) arg)[0];
    QueueNode *cur;
    while(true){
        if(is_ready_empty()){
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        if(!execute_process(cur, quanta)) {
            ready_queue_add_to_tail(cur);
        }
    }
    return 0;
}

int schedule_by_policy(char* policy){ //, bool mt){
    if(strcmp(policy, "FCFS")!=0 && strcmp(policy, "SJF")!=0 && 
        strcmp(policy, "RR")!=0 && strcmp(policy, "AGING")!=0 && strcmp(policy, "RR30")!=0){
            return SCHEDULING_ERROR;
    }
    if(active) return 0;
    if(in_background) return 0;
    int arg[1];
    if(strcmp("FCFS",policy)==0){
        scheduler_FCFS();
    }else if(strcmp("SJF",policy)==0){
        scheduler_SJF();
    }else if(strcmp("RR",policy)==0){
        arg[0] = 2;
        scheduler_RR((void *) arg);
    }else if(strcmp("AGING",policy)==0){
        scheduler_AGING();
    }else if(strcmp("RR30", policy)==0){
        arg[0] = 30;
        scheduler_RR((void *) arg);
    }
    return 0;
}

