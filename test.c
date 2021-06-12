
#include <stdio.h>
#include <stdlib.h>
#include "micolisp.h"

#define TEST(form)\
if (form){\
printf("success: %s at %s:%d\n", #form, __FILE__, __LINE__);\
}\
else {\
printf("error: %s at %s:%d\n", #form, __FILE__, __LINE__); abort();\
}

static void test_micolisp_allocate (){
  micolisp_machine machine;
  TEST(micolisp_open(&machine) == 0);
  // allocate number.
  {
    micolisp_number *number1 = micolisp_allocate_number(&machine);
    micolisp_number *number2 = micolisp_allocate_number(&machine);
    micolisp_number *number3 = micolisp_allocate_number(&machine);
    TEST(number1 != NULL);
    TEST(number2 != NULL);
    TEST(number3 != NULL);
    *number1 = 1;
    *number2 = 2;
    *number3 = 3;
    TEST(*number1 == 1);
    TEST(*number2 == 2);
    TEST(*number3 == 3);
    TEST(micolisp_decrease(number1, &machine) == 0);
    TEST(micolisp_decrease(number2, &machine) == 0);
    TEST(micolisp_decrease(number3, &machine) == 0);
  }
  // allocate symbol.
  {
    micolisp_symbol *syma = micolisp_allocate_symbol0("a", &machine);
    micolisp_symbol *symb = micolisp_allocate_symbol0("b", &machine);
    micolisp_symbol *symc = micolisp_allocate_symbol0("c", &machine);
    micolisp_symbol *syma2 = micolisp_allocate_symbol0("a", &machine);
    micolisp_symbol *symb2 = micolisp_allocate_symbol0("b", &machine);
    micolisp_symbol *symc2 = micolisp_allocate_symbol0("c", &machine);
    TEST(syma != NULL);
    TEST(symb != NULL);
    TEST(symc != NULL);
    TEST(syma != symb);
    TEST(symb != symc);
    TEST(syma == syma2);
    TEST(symb == symb2);
    TEST(symc == symc2);
    TEST(micolisp_decrease(syma, &machine) == 0);
    TEST(micolisp_decrease(symb, &machine) == 0);
    TEST(micolisp_decrease(symc, &machine) == 0);
    TEST(micolisp_decrease(syma2, &machine) == 0);
    TEST(micolisp_decrease(symb2, &machine) == 0);
    TEST(micolisp_decrease(symc2, &machine) == 0);
  }
  // allocate c function.
  {
    micolisp_c_function *function = micolisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
    TEST(function != NULL);
    TEST(micolisp_decrease(function, &machine) == 0);
  }
  // allocate user function.
  {
    micolisp_user_function *function = micolisp_allocate_user_function(MLISP_FUNCTION, NULL, NULL, &machine);
    TEST(function != NULL);
    TEST(micolisp_decrease(function, &machine) == 0);
  }
  // allocate cons.
  {
    micolisp_cons *cons1 = micolisp_allocate_cons(NULL, NULL, &machine);
    micolisp_cons *cons2 = micolisp_allocate_cons(NULL, NULL, &machine);
    micolisp_cons *cons3 = micolisp_allocate_cons(NULL, NULL, &machine);
    TEST(cons1 != NULL);
    TEST(cons2 != NULL);
    TEST(cons3 != NULL);
    TEST(cons1 != cons2);
    TEST(cons2 != cons3);
    TEST(micolisp_decrease(cons1, &machine) == 0);
    TEST(micolisp_decrease(cons2, &machine) == 0);
    TEST(micolisp_decrease(cons3, &machine) == 0);
  }
  // close machine.
  TEST(micolisp_close(&machine) == 0);
}

static void test_micolisp_scope_set (){
  // set function.
  {
    micolisp_machine machine;
    TEST(micolisp_open(&machine) == 0);
    // set function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("a", &machine);
      micolisp_c_function *function = micolisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symbol != NULL);
      TEST(function != NULL);
      TEST(micolisp_scope_set(function, symbol, &machine) == 0);
      TEST(micolisp_decrease(symbol, &machine) == 0);
      TEST(micolisp_decrease(function, &machine) == 0);
    }
    // set function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("b", &machine);
      micolisp_c_function *function = micolisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symbol != NULL);
      TEST(function != NULL);
      TEST(micolisp_scope_set(function, symbol, &machine) == 0);
      TEST(micolisp_decrease(symbol, &machine) == 0);
      TEST(micolisp_decrease(function, &machine) == 0);
    }
    // set function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("c", &machine);
      micolisp_c_function *function = micolisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symbol != NULL);
      TEST(function != NULL);
      TEST(micolisp_scope_set(function, symbol, &machine) == 0);
      TEST(micolisp_decrease(symbol, &machine) == 0);
      TEST(micolisp_decrease(function, &machine) == 0);
    }
    // get function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("a", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(micolisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
    }
    // get function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("b", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(micolisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
    }
    // get function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("c", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(micolisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
    }
    // get function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("d", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(micolisp_scope_get(symbol, &machine, &function) != 0); //always error!
    }
    // close machine.
    TEST(micolisp_close(&machine) == 0);
  }
  // set function.
  {
    micolisp_machine machine;
    TEST(micolisp_open(&machine) == 0);
    // set function.
    {
      micolisp_symbol *syma = micolisp_allocate_symbol0("a", &machine);
      micolisp_c_function *functiona = micolisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(syma != NULL);
      TEST(functiona != NULL);
      TEST(micolisp_scope_set(functiona, syma, &machine) == 0);
      TEST(micolisp_decrease(syma, &machine) == 0);
      TEST(micolisp_decrease(functiona, &machine) == 0);
      micolisp_symbol *symb = micolisp_allocate_symbol0("b", &machine);
      micolisp_c_function *functionb = micolisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symb != NULL);
      TEST(functionb != NULL);
      TEST((void*)symb - (void*)syma == sizeof(micolisp_symbol));
      TEST(micolisp_scope_set(functionb, symb, &machine) == 0);
      TEST(micolisp_decrease(symb, &machine) == 0);
      TEST(micolisp_decrease(functionb, &machine) == 0);
      micolisp_symbol *symc = micolisp_allocate_symbol0("c", &machine);
      micolisp_c_function *functionc = micolisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symc != NULL);
      TEST(functionc != NULL);
      TEST((void*)symc - (void*)symb == sizeof(micolisp_symbol));
      TEST(micolisp_scope_set(functionc, symc, &machine) == 0);
      TEST(micolisp_decrease(symc, &machine) == 0);
      TEST(micolisp_decrease(functionc, &machine) == 0);
    }
    // get function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("a", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(micolisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
      TEST(micolisp_decrease(symbol, &machine) == 0);
    }
    // get function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("b", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(micolisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
      TEST(micolisp_decrease(symbol, &machine) == 0);
    }
    // get function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("c", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(micolisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
      TEST(micolisp_decrease(symbol, &machine) == 0);
    }
    // get function.
    {
      micolisp_symbol *symbol = micolisp_allocate_symbol0("d", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(micolisp_scope_get(symbol, &machine, &function) != 0); //always error!
      TEST(micolisp_decrease(symbol, &machine) == 0);
    }
    // close machine.
    TEST(micolisp_close(&machine) == 0);
  }
}

