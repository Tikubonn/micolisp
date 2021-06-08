
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mlisp.h"

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
  mlisp_machine machine;
  if (mlisp_open(&machine) != 0){ return 1; }
  if (mlisp_load_library(&machine) != 0){ return 1; }
  if (mlisp_repl(input, stdout, stderr, &machine) != 0){ return 1; }
  if (mlisp_close(&machine) != 0){ return 1; }
  return 0;
}
