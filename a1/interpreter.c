#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
//#include <sys/stat.h> // import was left unused
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 7;

int badcommand(){
	printf("%s\n", "Unknown Command");
	return 1;
}

int badsetcommand() {
	printf("%s\n", "Bad command: set");
	return 1;
}

// For run command only
int badcommandFileDoesNotExist(){
	printf("%s\n", "Bad command: File not found");
	return 3;
}

int help();
int quit();
int set(char* var, char* value);
int print(char* var);
int run(char* script);
int badcommandFileDoesNotExist();
int echo(char* var);
int touch(char* name);
int cd(char* dir);
int cat(char* file);

// Interpret commands and their arguments
int interpreter(char* command_args[], int args_size){
	int i;

	if ( args_size < 1 || args_size > MAX_ARGS_SIZE){
		if (args_size < 1) return badcommand();
		if (strcmp(command_args[0], "set") == 0) return badsetcommand();
		return badcommand();
	}

	for ( i=0; i<args_size; i++){ //strip spaces new line etc
		command_args[i][strcspn(command_args[i], "\r\n")] = 0;
	}

	if (strcmp(command_args[0], "help")==0){
	    //help
	    if (args_size != 1) return badcommand();
	    return help();
	
	} else if (strcmp(command_args[0], "quit")==0) {
		//quit
		if (args_size != 1) return badcommand();
		return quit();

	} else if (strcmp(command_args[0], "set")==0) {
		//set
		if (args_size < 3) return badsetcommand();	
		for (i = 3; i < args_size; i++) {
			command_args[2] = strcat(command_args[2], " ");
			command_args[2] = strcat(command_args[2], command_args[i]);
		} 
		return set(command_args[1], command_args[2]);
	
	} else if (strcmp(command_args[0], "print")==0) {
		//print
		if (args_size != 2) return badcommand();
		return print(command_args[1]);
	
	} else if (strcmp(command_args[0], "run")==0) {
		//run
		if (args_size != 2) return badcommand();
		return run(command_args[1]);
	
	} else if (strcmp(command_args[0], "my_ls") == 0) {
		//my_ls
		if (args_size != 1) return badcommand();
		return system("ls");

	} else if (strcmp(command_args[0], "echo")==0) {
		//echo
		if (args_size != 2) return badcommand();
		return echo(command_args[1]);

	} else if (strcmp(command_args[0], "my_touch") == 0) {
		//my_touch
		if (args_size != 2) return badcommand();
		return touch(command_args[1]);

	} else if (strcmp(command_args[0], "my_cd") == 0) {
		//my_cd
		if (args_size != 2) return badcommand();
		return cd(command_args[1]);

	}
	else if (strcmp(command_args[0], "my_mkdir") == 0) {
		//my_mkdir
		if (args_size != 2) return badcommand();
		char buffer[100];
		strcpy(buffer, "mkdir ");
		strcat(buffer, command_args[1]);
		return system(buffer);

	}
	else if (strcmp(command_args[0], "my_cat") == 0) {
		//my_cat
		if (args_size != 2) return badcommand();
		if (cat(command_args[1]) == 0) {
			char buffer[100];
			strcpy(buffer, "cat ");
			strcat(buffer, command_args[1]);
			return system(buffer);
		}
		return 3;
	}
	else return badcommand();
}

int help(){

	// We wanted to add new commands to the description but it is not accepted by the autograder
	//char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory with up to 5 tokens\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n \
echo STRING/$VAR	Displays the STRING or the assigned VAR if denoted with a $\n \
my_ls			Lists all files in the present directory\n \
my_mkdir STRING	Creates a new directory STRING\n \
my_touch STRING	Creates a new file STRING in the current directory\n \
my_cd STRING 		Moves the directory STRING\n \
my_cat STRING		Opens the file STRING and prints the content\n ";
	char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
run SCRIPT.TXT		Executes the file SCRIPT.TXT\n ";
	printf("%s\n", help_string);
	return 0;
}

int quit(){
	printf("%s\n", "Bye!");
	exit(0);
}

int set(char* var, char* value){
	char *link = "=";
	char buffer[1000];
	strcpy(buffer, var);
	strcat(buffer, link);
	strcat(buffer, value);

	mem_set_value(var, value);

	return 0;

}

int print(char* var){
	printf("%s\n", mem_get_value(var)); 
	return 0;
}

int run(char* script){
	int errCode = 0;
	char line[1000];
	FILE *p = fopen(script,"rt");  // the program is in a file

	if(p == NULL){
		return badcommandFileDoesNotExist();
	}

	fgets(line,999,p);
	while(1){
		errCode = parseInput(line);	// which calls interpreter()
		memset(line, 0, sizeof(line));

		if(feof(p)){
			break;
		}
		fgets(line,999,p);
	}

    fclose(p);

	return errCode;
}

int echo(char* var) {
	if (var[0] == '$') {
		char tmp[100];
		strcpy(tmp, var+1);
		char result[100];
		strcpy(result, mem_get_value(tmp));
		if (strcmp(result, "Variable does not exist") == 0) {
			printf("\n");
		} else {
			printf("%s\n", result);
		}
	} else {
		printf("%s\n", var);
	}
	return 0;
}

int touch(char* name) {
	fclose(fopen(name, "w"));
	return 0;
}

int cd(char* dir) {
	int code = chdir(dir);
	if (code == -1) {
		printf("Bad command: my_cd\n");
		return 3;
	}
	return 0;
}

int cat(char* file) {
	FILE *fp;

	if (fp = fopen(file, "r")) {
		fclose(fp);
	} else {
		printf("Bad command: my_cat\n");
		return 3;
	}
	return 0;
}