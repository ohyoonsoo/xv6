#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	char *arg[argc-2];
	int i;

	for(i = 0; i < argc-2; i++){
		arg[i] = argv[i+2];
	}

	if(argc > 2){
		if(fork() == 0){
			trace(atoi(argv[1]));
			for(i = 0; i < argc - 2; i++){
				printf("%d %s\n", i, arg[i]);
			}
			exec(arg[0], arg);
		} else {
			wait(0);
		}
	} else {
		fprintf(2, "trace: needs more arguments.\n");
	}

	exit(0);
}