static void test_micolisp_read (){
  micolisp_machine machine;
  TEST(micolisp_open(&machine) == 0);
  // read from test/boolean.lisp 
  {
    FILE *file = fopen("test/boolean.lisp", "r");
    TEST(file != NULL);
    void *value;
    // read t. 
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(value == MLISP_T);
    TEST(micolisp_decrease(value, &machine) == 0);
    // read nil.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(value == MLISP_NIL);
    TEST(micolisp_decrease(value, &machine) == 0);
    // read eof.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_EOF);
    TEST(fclose(file) == 0);
  }
  // read from test/number.lisp 
  {
    FILE *file = fopen("test/number.lisp", "r");
    TEST(file != NULL);
    void *value;
    // read 1 
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_NUMBER, value, &machine) == true);
    TEST(*(micolisp_number*)value == 1.0);
    TEST(micolisp_decrease(value, &machine) == 0);
    // read 2.5
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_NUMBER, value, &machine) == true);
    TEST(*(micolisp_number*)value == 2.5);
    TEST(micolisp_decrease(value, &machine) == 0);
    // read -3
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_NUMBER, value, &machine) == true);
    TEST(*(micolisp_number*)value == -3.0);
    TEST(micolisp_decrease(value, &machine) == 0);
    // read -4.5
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_NUMBER, value, &machine) == true);
    TEST(*(micolisp_number*)value == -4.5);
    TEST(micolisp_decrease(value, &machine) == 0);
    // read eof.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_EOF);
    TEST(fclose(file) == 0);
  }
  // read from test/symbol.lisp
  {
    FILE *file = fopen("test/symbol.lisp", "r");
    TEST(file != NULL);
    void *value;
    // must be abc.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_SYMBOL, value, &machine) == true);
    micolisp_symbol *symabc = micolisp_allocate_symbol0("abc", &machine);
    TEST(symabc != NULL);
    TEST(symabc == value);
    TEST(micolisp_decrease(value, &machine) == 0);
    TEST(micolisp_decrease(symabc, &machine) == 0);
    // must be def.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_SYMBOL, value, &machine) == true);
    micolisp_symbol *symdef = micolisp_allocate_symbol0("def", &machine);
    TEST(symdef != NULL);
    TEST(symdef == value);
    TEST(micolisp_decrease(value, &machine) == 0);
    TEST(micolisp_decrease(symdef, &machine) == 0);
    // must be ghi.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_SYMBOL, value, &machine) == true);
    micolisp_symbol *symghi = micolisp_allocate_symbol0("ghi", &machine);
    TEST(symghi != NULL);
    TEST(symghi == value);
    TEST(micolisp_decrease(value, &machine) == 0);
    TEST(micolisp_decrease(symghi, &machine) == 0);
    // must be +.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_SYMBOL, value, &machine) == true);
    micolisp_symbol *symplus = micolisp_allocate_symbol0("+", &machine);
    TEST(symplus != NULL);
    TEST(symplus == value);
    TEST(micolisp_decrease(value, &machine) == 0);
    TEST(micolisp_decrease(symplus, &machine) == 0);
    // must be -.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(micolisp_typep(MLISP_SYMBOL, value, &machine) == true);
    micolisp_symbol *symminus = micolisp_allocate_symbol0("-", &machine);
    TEST(symminus != NULL);
    TEST(symminus == value);
    TEST(micolisp_decrease(value, &machine) == 0);
    TEST(micolisp_decrease(symminus, &machine) == 0);
    // read eof.
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_EOF);
    // close file.
    TEST(fclose(file) == 0);
  }
  // read from test/cons.lisp
  {
    FILE *file = fopen("test/cons.lisp", "r");
    TEST(file != NULL);
    // must be (1 2 3)
    {
      void *value;
      TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(micolisp_typep(MLISP_CONS, value, &machine) == true);
      micolisp_cons *cons1 = value;
      TEST(micolisp_typep(MLISP_NUMBER, cons1->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons1->car) == 1.0);
      micolisp_cons *cons2 = cons1->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons2->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons2->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons2->car) == 2.0);
      micolisp_cons *cons3 = cons2->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons3->car, &machine) == true);
      TEST(cons3->cdr == NULL);
      TEST(*(micolisp_number*)(cons3->car) == 3.0);
      TEST(micolisp_decrease(value, &machine) == 0);
    }
    // must be (1 (2 (3)))
    {
      // (1 (2 (3)))
      void *value;
      TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(micolisp_typep(MLISP_CONS, value, &machine) == true);
      // (1 (2 (3)))
      micolisp_cons *cons1 = value;
      TEST(micolisp_typep(MLISP_NUMBER, cons1->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons1->car) == 1.0);
      // (2 (3))
      micolisp_cons *cons2 = cons1->cdr;
      TEST(micolisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (2 (3))
      micolisp_cons *cons21 = cons2->car;
      TEST(micolisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons21->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons21->car) == 2.0);
      // (3)
      micolisp_cons *cons22 = cons21->cdr;
      TEST(micolisp_typep(MLISP_CONS, cons22->car, &machine) == true);
      TEST(cons22->cdr == NULL);
      // (3)
      micolisp_cons *cons221 = cons22->car;
      TEST(micolisp_typep(MLISP_NUMBER, cons221->car, &machine) == true);
      TEST(cons221->cdr == NULL);
      TEST(*(micolisp_number*)(cons221->car) == 3.0);
    }
    // must be (1 2 . 3)
    {
      // (1 2 . 3)
      void *value;
      TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(micolisp_typep(MLISP_CONS, value, &machine) == true);
      // (1 2 . 3)
      micolisp_cons *cons1 = value;
      TEST(micolisp_typep(MLISP_NUMBER, cons1->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons1->car) == 1.0);
      // (2 . 3)
      micolisp_cons *cons2 = cons1->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons2->car, &machine) == true);
      TEST(micolisp_typep(MLISP_NUMBER, cons2->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons2->car) == 2.0);
      TEST(*(micolisp_number*)(cons2->cdr) == 3.0);
      TEST(micolisp_decrease(value, &machine) == 0);
    }
    // must be (1 (2 . 3))
    {
      // (1 (2 . 3))
      void *value;
      TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(micolisp_typep(MLISP_CONS, value, &machine) == true);
      // (1 (2 . 3))
      micolisp_cons *cons1 = value;
      TEST(micolisp_typep(MLISP_NUMBER, cons1->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons1->car) == 1.0);
      // (2 . 3)
      micolisp_cons *cons2 = cons1->cdr;
      TEST(micolisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (2 . 3)
      micolisp_cons *cons21 = cons2->car;
      TEST(micolisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(micolisp_typep(MLISP_NUMBER, cons21->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons21->car) == 2.0);
      TEST(*(micolisp_number*)(cons21->cdr) == 3.0);
      TEST(micolisp_decrease(value, &machine) == 0);
    }
    // read eof.
    void *value;
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_EOF);
    // close file.
    TEST(fclose(file) == 0);
  }
  // read from test/string.lisp 
  {
    FILE *file = fopen("test/string.lisp", "r");
    TEST(file != NULL);
    // must be (quote (97 98 99))
    {
      // (quote (97 98 99))
      void *value;
      TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(micolisp_typep(MLISP_CONS, value, &machine) == true);
      micolisp_cons *cons1 = value;
      TEST(micolisp_typep(MLISP_SYMBOL, cons1->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      // ((97 98 99))
      micolisp_cons *cons2 = cons1->cdr;
      TEST(micolisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (97 98 99)
      micolisp_cons *cons21 = cons2->car;
      TEST(micolisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons21->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons21->car) == 97);
      // (98 99)
      micolisp_cons *cons22 = cons21->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons22->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons22->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons22->car) == 98);
      // (99)
      micolisp_cons *cons23 = cons22->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons23->car, &machine) == true);
      TEST(cons23->cdr == NULL);
      TEST(*(micolisp_number*)(cons23->car) == 99);
      TEST(micolisp_decrease(value, &machine) == 0);
    }
    // must be (quote (100 101 102))
    {
      // (quote (100 101 102))
      void *value;
      TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(micolisp_typep(MLISP_CONS, value, &machine) == true);
      micolisp_cons *cons1 = value;
      TEST(micolisp_typep(MLISP_SYMBOL, cons1->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      // ((100 101 102))
      micolisp_cons *cons2 = cons1->cdr;
      TEST(micolisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (100 101 102)
      micolisp_cons *cons21 = cons2->car;
      TEST(micolisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons21->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons21->car) == 100);
      // (101 102)
      micolisp_cons *cons22 = cons21->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons22->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons22->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons22->car) == 101);
      // (102)
      micolisp_cons *cons23 = cons22->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons23->car, &machine) == true);
      TEST(cons23->cdr == NULL);
      TEST(*(micolisp_number*)(cons23->car) == 102);
      TEST(micolisp_decrease(value, &machine) == 0);
    }
    // must be (quote (103 104 105))
    {
      // (quote (103 104 105))
      void *value;
      TEST(micolisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(micolisp_typep(MLISP_CONS, value, &machine) == true);
      micolisp_cons *cons1 = value;
      TEST(micolisp_typep(MLISP_SYMBOL, cons1->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      // ((103 104 105))
      micolisp_cons *cons2 = cons1->cdr;
      TEST(micolisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (103 104 105)
      micolisp_cons *cons21 = cons2->car;
      TEST(micolisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons21->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons21->car) == 103);
      // (104 105)
      micolisp_cons *cons22 = cons21->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons22->car, &machine) == true);
      TEST(micolisp_typep(MLISP_CONS, cons22->cdr, &machine) == true);
      TEST(*(micolisp_number*)(cons22->car) == 104);
      // (105)
      micolisp_cons *cons23 = cons22->cdr;
      TEST(micolisp_typep(MLISP_NUMBER, cons23->car, &machine) == true);
      TEST(cons23->cdr == NULL);
      TEST(*(micolisp_number*)(cons23->car) == 105);
      TEST(micolisp_decrease(value, &machine) == 0);
    }
    // read eof.
    void *value;
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_EOF);
    // close file.
    TEST(fclose(file) == 0);
  }
  // read from test/comment.lisp
  {
    FILE *file = fopen("test/comment.lisp", "r");
    TEST(file != NULL);
    void *value;
    TEST(micolisp_read(file, &machine, &value) == MLISP_READ_EOF);
    TEST(fclose(file) == 0);
  }
  TEST(micolisp_close(&machine) == 0);
}

