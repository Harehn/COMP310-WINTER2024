#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>
#include "ready_queue.h"
#include "pcb.h"

// To be used in other functions in shellmemory.c
void mem_free_lines_between(int start, int end);

// VAR_MEM_SIZE and FRAMESIZE are specified during make cmd
#define SHELL_MEM_LENGTH VAR_MEM_SIZE+FRAME_SIZE

// IMPLEMENTATION OF LRU
// Each time a page is created or accessed it gets younger
// Lower age means older, higher age means younger
// Can think of age as dob
// We use an int array for the whole framesize
// We have to make sure we consider the frame store as collections of 3 frames
// The current age increases each time age is assigned
long age[FRAME_SIZE];
long currentAge = 0;

struct memory_struct{
	char *var;
	char *value;
};

// The shell memory is composed of 1) the variable store 2) the frame store
// shellmemory = Variable store + frame store = [VAR_MEM_SIZE] + [FRAME_SIZE]
struct memory_struct shellmemory[SHELL_MEM_LENGTH];

//---------HELPER FUNCTIONS FOR LRU-------------------
//Increases the current age
void increaseAge() {
	currentAge += 1;
}

// Search through frame store to find page with the lowest age
// Returns the index in the age array which is the smallest ie the index corresponding to the oldest frame
// To get the index of the frame in the frame store -> shellmemory[VAR_MEM_SIZE + i]
int getOldest() {
	long oldest = age[0];
	int frame = 0;
	for (int i = 0; i < FRAME_SIZE; i+=3) {
		if (age[i] < oldest) {
			oldest = age[i];
			frame = i;
		}
	}
	return frame;
}

// function to print out and check the values of the age array
void printAge() {
	for (int i = 0; i < FRAME_SIZE; i++) {
		printf("%ld ", age[i]);
	}
}

// Set all the elements of the age array to 0 to prevent weird values
void setAge() {
	for (int i = 0; i < FRAME_SIZE; i++) {
		age[i] = 0;
	}
}


//------------HELPER FUNCTIONS--------------
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


// --------------SHELL MEMORY FUNCTIONS-------------

// Initialize the frame and variable store
void mem_init(){  
	int i;
	for (i=0; i<SHELL_MEM_LENGTH; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
	setAge(); // Set the age array to 0 for all elements
}

// The following functions have been modified to loop through only the variable store part of the shell memory

// resets all the variables in the variable store
void mem_reset() {
	int i;
	for (i=0; i<VAR_MEM_SIZE; i++) {
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair in the variable store
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

//get value based on input key from the variable store
char *mem_get_value(char *var_in) {
	int i;
	for (i=0; i<VAR_MEM_SIZE; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			return strdup(shellmemory[i].value);
		} 
	}
	return NULL;

}


/*
 * Function:  copy_to_mem
 * -----------------------
 * Copy one page of fp to a specified index of the shell memory
 * Loading format - var stores filename, value stores a line
 * Runs through file until at specified line, then copies the page
 * 
 * index:     The corresponding line in the file to go to
 * filename:  Input that is required when calling, stores the unique file name
 * i:         The index in the shell memory at which the page is copied
 * 
 * returns:true if there is more to read in the file, false otherwise 
 */
bool copy_to_mem(int index, FILE* fp, char* filename, size_t i) {
	//start from the beginning and loop until page number
	char* line;
	fseek(fp, 0, SEEK_SET);
	for (int line_num = 0; line_num < index; line_num++) {
		line = calloc(1, 100); // 100 is the maximum length used here
		fgets(line, 100, fp);
		free(line);
	}

	// Write the pages in the memory
	for (size_t j = i; j < i+3; j++){
		age[j-VAR_MEM_SIZE] = currentAge;
		if(!feof(fp)) {	
			line = calloc(1, 100);
			if (fgets(line, 100, fp) == NULL)
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
 * Loading format - var stores fileID, value stores a line
 * If no empty space is found, the least recently used page is kicked out
 * The lines set for the variable store are ignored, only searching through the frame store
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
	//Try to write in empty pages first
	for (i; i < SHELL_MEM_LENGTH; i+=3){
		if(strcmp(shellmemory[i].var,"none") == 0){
			hasSpace = true;
			break;
		}
	}
	if (hasSpace) {
		pcb->page_table[page] = (int)i;
		// Use PC*3 to find the first line in the page
		copy_to_mem((pcb->PC)*3,pcb->fp, pcb->filename, i);
	} else {
		// Get the index for replacement
		int LRU_index = getOldest() + VAR_MEM_SIZE;
		int upper_bound = 2;
		printf("Page fault! Victim page contents: \n");
		for (int i = LRU_index; i < LRU_index+3; i++) {
			if (strcmp(shellmemory[i].var, "none") == 0) {
				upper_bound--;  // Keep track to avoid freeing unallocated memory
			} else {
				printf("%s", shellmemory[i].value);
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
		copy_to_mem((pcb->PC)*3, pcb->fp, pcb->filename, LRU_index);
	}
	pcb->valid_page[page] = true;
}

// Load the first 2 pages into memory
int load_file(PCB* pcb) {
	replace_page(pcb, 0);
	if (pcb->page_count > 1) {  // Stop if there is not a second page
		pcb->PC = 1;  // To access the right lines when copy_to_mem is called
		replace_page(pcb, 1);
		pcb->PC = 0;
	}
	return 0;
}

char * mem_get_value_at_line(int index){
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL;
	// If a page from the framestore is being accessed, increase the age number ie make it younger
	if (index >= VAR_MEM_SIZE || index < SHELL_MEM_LENGTH) {
		int i = index - VAR_MEM_SIZE;
		i -= (i % 3);
		increaseAge();
		age[i] = currentAge;
		increaseAge();
		age[i + 1] = currentAge;
		increaseAge();
		age[i + 2] = currentAge;
	}
	return shellmemory[index].value;
}

// Delete the lines in between start and end index
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