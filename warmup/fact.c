#include "common.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
int fact(int n);
int main(int argc, char**argv)
{
  int i;
  int result = 0;
  for (i = 0; i < strlen(argv[1]); i++){
    if (!isdigit(argv[1][i])){
      printf("Huh?\n");
      return 0;
    }
  }


  for (i = 0; i < strlen(argv[1]); i++){
    result = result*10 + (argv[1][i]-'0');
    
  }
  if (result <= 0){
    printf("Huh?\n");
  }
  else  if (result > 12){
    printf("Overflow\n");
  }
  else {
    printf("%d\n",fact(result));
  }
  
  return 0; 
    
}

int fact(int n){
  if (n == 0)
    return 1;
  else
    return n*fact(n-1);
    
}
