
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "micolisp.h"

int main (int argslen, char **args){
  FILE *input;
  if (1 < argslen){
    FILE *file = fopen(args[1], "r");
    if (file == NULL){ return 1; }
    input = file;
  }
  else {
    input = stdin;
  }
  micolisp_machine machine;
  if (micolisp_open(&machine) != 0){ return 1; }
  if (micolisp_load_library(&machine) != 0){ return 1; }
  if (micolisp_repl(input, stdout, stderr, &machine) != 0){ return 1; }
  if (micolisp_close(&machine) != 0){ return 1; }
  return 0;
}
