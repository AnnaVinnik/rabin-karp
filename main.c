#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include <err.h>
#include <sys/stat.h>
#include <unistd.h>

#define HASH_MUL 31
#define HASH_SIZE 71

#define WS_NONE		0
#define WS_RECURSIVE	(1 << 0)
#define WS_DEFAULT	WS_RECURSIVE
#define WS_FOLLOWLINK	(1 << 1)
#define WS_DOTFILES	(1 << 2)
#define WS_MATCHDIRS	(1 << 3)
enum {
	WALK_OK = 0,
	WALK_BADPATTERN,
	WALK_NAMETOOLONG,
	WALK_BADIO,
};

int proverka(char *string, char *search, int position, int sizeSearch)
{
     for(int i = 0; i < sizeSearch; i++){
         if (string[i + position] != search[i])
              return 1;
     }
     return 0;

}

int hash_kp(char *key, int i)
{
int j;
    unsigned int h = 0;
    char *p;

    for (p = key, j = 0; j < i; p++, j++){
        h = h * HASH_MUL + (unsigned int)*p;
    }
    return h%HASH_SIZE;
}

int hsub(char *search, char *nameFile)
{
FILE *file;
int hsub;
int i = 0, j = 0, n = 100, number;
char *string = (char*)malloc(sizeof(char) * n);
int position[10] = {-1};
    file = fopen(nameFile, "r");
    while (!feof(file)){
        string[i] = fgetc(file);
        i++;
        if (n <= i){
            n = n * 2;
            string = realloc(string, sizeof(char) * n);
        }
    }   
    int sizeString = i - 2;
    for (i = 0; search[i] != '\0'; i++);
    int sizeSearch = i;

    hsub = hash_kp(search, sizeSearch);
    for (i = 0, j = 0; i <= (sizeString - sizeSearch); i++){
        if (hash_kp(&string[i], sizeSearch) == hsub){
            if (proverka(string, search, i, sizeSearch) == 0){
                position[j] = i;
                j++;
            }
        }
    }

if (position[0] >= 0){
    for (i = 1; position[i] != 0; i++);
    number = i;
    printf("In the file \"%s\" found %d coincidences \n", nameFile, number);
}




    
fclose(file);
return 0;
}

int walk_recur(char *dname, regex_t *reg, int spec, char *obr)
{
	struct dirent *dent;
	DIR *dir;
	struct stat st;
	char fn[FILENAME_MAX];
	int res = WALK_OK;
	int len = strlen(dname);
	if (len >= FILENAME_MAX - 1)
		return WALK_NAMETOOLONG;
 
	strcpy(fn, dname);
	fn[len++] = '/';
 
	if (!(dir = opendir(dname)))
	{
		warn("can't open %s", dname);
		return WALK_BADIO;
	}
 
	errno = 0;
	while ((dent = readdir(dir)))
	{
		if (!(spec & WS_DOTFILES) && dent->d_name[0] == '.')
			continue;
		if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
			continue;
 
		strncpy(fn + len, dent->d_name, FILENAME_MAX - len);
		if (lstat(fn, &st) == -1)
		{
			warn("Can't stat %s", fn);
			res = WALK_BADIO;
			continue;
		}
 
		if (S_ISLNK(st.st_mode) && !(spec & WS_FOLLOWLINK))
			continue;
 
		if (S_ISDIR(st.st_mode))
		{
			if ((spec & WS_RECURSIVE))
				walk_recur(fn, reg, spec, obr);
 
			if (!(spec & WS_MATCHDIRS)) continue;
		}
		if (!regexec(reg, fn, 0, 0, 0)) 
		{
			hsub(obr, fn);
		}
	}
 
	if (dir) closedir(dir);
	return res ? res : errno ? WALK_BADIO : WALK_OK;
}

int walk_dir(char *dname, char *pattern, int spec, char *obr)
{
	regex_t r;
	int res;
	if (regcomp(&r, pattern, REG_EXTENDED | REG_NOSUB))
		return WALK_BADPATTERN;
	res = walk_recur(dname, &r, spec, obr);
	regfree(&r);
 
	return res;
}

int main(int argc, char *argv[])
{
    int r = walk_dir(argv[2], ".\\.txt$", WS_DEFAULT|WS_MATCHDIRS, argv[1]);
	switch(r)
	{
		case WALK_OK:		break;
		case WALK_BADIO:	err(1, "IO error");
		case WALK_BADPATTERN:	err(1, "Bad pattern");
		case WALK_NAMETOOLONG:	err(1, "Filename too long");
		default:
			err(1, "Unknown error?");
	}
    return 0;
}
