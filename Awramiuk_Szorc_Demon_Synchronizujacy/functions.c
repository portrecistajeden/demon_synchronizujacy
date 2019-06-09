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


clock_t start,end;


void copybigfile(char* path1, char* path2)
{
	char *addr;
	int sourcefile, targetfile;
	struct stat sb;
	size_t filesize;
	
	start=clock();


	sourcefile = open(path1, O_RDONLY); //opening source file
	if (sourcefile == -1)  
	{
		perror("Failed to open sourcefile\n");
		syslog(LOG_INFO,"Couldn't open %s file", path1);
		exit(EXIT_FAILURE);
	}
	if (stat(path1, &sb) == -1)
	{
		perror("Failed to use stat\n");
		syslog(LOG_INFO,"Couldn't use stat on %s file", path1);
		exit(EXIT_FAILURE);
	}
	targetfile = open(path2, O_WRONLY | O_CREAT | O_TRUNC, sb.st_mode); //opening target file
	if (targetfile == -1)
	{
		perror("Failed to open targetfile\n");
		syslog(LOG_INFO,"Couldn't open %s file", path2);
		exit(EXIT_FAILURE);
	}
	filesize = sb.st_size;	//getting size of file using stat
	char* data =(char*) mmap(NULL, filesize, PROT_READ, MAP_SHARED | MAP_FILE, sourcefile, 0); //mapping file
	if (data == MAP_FAILED)
	{
		perror("Failed to map data");
		syslog(LOG_INFO,"Couldn't map data from %s file", path1);
		exit(EXIT_FAILURE);
	}
	if (write(targetfile, data, filesize) == -1) //writing
	{
		perror("Failed to write data to file");
		syslog(LOG_INFO,"Couldn't write data to %s file", path2);
		exit(EXIT_FAILURE);
	}
	
	if(munmap(data, filesize) == -1) //deleting mapping
	{
		perror("Failed to delete mapping");
		syslog(LOG_INFO,"Couldn't delete mapping from %s file", path1);
		exit(EXIT_FAILURE);
	}

	close(sourcefile); //closing files
	close(targetfile);



	end=clock();
	syslog(LOG_INFO,"Time needed to copy the file using copybigfile: %f",((double)(end-start))/CLOCKS_PER_SEC);
}



void copyfile(char* path1, char* path2)
{
	char buffer[131072];
	size_t bytes_read=0;
	int i;
	struct stat sb;

	start=clock();



	int sourcefile = open(path1, O_RDONLY); //opening source file
	if (sourcefile == -1)
	{
		perror("Failed to open sourcefile\n");
		syslog(LOG_INFO,"Couldn't open %s file", path1);
		exit(EXIT_FAILURE);
	}
	if (stat(path1, &sb) == -1)
	{
		perror("Failed to use stat\n");
		syslog(LOG_INFO,"Couldn't use stat on %s file", path1);
		exit(EXIT_FAILURE);
	}
	int targetfile = open(path2, O_WRONLY | O_CREAT | O_TRUNC, sb.st_mode); //opening target file
	if (targetfile == -1)
	{
		perror("Failed to open targetfile\n");
		syslog(LOG_INFO,"Couldn't open %s file", path2);
		exit(EXIT_FAILURE);
	}
	
	//reading source file while it isn't empty
	while (bytes_read = read (sourcefile, buffer, sizeof(buffer)))
	{
		size_t bytes_written;
		//writing to target file while buffer isn't empty
		bytes_written = write(targetfile,buffer,bytes_read);
		if (bytes_written == -1)
		{
			perror("Failed to write data to file");
			syslog(LOG_INFO,"Couldn't write data to %s file", path2);
			exit(EXIT_FAILURE);
		} 
	}
	if (bytes_read == -1)
	{
		perror("Failed to read data from file");
		syslog(LOG_INFO, "Couldn't read data from %s file", path1);
		exit(EXIT_FAILURE);
	}
	close(sourcefile); //closing files
	close(targetfile);


	end=clock();
	syslog(LOG_INFO,"Time needed to copy the file using copyfile: %f",((double)(end-start))/CLOCKS_PER_SEC);
}



