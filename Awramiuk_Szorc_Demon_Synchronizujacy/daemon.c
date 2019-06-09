#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include "functions.h"



bool sig=false;


void handler(int signum)
{
	sig=true; //changing flag
	syslog(LOG_INFO,"Daemon was awakened by SIGUSR1"); //writing info to syslog
}



int main(int argc,char* argv[])
{	
	openlog("Daemon",LOG_NDELAY | LOG_PID, LOG_NDELAY); //opening connection to system logger
  pid_t pid=0, sid=0;
  int c, i;	
	bool R=false;
	int Time = 300;
	long int S=1024;
	DIR *sourcedir,*targetdir;

  while((c = getopt (argc, argv, "RS:T:")) != -1) //getting arguments provided by user
	{
		switch(c)
		{
			case 'R':
				R = true;
				break;
			case 'T':
				for (i=0;i<strlen(optarg);i++)
				{
					if ((int)optarg[i]<48 || (int)optarg[i]>57)
					{
						perror("Your T argument isn't an integer\n");
						return 2;
					}
				}
				Time=atoi(optarg);
				break;
			case 'S':			
				for (i=0;i<strlen(optarg);i++)
				{
					if ((int)optarg[i]<48 || (int)optarg[i]>57)
					{
						perror("Your S argument isn't an integer\n");
						return 2;
					}
				}
				S = 1024 * atoi(optarg);
				break;
			default:
				return 1;
		}
	}
  
	sourcedir=opendir(argv[optind]);
	targetdir=opendir(argv[optind+1]);
	//checking if given paths lead to existing directories
	if (!sourcedir)
	{
		perror(argv[optind]);
		syslog(LOG_INFO,"Given directory does not exist");
		exit(EXIT_FAILURE);
	}
	if (!targetdir)
	{
		perror(argv[optind+1]);
		syslog(LOG_INFO,"Given directory does not exist");
		exit(EXIT_FAILURE);
	}
	closedir(sourcedir);
	closedir(targetdir);

  //making a daemon
  pid = fork();
  if (pid < 0) 
	{
		syslog(LOG_INFO,"Couldn't create a new process");
  	exit(1);
  }

  if (pid > 0) 
	{
		//printf("pid of child process %d \n", pid);
    exit(0);
  }
       
  umask(0);
                        
       
  sid = setsid();
  if (sid < 0) 
	{
		syslog(LOG_INFO,"Couldn't create a new session");
   	exit(1);
  }     
        
  close(STDIN_FILENO);
  //close(STDOUT_FILENO);
  close(STDERR_FILENO);
	


	signal(SIGUSR1, handler);	
  /* The Big Loop */
  while (1) 
	{
	  sleep(Time);
		if (sig==false) syslog(LOG_INFO,"Daemon was awakened normally");
		if (R == false)
		{
			searchSTT(argv[optind],argv[optind+1],S);
			searchTTS(argv[optind],argv[optind+1]);
		}
		else
		{
			searchSTTDIR(argv[optind],argv[optind+1],S);
			searchTTSDIR(argv[optind],argv[optind+1]);
		}	
		syslog(LOG_INFO,"Daemon entered sleeping mode");
		sig=false;
  }
  return 0;
}