static void test_micolisp_print (){
  micolisp_machine machine;
  TEST(micolisp_open(&machine) == 0);
  // print 0.
  {
    // write to test/number.txt
    {
      FILE *file = fopen("test/number.txt", "w");
      TEST(file != NULL);
      micolisp_number *number = micolisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = 0;
      TEST(micolisp_print(number, file, &machine) == 0);
      TEST(micolisp_decrease(number, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/number.txt
    {
      FILE *file = fopen("test/number.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("0", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print 1.
  {
    // write to test/number2.txt
    {
      FILE *file = fopen("test/number2.txt", "w");
      TEST(file != NULL);
      micolisp_number *number = micolisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = 1;
      TEST(micolisp_print(number, file, &machine) == 0);
      TEST(micolisp_decrease(number, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/number2.txt
    {
      FILE *file = fopen("test/number2.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("1", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print 1.5.
  {
    // write to test/number3.txt
    {
      FILE *file = fopen("test/number3.txt", "w");
      TEST(file != NULL);
      micolisp_number *number = micolisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = 1.5;
      TEST(micolisp_print(number, file, &machine) == 0);
      TEST(micolisp_decrease(number, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/number3.txt
    {
      FILE *file = fopen("test/number3.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("1.5", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print -1.
  {
    // write to test/number4.txt
    {
      FILE *file = fopen("test/number4.txt", "w");
      TEST(file != NULL);
      micolisp_number *number = micolisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = -1;
      TEST(micolisp_print(number, file, &machine) == 0);
      TEST(micolisp_decrease(number, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/number4.txt
    {
      FILE *file = fopen("test/number4.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("-1", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print -1.5.
  {
    // write to test/number5.txt
    {
      FILE *file = fopen("test/number5.txt", "w");
      TEST(file != NULL);
      micolisp_number *number = micolisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = -1.5;
      TEST(micolisp_print(number, file, &machine) == 0);
      TEST(micolisp_decrease(number, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/number5.txt
    {
      FILE *file = fopen("test/number5.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("-1.5", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print abc.
  {
    // write to test/symbol.txt
    {
      FILE *file = fopen("test/symbol.txt", "w");
      TEST(file != NULL);
      micolisp_symbol *symbol = micolisp_allocate_symbol0("abc", &machine);
      TEST(symbol != NULL);
      TEST(micolisp_print(symbol, file, &machine) == 0);
      TEST(micolisp_decrease(symbol, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/symbol.txt
    {
      FILE *file = fopen("test/symbol.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("abc", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print (1 2 3).
  {
    // write to test/cons.txt
    {
      FILE *file = fopen("test/cons.txt", "w");
      TEST(file != NULL);
      // (3)
      micolisp_number *number3 = micolisp_allocate_number(&machine);
      TEST(number3 != NULL);
      *number3 = 3;
      micolisp_cons *cons3 = micolisp_allocate_cons(number3, NULL, &machine);
      TEST(cons3 != NULL);
      // (2 3)
      micolisp_number *number2 = micolisp_allocate_number(&machine);
      TEST(number2 != NULL);
      *number2 = 2;
      micolisp_cons *cons2 = micolisp_allocate_cons(number2, cons3, &machine);
      TEST(cons2 != NULL);
      // (1 2 3)
      micolisp_number *number1 = micolisp_allocate_number(&machine);
      TEST(number1 != NULL);
      *number1 = 1;
      micolisp_cons *cons1 = micolisp_allocate_cons(number1, cons2, &machine);
      TEST(cons1 != NULL);
      TEST(micolisp_print(cons1, file, &machine) == 0);
      TEST(micolisp_decrease(cons1, &machine) == 0);
      TEST(micolisp_decrease(cons2, &machine) == 0);
      TEST(micolisp_decrease(cons3, &machine) == 0);
      TEST(micolisp_decrease(number1, &machine) == 0);
      TEST(micolisp_decrease(number2, &machine) == 0);
      TEST(micolisp_decrease(number3, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/cons.txt
    {
      FILE *file = fopen("test/cons.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("(1 2 3)", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print (1 (2 (3)))
  {
    // write to test/cons2.txt
    {
      FILE *file = fopen("test/cons2.txt", "w");
      // (3)
      micolisp_number *number3 = micolisp_allocate_number(&machine);
      TEST(number3 != NULL);
      *number3 = 3;
      micolisp_cons *cons3 = micolisp_allocate_cons(number3, NULL, &machine);
      TEST(cons3 != NULL);
      // ((3))
      micolisp_cons *cons21 = micolisp_allocate_cons(cons3, NULL, &machine);
      TEST(cons21 != NULL);
      // (2 (3))
      micolisp_number *number2 = micolisp_allocate_number(&machine);
      TEST(number2 != NULL);
      *number2 = 2;
      micolisp_cons *cons2 = micolisp_allocate_cons(number2, cons21, &machine);
      TEST(cons2 != NULL);
      // ((2 (3)))
      micolisp_cons *cons11 = micolisp_allocate_cons(cons2, NULL, &machine);
      TEST(cons11 != NULL);
      micolisp_number *number1 = micolisp_allocate_number(&machine);
      TEST(number1 != NULL);
      *number1 = 1;
      // (1 (2 (3)))
      micolisp_cons *cons1 = micolisp_allocate_cons(number1, cons11, &machine);
      TEST(cons1 != NULL);
      TEST(micolisp_print(cons1, file, &machine) == 0);
      TEST(micolisp_decrease(cons1, &machine) == 0);
      TEST(micolisp_decrease(cons11, &machine) == 0);
      TEST(micolisp_decrease(cons2, &machine) == 0);
      TEST(micolisp_decrease(cons21, &machine) == 0);
      TEST(micolisp_decrease(cons3, &machine) == 0);
      TEST(micolisp_decrease(number1, &machine) == 0);
      TEST(micolisp_decrease(number2, &machine) == 0);
      TEST(micolisp_decrease(number3, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/cons2.txt
    {
      FILE *file = fopen("test/cons2.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("(1 (2 (3)))", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print (1 2 . 3).
  {
    // write to test/cons3.txt
    {
      FILE *file = fopen("test/cons3.txt", "w");
      TEST(file != NULL);
      // (2 . 3)
      micolisp_number *number3 = micolisp_allocate_number(&machine);
      TEST(number3 != NULL);
      *number3 = 3;
      micolisp_number *number2 = micolisp_allocate_number(&machine);
      TEST(number2 != NULL);
      *number2 = 2;
      micolisp_cons *cons2 = micolisp_allocate_cons(number2, number3, &machine);
      TEST(cons2 != NULL);
      // (1 2 . 3)
      micolisp_number *number1 = micolisp_allocate_number(&machine);
      TEST(number1 != NULL);
      *number1 = 1;
      micolisp_cons *cons1 = micolisp_allocate_cons(number1, cons2, &machine);
      TEST(cons1 != NULL);
      TEST(micolisp_print(cons1, file, &machine) == 0);
      TEST(micolisp_decrease(cons1, &machine) == 0);
      TEST(micolisp_decrease(cons2, &machine) == 0);
      TEST(micolisp_decrease(number1, &machine) == 0);
      TEST(micolisp_decrease(number2, &machine) == 0);
      TEST(micolisp_decrease(number3, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/cons3.txt
    {
      FILE *file = fopen("test/cons3.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("(1 2 . 3)", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  // print (1 (2 . 3))
  {
    // write to test/cons4.txt
    {
      FILE *file = fopen("test/cons4.txt", "w");
      TEST(file != NULL);
      // (2 . 3)
      micolisp_number *number3 = micolisp_allocate_number(&machine);
      TEST(number3 != NULL);
      *number3 = 3;
      micolisp_number *number2 = micolisp_allocate_number(&machine);
      TEST(number2 != NULL);
      *number2 = 2;
      micolisp_cons *cons21 = micolisp_allocate_cons(number2, number3, &machine);
      TEST(cons21 != NULL);
      // ((2 . 3))
      micolisp_cons *cons2 = micolisp_allocate_cons(cons21, NULL, &machine);
      TEST(cons2 != NULL);
      // (1 (2 . 3))
      micolisp_number *number1 = micolisp_allocate_number(&machine);
      TEST(number1 != NULL);
      *number1 = 1;
      micolisp_cons *cons1 = micolisp_allocate_cons(number1, cons2, &machine);
      TEST(cons1 != NULL);
      TEST(micolisp_print(cons1, file, &machine) == 0);
      TEST(micolisp_decrease(number1, &machine) == 0);
      TEST(micolisp_decrease(number2, &machine) == 0);
      TEST(micolisp_decrease(number3, &machine) == 0);
      TEST(micolisp_decrease(cons1, &machine) == 0);
      TEST(micolisp_decrease(cons2, &machine) == 0);
      TEST(micolisp_decrease(cons21, &machine) == 0);
      TEST(fclose(file) == 0);
    }
    // read from test/cons4.txt
    {
      FILE *file = fopen("test/cons4.txt", "r");
      TEST(file != NULL);
      char buffer[256] = {};
      fread(buffer, sizeof(char), sizeof(buffer) -1, file);
      TEST(strcmp("(1 (2 . 3))", buffer) == 0);
      TEST(fclose(file) == 0);
    }
  }
  TEST(micolisp_close(&machine) == 0);
}

static void test_micolisp_load_library (){
  micolisp_machine machine;
  TEST(micolisp_open(&machine) == 0);
  TEST(micolisp_load_library(&machine) == 0);
  TEST(micolisp_close(&machine) == 0);
}

static void test_micolisp_eval (){
  micolisp_machine machine;
  TEST(micolisp_open(&machine) == 0);
  TEST(micolisp_load_library(&machine) == 0);
  // eval from test/eval.lisp (+ 1 2 3)
  {
    FILE *file = fopen("test/eval.lisp", "r");
    TEST(file != NULL);
    void *form;
    TEST(micolisp_read(file, &machine, &form) == 0);
    void *formevaluated;
    TEST(micolisp_eval(form, &machine, &formevaluated) == 0);
    TEST(micolisp_typep(MLISP_NUMBER, formevaluated, &machine));
    TEST(*(micolisp_number*)formevaluated == 6);
    TEST(fclose(file) == 0);
  }
  // eval from test/eval2.lisp (+ 1 (+ 2 3))
  {
    FILE *file = fopen("test/eval2.lisp", "r");
    TEST(file != NULL);
    void *form;
    TEST(micolisp_read(file, &machine, &form) == 0);
    void *formevaluated;
    TEST(micolisp_eval(form, &machine, &formevaluated) == 0);
    TEST(micolisp_typep(MLISP_NUMBER, formevaluated, &machine));
    TEST(*(micolisp_number*)formevaluated == 6);
    TEST(fclose(file) == 0);
  }
  TEST(micolisp_close(&machine) == 0);
}

int main (){
  test_micolisp_allocate();
  test_micolisp_scope_set();
  test_micolisp_read();
  test_micolisp_print();
  test_micolisp_load_library();
  test_micolisp_eval();
  return 0;
}
