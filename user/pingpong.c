#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	int p1[2]; 	// Pipe from parent to child.
	int p2[2];	// Pipe from child to parent.
	char buf[1];
	int pid;

	if(argc == 1){
		pipe(p1);
		pipe(p2);

		// Child process
		if(fork() == 0){
			// Setting pipe p1.
			close(p1[1]);

			// Setting pipe p2.
			close(p2[0]);

			// Wait to receive a byte from parent.
			read(p1[0], buf, 1);
			pid = getpid();
			printf("%d: received ping\n", pid);
			
			// Send a byte to parent.
			write(p2[1], buf, 1);

			// Close the pipes
			close(p1[0]);
			close(p2[1]);
		} else {
			// Parent process
			
			// Setting pipe p1.
			close(p1[0]);

			// Setting pipe p2.
			close(p2[1]);

			// Send a byte to child.
			*buf = 'a';
			write(p1[1], buf, 1);

			// Wait to receive a byte from child.
			read(p2[0], buf, 1);
			pid = getpid();
			printf("%d: received pong\n", pid);

			// Close the pipes
			close(p1[1]);
			close(p2[0]);
		}
			
	} else {
		fprintf(2, "Error: pingpong doesn't require any arguments.\n");
	}
	exit(0);
}
