#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
	int i;
	char *ptr;
	long int code;
	for(i = 0; i < 10; i++){
		ptr = sbrk(PGSIZE);
		memcpy(&code, ptr, 8);
		printf("%lx\n", code);
	}
}