void modtime(struct stat sb1, char* targetpath)
{	
	struct utimbuf timebuf;
	timebuf.actime = sb1.st_atime;
	timebuf.modtime = sb1.st_mtime;
	if(utime(targetpath,&timebuf) != 0) 	//changing modification and access time
	{
		perror("utime() error");
		syslog(LOG_INFO, "Couldn't change modification date in %s file",targetpath);
		exit(EXIT_FAILURE);
	}
}


char* getpath (char* path, char* filename)
{
	char *temp=(char*)malloc(strlen(path)+strlen(filename)+2);
	if (temp == NULL)
	{
		perror("Failed to use malloc");
		syslog(LOG_INFO,"Failed to use malloc");
		exit(EXIT_FAILURE);
	}					
	strcpy(temp,path);
	strcat(temp,"/");
	strcat(temp,filename);
	temp[strlen(path)+strlen(filename)+1] = '\0';
	return temp; //returning path
}


void searchSTT (char* sourcepath, char* targetpath,long int S)
{
	DIR *sourcedir,*targetdir;
	struct dirent *dir1, *dir2;
	struct stat sb1, sb2;
	bool found=false;
	
	sourcedir=opendir(sourcepath); //opening source directory
	if(sourcedir) //able to open directory
	{
		while((dir1=readdir(sourcedir))!=NULL) //reading while source directory isn't empty
		{	
			if(dir1->d_type==DT_REG) //regular file
			{
				targetdir=opendir(targetpath); //opening target directory
				if(targetdir) //able to open directory
				{	
					while((dir2=readdir(targetdir))!=NULL) //reading while target directory isn't empty
					{
						if(dir2->d_type==DT_REG) //regular file
						{
							if(strcmp(dir1->d_name,dir2->d_name)==0) //found files with same name
							{		
								char* pomS = getpath(sourcepath,dir1->d_name);
								char* pomT = getpath(targetpath,dir2->d_name);

								if (lstat(pomS,&sb1) == -1)
								{
									perror("Failed to use lstat");
									syslog(LOG_INFO, "Failed to use lstat on %s file",pomS);
									exit(EXIT_FAILURE);
								}
								if (lstat(pomT,&sb2) == -1)
								{
									perror("Failed to use lstat");
									syslog(LOG_INFO, "Failed to use lstat on %s file",pomT);
									exit(EXIT_FAILURE);
								}

								if(sb1.st_mtime!=sb2.st_mtime) //different modification time in files
								{
									if(sb1.st_size>S) //size of file bigger than S
									{
										copybigfile(pomS,pomT); //copying file using mapping
										syslog(LOG_INFO,"Copied file %s to %s using mmap copying method",dir1->d_name,targetpath);
									}
									else 
									{
										copyfile(pomS,pomT); //copying file using read/write
										syslog(LOG_INFO,"Copied file %s to %s using read/write copying method",dir1->d_name,targetpath);
									}
									modtime(sb1,pomT);	//changing modification time
								}
								found=true;

								free(pomS);
								free(pomT);
								break;
							}
						}
					}
					if(found==false) //didn't find file in target directory with same name as in source directory
					{
						char *pomS = getpath(sourcepath,dir1->d_name);
						char *pomT = getpath(targetpath,dir1->d_name);

						if (lstat(pomS,&sb1) == -1)
						{
							perror("Failed to use lstat");
							syslog(LOG_INFO, "Failed to use lstat on %s file",pomS);
							exit(EXIT_FAILURE);
						}

						if(sb1.st_size>S) //size of file bigger than S
						{
							copybigfile(pomS,pomT); //copying file using mapping
							syslog(LOG_INFO,"Copied file %s to %s using mmap copying method",dir1->d_name,targetpath);
						}
						else 
						{
							copyfile(pomS,pomT); //copying file using read/write
			 				syslog(LOG_INFO,"Copied file %s to %s using read/write copying method",dir1->d_name,targetpath);
						}
						modtime(sb1,pomT); //changing modification time
						free(pomT);
						free(pomS);
					}
					found=false;
				  closedir(targetdir); //closing target directory
				}
				else //unable to open target directory
				{
					perror("The following error occurred");
					syslog(LOG_INFO,"Couldn't open target directory");
					exit(EXIT_FAILURE);
				}	
			}	
		}
	closedir(sourcedir); //closing source directory
	}
	else //unable to open source directory
	{
		perror("The following error occurred");
		syslog(LOG_INFO,"Couldn't open source directory");
		exit(EXIT_FAILURE);
	}
}

