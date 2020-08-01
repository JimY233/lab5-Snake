#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/******************************************************

  Usage:
    ./ktimer [flag] [message]

	-l
    List to stdout expiration time in seconds and message.
	-s [SEC] [MSG]
    Register new timer after [SEC] seconds will print
    message [MSG]. If active timer with same message
    exists, reset timer to [SEC].
        -r
    Remove the timer

  Examples:
	./ktimer -l
		Print whatever message that the ktimer module is holding
        ./ktimer -s 5 hello
		Print message when time expires
        ./ktimer -r
		Remove the timer

******************************************************/
void sighandler(int);
void printManPage(void);
char message[256];

int main(int argc, char **argv) {
	char line[256];
	int count = 1, i = 0;
	char *flag;
	char *msg;
	char *sec;
	char *limiter = ";";
	char *cp_line;

	/* //Check to see if the ktimer successfully has mknod run;  Assumes that mytimer is tied to /dev/mytimer
	FILE * pFile;
	pFile = fopen("/dev/mytimer", "r+");
	if (pFile==NULL) {
		fputs("mytimer module isn't loaded\n",stderr);
		return -1;
	}
	*/

	int pFile, oflags;
	struct sigaction action, oa;

        extern int errno;
	unsigned long seconds;

	// Opens to device file
	pFile = open("/dev/mytimer", O_RDWR);
	if (pFile < 0) {
		fprintf (stderr, "mytimer module isn't loaded\n");
		return 1;
	}

	// Check if list time
	if (argc == 2 && strcmp(argv[1], "-l") == 0) {

	  while(read(pFile, &line, sizeof(line))!=0){
	    printf("%s", line);
	  }
	  
	}

	// Check if register new timer
	else if (argc == 4 && strcmp(argv[1], "-s") == 0) {

	  // Setup signal handler
	  memset(&action, 0, sizeof(action));
	  action.sa_handler = sighandler;
	  action.sa_flags = SA_SIGINFO;
	  sigemptyset(&action.sa_mask);
	  sigaction(SIGIO, &action, NULL);
	  fcntl(pFile, F_SETOWN, getpid());
	  oflags = fcntl(pFile, F_GETFL);
	  fcntl(pFile, F_SETFL, oflags | FASYNC);

	  strcpy(line, argv[1]); // -s;hello;20
	  strcat(line, ";");
	  strcat(line, argv[3]);
	  strcat(line, ";");
	  strcat(line, argv[2]);

	  strcpy(message,argv[3]);
	  sscanf(argv[2], "%lu", &seconds);
          
          //printf("%s\n",line);
	  write(pFile,line,sizeof(line));


          if(errno == 17){
	    printf("A timer exists already\n");
	    close(pFile);
	    return 0;
	  }
          else if(errno == 16){
        printf("Timer %s has now been reset to %d seconds\n",message,seconds);
	    close(pFile);
	    return 0;
	  }
	  else{
	    // Waits.
	    //printf("Sleep!\n");
	    pause();

	    //printf("Closing out!\n");
	    printf("%s\n",message);
	  }

	}

	else if (argc == 2 && strcmp(argv[1], "-r") == 0) {
          strcpy(line, "-r"); // -s;hello;20
          
	  write(pFile,line,sizeof(line));

	}

	// Otherwise invalid
	else {
		printManPage();
	}

	close(pFile);
	return 0;
}

void printManPage() {
	printf("Error: invalid use.\n");
	printf(" ktimer [-flag] [message]\n");
	printf(" -l: List to stdout expiration time in seconds and message\n");
	printf(" -s [SEC] [MSG]: Start timer after [SEC] will print [MSG]\n");
	printf("                 If active timer exists, reset to [SEC]\n");
        printf(" -r: Remove the timer\n");
}

// SIGIO handler
void sighandler(int signo)
{
  //printf("%s\n",message);
}
