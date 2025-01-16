#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

// Length of the buffer that is used for
// reading characters from STDIN.
#define ARG_LEN 16

// ------------------------------------------
// Input:
// char *arg[]: String array that stores the 
// 								arguments to execute.
// int start_idx:	Index of first NULL element
// 								in the arg[] string array.
//
// Read the arguments from the STDIN(connected
// to the pipe) and store to the arg[] from 
// start_idx.
// ------------------------------------------
void
readArg(char *arg[], int start_idx)
{
	// Loop that stores one string to arg[] for
	// each iteration.
	while(1){
		char *buf = malloc(ARG_LEN);
		int i = 0; // Index in the buffer.

		memset(buf, 0, ARG_LEN); // Initialize the buffer

		// Loop that reads one character for each iteration.
		while(1){
			if(i >= ARG_LEN){ // If ARG_LEN is not enough.
				fprintf(2, "xargs: need to extend ARG_LEN\n");
				return;
			}
			// If the pipe is closed.
			if(read(0, buf+i, 1) == 0){
				if(i == 0){
					free(buf);
				} else {
					*(buf+i) = 0;	// Add NULL pointer to the end of string
					arg[start_idx++] = buf;
				}
				return;
			} else {
				// Split the string by '\n'.
				if(*(buf+i) == '\n'){
					*(buf+i) = 0;	// Add NULL pointer to the end of string
					arg[start_idx++] = buf;
					break;
				}
				i++;
			}
		}
	}	
}


int
main(int argc, char *argv[])
{
	char *arg[MAXARG];			// string array that stores arguments for xargs.
	char *argExec[MAXARG];	// string array that stores arugments for exec
	int i;
	int execArgNum;					// number of arguments per execution.
	int index = 1;
	
	// Initialize arg[] and argExec[] to NULL pointer.
	for(i = 0; i < MAXARG; i++){
		arg[i] = 0;
		argExec[i] = 0;
	}
	
	// if xargs uses "-n" option.
	if(strcmp(argv[1], "-n") == 0){
		execArgNum = atoi(argv[2]);
		if(execArgNum == 0){
			fprintf(2, "xargs: syntax error\n");
			exit(0);
		}
		for(i = 0; i < argc - 3; i++){
			arg[i] = argv[i+3];
		}
		// Store the new arguments to arg[].
		readArg(arg, argc-3);

	} else {
		execArgNum = MAXARG;
		// Store the existing arguments to arg[].
		for(i = 0; i < argc-1; i++){
			arg[i] = argv[i+1];
		}
		// Store the new arguments to arg[].
		readArg(arg, argc-1);
	}
	
	while(arg[index] != 0){
		argExec[0] = arg[0];
		for(i = 1; i < execArgNum+1; i++){
			argExec[i] = arg[index++];
		}
		if(fork() == 0){
			exec(arg[0], argExec);
		} else {
			wait(0);
		}
	}
	// Free the strings.
	for(i = 0; i < MAXARG; i++){
		if(arg[i] != 0){
			free(arg[i]);
		}
	}
	exit(0);
}

