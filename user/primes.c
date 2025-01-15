#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// -----------------------------------------------
// Helper function
// Input:
// int p[]:	Pipe connected from parent to child.
// int pastReadFD: Read end of the pipe connected
// 									from grandparent to parent.
//
// 	Create a child that create the pipe that 
// 	connects to its child (grandchild). Receive
// 	data from a parent, filter the data, and send
// 	the data to its child.
// -----------------------------------------------
void
createChild(int p[2], int pastReadFD)
{
	int prime;
	int n;
	int pc[2];	// Pipe that will connect child
							// and grandchild.

	// Child process
	if(fork() == 0){
		close(p[1]);						

		// Close the fild descriptor passed as an argument
		// to secure spare file descriptors.
		if(pastReadFD != -1){
			close(pastReadFD);
		}

		// If no input received from the pipe,
		// close the pipe and exit.
		if(read(p[0], &prime, sizeof(int)) == 0){
			close(p[0]);
			exit(0);
		} else {
			// If there's input from the pipe, print it.
			printf("prime %d\n", prime);
		}
		

		// Create a pipe from child to grandchild.
		pipe(pc);

		// Recursively call the function.
		// Since grandchild doesn't need read end of pipe
		// connected from parent to child, pass it to the 
		// function.
		createChild(pc, p[0]);
		
		// Recieve data from parent, filter the data,
		// and send the data to its child.
		while(read(p[0], &n, sizeof(int)) != 0){
			if(n % prime != 0){
				write(pc[1], &n, sizeof(int));
			}
		}

		close(p[0]);
		close(pc[1]);
		wait(0);
		exit(0);

	} else {
		// Parent process.
		close(p[0]);
	}		
}


int
main(int argc, char *argv[])
{
	int n;
	int p[2];

	if(argc == 1){
		pipe(p);	
		createChild(p, -1);

		for(n = 2; n <=280; n++){
			write(p[1], &n, sizeof(int));
		}
		close(p[1]);
		wait(0);

	} else {
		fprintf(2, "Error: primes doesn't require any arguments.\n");
	}

	return 0;
}
