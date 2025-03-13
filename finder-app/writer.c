#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

int main(int argc, char *argv[]){
	openlog("Writer", LOG_PID | LOG_CONS, LOG_USER);
	if(argc < 3){
		syslog(LOG_ERR, "Please add argument <path> <content>");
		return 1;
	}
	char *writefile = argv[1];
	char *writestr = argv[2];

	int fd = open(writefile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	if(fd == -1)
	{
		syslog(LOG_ERR, "Cannot open file %s", writefile);
		return 1;
	}
	
	write(fd, writestr, strlen(writestr));

	close(fd);

	syslog(LOG_DEBUG, "Writing %s to %s", writestr, writefile);

	return 0;
}
