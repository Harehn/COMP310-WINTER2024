#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>
#include "ready_queue.h"
#include "pcb.h"

// To be used in other functions in shellmemory.c
void mem_free_lines_between(int start, int end);

//#define VAR_MEM_SIZE 100
//#define FRAME_SIZE 300
#define SHELL_MEM_LENGTH VAR_MEM_SIZE+FRAME_SIZE

struct memory_struct{
	char *var;
	char *value;
};

// The shell memory is now composed of 1) the variable store 2) the frame store
struct memory_struct shellmemory[SHELL_MEM_LENGTH];

// Helper functions
int match(char *model, char *var) {
	int i, len=strlen(var), matchCount=0;
	for(i=0;i<len;i++)
		if (*(model+i) == *(var+i)) matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model) {
	char token='=';    // look for this to find value
	char value[1000];  // stores the extract value
	int i,j, len=strlen(model);
	for(i=0;i<len && *(model+i)!=token;i++); // loop till we get there
	// extract the value
	for(i=i+1,j=0;i<len;i++,j++) value[j]=*(model+i);
	value[j]='\0';
	return strdup(value);
}


// Shell memory functions

void mem_init(){  // Initialize the frame and variable store
	int i;
	for (i=0; i<SHELL_MEM_LENGTH; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// These functions have been modified to loop through only the variable store part of the shell memory

void mem_reset() {
	int i;
	for (i=0; i<VAR_MEM_SIZE; i++) {
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=0; i<VAR_MEM_SIZE; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<VAR_MEM_SIZE; i++){
		if (strcmp(shellmemory[i].var, "none") == 0){
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	return;

}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;
	for (i=0; i<VAR_MEM_SIZE; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			return strdup(shellmemory[i].value);
		} 
	}
	return NULL;

}


void printShellMemory(){
	int count_empty = 0;
	for (int i = 0; i < SHELL_MEM_LENGTH; i++){
		if(strcmp(shellmemory[i].var,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
		}
    }
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", SHELL_MEM_LENGTH, SHELL_MEM_LENGTH-count_empty, count_empty);
}

/*
 * Function:  copy_to_mem
 * -----------------------
 * Copy one page of fp to a specified index of the shell memory
 *  	Loading format - var stores filename, value stores a line
 * 
 * filename:  Input that is required when calling, stores the unique file name
 * i:  The index in the shell memory at which the page is copied
 * 
 * returns:true if there is more to read in the file, false otherwise 
 */
// ! May have to update to search for a page from the beginning
bool copy_to_mem(FILE* fp, char* filename, size_t i) {
	char *line;
	for (size_t j = i; j < i+3; j++){
		if(feof(fp))
		{
			return false;
		}else{
			line = calloc(1, SHELL_MEM_LENGTH);
			if (fgets(line, SHELL_MEM_LENGTH, fp) == NULL)
			{
				continue;
			}
			shellmemory[j].var = strdup(filename);
			shellmemory[j].value = strndup(line, strlen(line));
			free(line);
		}
	}
	return !feof(fp);
}

/*
 * Function:  replace_page
 * --------------------
 * Load a page into the frame store
 * 		Loading format - var stores fileID, value stores a line
 * 		If no empty space is found, the least recently used page is kicked out
 *		The lines set for the variable store are ignored, only searching through the frame store
 *
 *  pcb: Input that is required to access all relevant fields of a process
 *  page: Input that details which page of the pcb's file that needs to be loaded
 * 
 */

void replace_page(PCB* pcb, int page)
{
	char *line;
    size_t i;
	bool hasSpace = false;
	i=VAR_MEM_SIZE;
	for (i; i < SHELL_MEM_LENGTH; i+=3){
		if(strcmp(shellmemory[i].var,"none") == 0){
			hasSpace = true;
			break;
		}
	}
	if (hasSpace) {
		pcb->page_table[page] = (int)i;
		copy_to_mem(pcb->fp, pcb->filename, i);
	} else {
		int LRU_index = VAR_MEM_SIZE; // ! Change this later to LRU
		int upper_bound = 2;
		printf("Page fault! Victim page contents: \n");
		for (int i = LRU_index; i < LRU_index+3; i++) {
			if (strcmp(shellmemory[i].var, "none") == 0) {
				upper_bound--;  // Keep track to avoid freeing unallocated memory
			} else {
				printf("\nline %d: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
			}
		}
		printf("End of victim page contents.\n");
		mem_free_lines_between(LRU_index, LRU_index+upper_bound);

		// Find the corresponding PCB and change its valid bit to false
		PCB* prev_pcb;
		if (strcmp(shellmemory[LRU_index].var, pcb->filename) == 0) {
			prev_pcb = pcb;
		} else {
			prev_pcb = find_process(shellmemory[LRU_index].var);
		}

		// Skip the valid bit if the data is from a completed process
		if (prev_pcb != NULL) {
			for (int i = 0; i < prev_pcb->page_count; i++) {
				if (prev_pcb->page_table[i] == LRU_index) {
					prev_pcb->valid_page[i] = false;
					break;
				}
			}
		}
		// Copy page into memory
		pcb->page_table[page] = LRU_index;
		copy_to_mem(pcb->fp, pcb->filename, LRU_index);
	}
	pcb->valid_page[page] = true;
}

// Load the first 2 pages into memory
int load_file(PCB* pcb) {
	replace_page(pcb, 0);
	replace_page(pcb, 1);
	return 0;
}

char * mem_get_value_at_line(int index){
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL; 
	return shellmemory[index].value;
}

void mem_free_lines_between(int start, int end){
	for (int i=start; i<=end && i<SHELL_MEM_LENGTH; i++){
		if(shellmemory[i].var != NULL){
			free(shellmemory[i].var);
		}	
		if(shellmemory[i].value != NULL){
			free(shellmemory[i].value);
		}	
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}