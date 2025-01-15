#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

// ----------------------------------------------
// input:
// char *path: String of path.
//
// output:
// char *: String after the last '/' in the path.
//
// It gets the path and retrun the pure file name
// in the path.
// ----------------------------------------------
char *
getFileName(char *path)
{
	char *p;
	for(p = path + strlen(path); p >= path && *p != '/'; p--)
		;
	p++;
	
	return p;
}

// ---------------------------------------------
// input: 
// char *path: Path of starting directory to
// 							find the files.
// char *fname: Name of file to search.
//
// It search every files that has the name of 
// fname and print their paths.
// ---------------------------------------------
void
findFile(char *path, char *fname)
{
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	// Open the file and check the type of the file.
	if((fd = open(path, O_RDONLY)) < 0){
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}
	if(fstat(fd, &st) < 0){
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}
	
	switch(st.type){
		// If it is a file, compare the name.
		case T_FILE:
			if(strcmp(getFileName(path), fname) == 0){
				printf("%s\n", path);
			}
			close(fd);
			break;
		// If it is a dierctory, apply findFile() to all
		// directory entry except the empty entries.
		case T_DIR:
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
				printf("find: path too long\n");
				break;
			}
			strcpy(buf, path);
			p = buf + strlen(buf);
			*p++ = '/';
			while(read(fd, &de, sizeof(de)) == sizeof(de)){
				if(de.inum == 0) // empty directory entry.
					continue;
				if(strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0){
					memmove(p, de.name, DIRSIZ);
					p[DIRSIZ] = 0;
					findFile(buf, fname); // Apply findFile() recurviely.
				}
			}
			close(fd);
			break;
		default:
			break;
	}
	return;
}


int
main(int argc, char *argv[])
{

	if(argc == 3){
		findFile(argv[1], argv[2]);
	} else {
		fprintf(2, "find: two arguments are required\n");
	}
}
