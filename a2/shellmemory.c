#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>

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
 * Function:  load_file
 * --------------------
 * Load the source code of the file fp into the shell memory:
 * 		Loading format - var stores fileID, value stores a line
 *		The lines set for the variable store are ignored, only searching through the frame store
 *
 *  pStart: This function will store the first line of the loaded file 
 * 			in shell memory in here
 *	pOffset_End: This function wil store the last line in the offset of the
 *			final page in here
 *  page_table: This function will store pointers from the pages to the the
 * 			location of the code in shell memory here
 *  fileID: Input that need to provide when calling the function, 
 *			stores the ID of the file
 * 
 * returns: 0
 */
int load_file(FILE* fp, int* pEnd, int* pOffset_End, int* page_table, char* filename)
{
	char *line;
    size_t i;
	bool flag = true;
	i=VAR_MEM_SIZE;
	*pOffset_End = 3;
	int counter = 0;  // Counts number of pages
	while(flag){
		if (feof(fp)) {
			break;  // Avoid incrementing counter if there's no more to read
		}
		for (i; i < SHELL_MEM_LENGTH; i+=3){
			if(strcmp(shellmemory[i].var,"none") == 0){
				page_table[counter] = i;
				counter++;
				break;
			}
		}
		for (size_t j = i; j < i+3; j++){
			if(feof(fp))
			{
				*pOffset_End = (int)(j-i);
				flag = false;
				break;
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
	}
	// Set end offset to 3 for when the process is freed
	if (*pOffset_End == 0) {
		*pOffset_End = 3;
		printf("changed offset to %d\n", *pOffset_End);
	}
	*pEnd = counter;
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