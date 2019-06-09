#ifndef FUNCTIONS_H
#define FUNCTIONS_H

void copybigfile(char* path1, char* path2);
void copyfile(char* path1, char* path2);
void modtime(struct stat sb1, char* targetpath);
char* getpath (char* path, char* filename);
void searchSTT (char* sourcepath, char* targetpath,long int S);
void searchSTTDIR (char* sourcepath, char* targetpath,long int S);
void searchTTS (char* sourcepath, char* targetpath);
void Remove(char* path);
void searchTTSDIR(char* sourcepath, char* targetpath);


#endif

