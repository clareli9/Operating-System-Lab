#include "common.h"
#include <stdio.h>
#include <string.h>

int
main(int argc, char**argv)
{
  int i;
  for (i = 1; i < argc; ++i){
    printf("%s\n", argv[i]);
  }
  /*
  char sentence[256];
  char *p;
  // fgets(sentence,sizeof(sentence),stdin);
  p = strtok(sentence," ");
  while (p != NULL){
    printf("%s\n",p);
    p = strtok(NULL," ");
    }*/
	 
  return 0;
}
