#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
	int tick_sleep = 0;
	if(argc == 2){
		tick_sleep = atoi(argv[1]);
		sleep(tick_sleep);
	} else {
		fprintf(2, "Error: sleep requires one argument.\n");
	}
	exit(0);
}