void searchSTTDIR (char*sourcepath, char* targetpath,long int S)
{
	DIR *sourcedir,*targetdir;
	struct dirent *dir1, *dir2;
	struct stat sb1, sb2;
	bool found = false;

	sourcedir=opendir(sourcepath); //opening source directory
	if(sourcedir) //able to open directory
	{
		while((dir1=readdir(sourcedir))!=NULL) //reading while source directory isn't empty
		{
			if((dir1->d_type==DT_REG || dir1->d_type==DT_DIR) && strcmp(dir1->d_name,".")!=0 && strcmp(dir1->d_name,"..")!=0 ) //regular file or directory different than ./..
			{
				targetdir=opendir(targetpath); //opening source directory
				if(targetdir) //able to open directory
				{
					while((dir2=readdir(targetdir))!=NULL) //reading while target directory isn't empty
					{
						if((dir2->d_type==DT_REG || dir2->d_type==DT_DIR) && strcmp(dir2->d_name,".")!=0 && strcmp(dir2->d_name,"..")!=0) //regular file or directory different than ./..
						{
							if(dir1->d_type==DT_DIR && dir2->d_type==DT_DIR && strcmp(dir1->d_name,dir2->d_name)==0) //found directories with same name
							{
								found=true;
								searchSTTDIR(getpath(sourcepath,dir1->d_name),getpath(targetpath,dir2->d_name),S); //calling another search using recursion
								break;
							}		
							if(dir1->d_type==DT_REG && dir2->d_type==DT_REG	&& strcmp(dir1->d_name,dir2->d_name)==0) // found files with same name
							{
								char* pomS = getpath(sourcepath,dir1->d_name);
								char* pomT = getpath(targetpath,dir2->d_name);

								if (lstat(pomS,&sb1) == -1)
								{
									perror("Failed to use lstat");
									syslog(LOG_INFO, "Failed to use lstat on %s file",pomS);
									exit(EXIT_FAILURE);
								}
								if (lstat(pomT,&sb2) == -1)
								{
									perror("Failed to use lstat");
									syslog(LOG_INFO, "Failed to use lstat on %s file",pomT);
									exit(EXIT_FAILURE);
								}

								if(sb1.st_mtime!=sb2.st_mtime)  //different modification time in files
								{
									if(sb1.st_size>S) //size of file bigger than S
									{
										copybigfile(pomS,pomT); //copying file using mapping
										syslog(LOG_INFO,"Copied file %s to %s using mmap copying method",dir1->d_name,targetpath);
									}
									else 
									{
										copyfile(pomS,pomT); //copying file using read/write
			 							syslog(LOG_INFO,"Copied file %s to %s using read/write copying method",dir1->d_name,targetpath);
									}
									modtime(sb1,pomT);	//changing modification time
								}
								found=true;

								free(pomS);
								free(pomT);
								break;
							}
						}					
					}
					if(found==false) //didn't find file or directory in target directory with same name as in source directory
					{		
						if (dir1->d_type==DT_DIR) //dir1 is a directory
						{
							char *pomS = getpath(sourcepath,dir1->d_name);
							char *pomT = getpath(targetpath,dir1->d_name);

							if (lstat(pomS,&sb1) == -1)
							{
								perror("Failed to use lstat");
								syslog(LOG_INFO, "Failed to use lstat on %s file",pomS);
								exit(EXIT_FAILURE);
							}
							if (mkdir(pomT, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) //creating new directory
							{
								perror("Failed to use mkdir");
								syslog(LOG_INFO, "Failed to make new directory in %s path",pomT);
								exit(EXIT_FAILURE);
							}
							syslog(LOG_INFO,"Made new directory %s",pomT);
							free(pomS); 
							free(pomT);
							searchSTTDIR(getpath(sourcepath,dir1->d_name),getpath(targetpath,dir1->d_name),S); //calling another search using recursion
						}	
						else //dir1 isn't a directory
						{
							char *pomS = getpath(sourcepath,dir1->d_name);
							char *pomT = getpath(targetpath,dir1->d_name);

							if (lstat(pomS,&sb1) == -1)
							{
								perror("Failed to use lstat");
								syslog(LOG_INFO, "Failed to use lstat on %s file",pomS);
								exit(EXIT_FAILURE);
							}
	
							if(sb1.st_size>S) //size of file bigger than S
							{
								copybigfile(pomS,pomT); //copying file using mapping
								syslog(LOG_INFO,"Copied file %s to %s using mmap copying method",dir1->d_name,targetpath);
							}
							else 
							{
								copyfile(pomS,pomT); //copying file using read/write
			 					syslog(LOG_INFO,"Copied file %s to %s using read/write copying method",dir1->d_name,targetpath);
							}
							modtime(sb1,pomT); //changing modification time
							free(pomT);
							free(pomS);
						}			
						
					}
					found=false;
				  closedir(targetdir); //closing target directory
				}
				else //unable to open target directory
				{
					perror("The following error occurred");
					syslog(LOG_INFO,"Couldn't open target directory");
					exit(EXIT_FAILURE);
				}
			}
		}
		closedir(sourcedir); //closing source directory
	}
	else //unable to open source directory
	{
		perror("The following error occurred");
		syslog(LOG_INFO,"Couldn't open docelowy directory");
		exit(EXIT_FAILURE);
	}
}

void searchTTS (char* sourcepath, char* targetpath)
{
	DIR *sourcedir,*targetdir;
	struct dirent *dir1, *dir2;
	bool found=false;
	int ret;
	targetdir=opendir(targetpath); //opening target directory
	if(targetdir) //able to open directory
	{
		while((dir1=readdir(targetdir))!=NULL) //reading while target directory isn't empty
		{
			if(dir1->d_type==DT_REG) // regular file
			{
				sourcedir=opendir(sourcepath); //opening source direcotry
				if(sourcedir) //able to open directory
				{	
					while((dir2=readdir(sourcedir))!=NULL) //reading while source directory isn't empty
					{
						if(dir2->d_type==DT_REG) // regular file
						{
							if(strcmp(dir1->d_name,dir2->d_name)==0) 
							{
								found=true; //found files with same name
								break;
							}
						}
					}
					if(found==false) //didn't find files with same name
					{
						char* pomT = getpath(targetpath,dir1->d_name);
						if(remove(pomT)==-1) //removing file from target directory
						{
							perror("The following error occurred");
							syslog(LOG_INFO,"Failed to remove file %s",dir1->d_name);
							exit(EXIT_FAILURE);
						}
						syslog(LOG_INFO,"Removed file %s",dir1->d_name);
						free(pomT);
					}
					found=false;

				}
				else //unable to open source directory
				{
					perror("The following error occurred");
					syslog(LOG_INFO,"Couldn't open source directory");
					exit(EXIT_FAILURE);
				}
				closedir(sourcedir); //closing source directory
			}
		}
	}
	else //unable to open target directory
	{
		perror("The following error occurred");
		syslog(LOG_INFO,"Couldn't open target directory");
		exit(EXIT_FAILURE);
	}
	closedir(targetdir); //closing target directory
}

void Remove(char* path)
{
	DIR *sourcedir;
	struct dirent *dir1;
	sourcedir=opendir(path); //opening directory
	if(!sourcedir) //unable to open a directory
	{
		perror("The following error occurred");
		syslog(LOG_INFO,"Failed to open directory %s", path);
		exit(EXIT_FAILURE);
	}
	while((dir1=readdir(sourcedir))!=NULL) //reading while directory isn't empty
	{
		if(dir1->d_type==DT_DIR && strcmp(dir1->d_name,".")!=0 && strcmp(dir1->d_name,"..")!=0) //found a directory that isn't ./..
		{
			Remove(getpath(path,dir1->d_name)); //calling another remove using recursion
			if(rmdir(getpath(path,dir1->d_name))==-1) //removing directory
			{
				perror("The following error occurred");
				syslog(LOG_INFO,"Failed to remove directory %s", dir1->d_name);
				exit(EXIT_FAILURE);
			}
			syslog(LOG_INFO,"Removed directory %s", dir1->d_name);
		}
		else if(strcmp(dir1->d_name,".")!=0 && strcmp(dir1->d_name,"..")!=0) //not a directory
		{
			char* pomT = getpath(path,dir1->d_name);
			if(remove(pomT)==-1) //removing file
			{
				perror("The following error occurred");
				syslog(LOG_INFO,"Failed to remove file %s",dir1->d_name);
				exit(EXIT_FAILURE);
			}
			syslog(LOG_INFO,"Removed file %s", dir1->d_name);
			free(pomT);
		}
	}
}

void searchTTSDIR(char* sourcepath, char* targetpath)
{
	DIR *sourcedir,*targetdir;
	struct dirent *dir1, *dir2;
	bool found=false;
	int ret;

	targetdir=opendir(targetpath); //opening target directory
	if(targetdir) //able to open directory
	{
		while((dir1=readdir(targetdir))!=NULL) //reading while target directory isn't empty
		{
			if((dir1->d_type==DT_DIR || dir1->d_type==DT_REG) && strcmp(dir1->d_name,".")!=0 && strcmp(dir1->d_name,"..")!=0 ) //regular file or directory different than ./..
			{
				sourcedir=opendir(sourcepath); //opening source directory
				if(sourcedir) //able to open directory
				{
					while((dir2=readdir(sourcedir))!=NULL) //reading while source directory isn't empty
					{
						if((dir2->d_type==DT_REG || dir2->d_type==DT_DIR) && strcmp(dir2->d_name,".")!=0 && strcmp(dir2->d_name,"..")!=0) //regular file or directory different than ./..
						{
							if(dir1->d_type==DT_DIR && dir2->d_type==DT_DIR && strcmp(dir1->d_name,dir2->d_name)==0) //found directories with same name
							{
								found = true;
								searchTTSDIR(getpath(sourcepath,dir1->d_name),getpath(targetpath,dir2->d_name)); //calling another search using recursion
								break;
							}
							if(dir1->d_type==DT_REG && dir2->d_type==DT_REG	&& strcmp(dir1->d_name,dir2->d_name)==0) //found files with same name
							{
								found = true;
								break;
							}
						}
					}
					if(found==false) //didn't find file nor directory in source directory
					{
						if (dir1->d_type==DT_DIR) //dir1 is a directory
						{
							Remove(getpath(targetpath,dir1->d_name)); //calling remove function which will delete everything inside that directory
							if(rmdir(getpath(targetpath,dir1->d_name))==-1) //removing directory
							{
								perror("The following error occurred");
								syslog(LOG_INFO,"Failed to remove directory %s", dir1->d_name);
								exit(EXIT_FAILURE);
							}
							syslog(LOG_INFO,"Removed directory %s", dir1->d_name);
						}
						else //not a directory
						{
							char* pomT = getpath(targetpath,dir1->d_name);
							if(remove(pomT)==-1) //removing file
							{
								perror("The following error occurred");
								syslog(LOG_INFO,"Failed to remove file %s",dir1->d_name);
								exit(EXIT_FAILURE);
							}
							free(pomT);
							syslog(LOG_INFO,"Removed file %s",dir1->d_name);
						}
					}
					found=false;
				}
				else //unable to open source directory
				{
					perror("The following error occurred");
					syslog(LOG_INFO,"Couldn't open source directory");
					exit(EXIT_FAILURE);
				}
				closedir(sourcedir);
			}
		}
		
	}
	else //unable to open target directory
	{
		perror("The following error occurred");
		syslog(LOG_INFO,"Couldn't open target directory");
		exit(EXIT_FAILURE);
	}
	closedir(targetdir);
}




