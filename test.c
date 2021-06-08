
#include <stdio.h>
#include <stdlib.h>
#include "mlisp.h"

#define TEST(form)\
if (form){\
printf("success: %s at %s:%d\n", #form, __FILE__, __LINE__);\
}\
else {\
printf("error: %s at %s:%d\n", #form, __FILE__, __LINE__); abort();\
}

static void test_mlisp_allocate (){
  mlisp_machine machine;
  TEST(mlisp_open(&machine) == 0);
  // allocate number.
  {
    mlisp_number *number1 = mlisp_allocate_number(&machine);
    mlisp_number *number2 = mlisp_allocate_number(&machine);
    mlisp_number *number3 = mlisp_allocate_number(&machine);
    TEST(number1 != NULL);
    TEST(number2 != NULL);
    TEST(number3 != NULL);
    *number1 = 1;
    *number2 = 2;
    *number3 = 3;
    TEST(*number1 == 1);
    TEST(*number2 == 2);
    TEST(*number3 == 3);
    TEST(mlisp_decrease(number1, &machine) == 0);
    TEST(mlisp_decrease(number2, &machine) == 0);
    TEST(mlisp_decrease(number3, &machine) == 0);
  }
  // allocate symbol.
  {
    mlisp_symbol *syma = mlisp_allocate_symbol0("a", &machine);
    mlisp_symbol *symb = mlisp_allocate_symbol0("b", &machine);
    mlisp_symbol *symc = mlisp_allocate_symbol0("c", &machine);
    mlisp_symbol *syma2 = mlisp_allocate_symbol0("a", &machine);
    mlisp_symbol *symb2 = mlisp_allocate_symbol0("b", &machine);
    mlisp_symbol *symc2 = mlisp_allocate_symbol0("c", &machine);
    TEST(syma != NULL);
    TEST(symb != NULL);
    TEST(symc != NULL);
    TEST(syma != symb);
    TEST(symb != symc);
    TEST(syma == syma2);
    TEST(symb == symb2);
    TEST(symc == symc2);
    TEST(mlisp_decrease(syma, &machine) == 0);
    TEST(mlisp_decrease(symb, &machine) == 0);
    TEST(mlisp_decrease(symc, &machine) == 0);
    TEST(mlisp_decrease(syma2, &machine) == 0);
    TEST(mlisp_decrease(symb2, &machine) == 0);
    TEST(mlisp_decrease(symc2, &machine) == 0);
  }
  // allocate c function.
  {
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
    TEST(function != NULL);
    TEST(mlisp_decrease(function, &machine) == 0);
  }
  // allocate user function.
  {
    mlisp_user_function *function = mlisp_allocate_user_function(MLISP_FUNCTION, NULL, NULL, &machine);
    TEST(function != NULL);
    TEST(mlisp_decrease(function, &machine) == 0);
  }
  // allocate cons.
  {
    mlisp_cons *cons1 = mlisp_allocate_cons(NULL, NULL, &machine);
    mlisp_cons *cons2 = mlisp_allocate_cons(NULL, NULL, &machine);
    mlisp_cons *cons3 = mlisp_allocate_cons(NULL, NULL, &machine);
    TEST(cons1 != NULL);
    TEST(cons2 != NULL);
    TEST(cons3 != NULL);
    TEST(cons1 != cons2);
    TEST(cons2 != cons3);
    TEST(mlisp_decrease(cons1, &machine) == 0);
    TEST(mlisp_decrease(cons2, &machine) == 0);
    TEST(mlisp_decrease(cons3, &machine) == 0);
  }
  // close machine.
  TEST(mlisp_close(&machine) == 0);
}

static void test_mlisp_scope_set (){
  // set function.
  {
    mlisp_machine machine;
    TEST(mlisp_open(&machine) == 0);
    // set function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("a", &machine);
      mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symbol != NULL);
      TEST(function != NULL);
      TEST(mlisp_scope_set(function, symbol, &machine) == 0);
      TEST(mlisp_decrease(symbol, &machine) == 0);
      TEST(mlisp_decrease(function, &machine) == 0);
    }
    // set function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("b", &machine);
      mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symbol != NULL);
      TEST(function != NULL);
      TEST(mlisp_scope_set(function, symbol, &machine) == 0);
      TEST(mlisp_decrease(symbol, &machine) == 0);
      TEST(mlisp_decrease(function, &machine) == 0);
    }
    // set function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("c", &machine);
      mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symbol != NULL);
      TEST(function != NULL);
      TEST(mlisp_scope_set(function, symbol, &machine) == 0);
      TEST(mlisp_decrease(symbol, &machine) == 0);
      TEST(mlisp_decrease(function, &machine) == 0);
    }
    // get function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("a", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(mlisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
    }
    // get function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("b", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(mlisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
    }
    // get function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("c", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(mlisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
    }
    // get function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("d", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(mlisp_scope_get(symbol, &machine, &function) != 0); //always error!
    }
    // close machine.
    TEST(mlisp_close(&machine) == 0);
  }
  // set function.
  {
    mlisp_machine machine;
    TEST(mlisp_open(&machine) == 0);
    // set function.
    {
      mlisp_symbol *syma = mlisp_allocate_symbol0("a", &machine);
      mlisp_c_function *functiona = mlisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(syma != NULL);
      TEST(functiona != NULL);
      TEST(mlisp_scope_set(functiona, syma, &machine) == 0);
      TEST(mlisp_decrease(syma, &machine) == 0);
      TEST(mlisp_decrease(functiona, &machine) == 0);
      mlisp_symbol *symb = mlisp_allocate_symbol0("b", &machine);
      mlisp_c_function *functionb = mlisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symb != NULL);
      TEST(functionb != NULL);
      TEST((void*)symb - (void*)syma == sizeof(mlisp_symbol));
      TEST(mlisp_scope_set(functionb, symb, &machine) == 0);
      TEST(mlisp_decrease(symb, &machine) == 0);
      TEST(mlisp_decrease(functionb, &machine) == 0);
      mlisp_symbol *symc = mlisp_allocate_symbol0("c", &machine);
      mlisp_c_function *functionc = mlisp_allocate_c_function(MLISP_FUNCTION, NULL, &machine);
      TEST(symc != NULL);
      TEST(functionc != NULL);
      TEST((void*)symc - (void*)symb == sizeof(mlisp_symbol));
      TEST(mlisp_scope_set(functionc, symc, &machine) == 0);
      TEST(mlisp_decrease(symc, &machine) == 0);
      TEST(mlisp_decrease(functionc, &machine) == 0);
    }
    // get function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("a", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(mlisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
      TEST(mlisp_decrease(symbol, &machine) == 0);
    }
    // get function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("b", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(mlisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
      TEST(mlisp_decrease(symbol, &machine) == 0);
    }
    // get function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("c", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(mlisp_scope_get(symbol, &machine, &function) == 0);
      TEST(function != NULL);
      TEST(mlisp_decrease(symbol, &machine) == 0);
    }
    // get function.
    {
      mlisp_symbol *symbol = mlisp_allocate_symbol0("d", &machine);
      void *function;
      TEST(symbol != NULL);
      TEST(mlisp_scope_get(symbol, &machine, &function) != 0); //always error!
      TEST(mlisp_decrease(symbol, &machine) == 0);
    }
    // close machine.
    TEST(mlisp_close(&machine) == 0);
  }
}

static void test_mlisp_read (){
  mlisp_machine machine;
  TEST(mlisp_open(&machine) == 0);
  // read from test/boolean.lisp 
  {
    FILE *file = fopen("test/boolean.lisp", "r");
    TEST(file != NULL);
    void *value;
    // read t. 
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(value == MLISP_T);
    TEST(mlisp_decrease(value, &machine) == 0);
    // read nil.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(value == MLISP_NIL);
    TEST(mlisp_decrease(value, &machine) == 0);
    // read eof.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_EOF);
    TEST(fclose(file) == 0);
  }
  // read from test/number.lisp 
  {
    FILE *file = fopen("test/number.lisp", "r");
    TEST(file != NULL);
    void *value;
    // read 1 
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_NUMBER, value, &machine) == true);
    TEST(*(mlisp_number*)value == 1.0);
    TEST(mlisp_decrease(value, &machine) == 0);
    // read 2.5
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_NUMBER, value, &machine) == true);
    TEST(*(mlisp_number*)value == 2.5);
    TEST(mlisp_decrease(value, &machine) == 0);
    // read -3
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_NUMBER, value, &machine) == true);
    TEST(*(mlisp_number*)value == -3.0);
    TEST(mlisp_decrease(value, &machine) == 0);
    // read -4.5
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_NUMBER, value, &machine) == true);
    TEST(*(mlisp_number*)value == -4.5);
    TEST(mlisp_decrease(value, &machine) == 0);
    // read eof.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_EOF);
    TEST(fclose(file) == 0);
  }
  // read from test/symbol.lisp
  {
    FILE *file = fopen("test/symbol.lisp", "r");
    TEST(file != NULL);
    void *value;
    // must be abc.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_SYMBOL, value, &machine) == true);
    mlisp_symbol *symabc = mlisp_allocate_symbol0("abc", &machine);
    TEST(symabc != NULL);
    TEST(symabc == value);
    TEST(mlisp_decrease(value, &machine) == 0);
    TEST(mlisp_decrease(symabc, &machine) == 0);
    // must be def.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_SYMBOL, value, &machine) == true);
    mlisp_symbol *symdef = mlisp_allocate_symbol0("def", &machine);
    TEST(symdef != NULL);
    TEST(symdef == value);
    TEST(mlisp_decrease(value, &machine) == 0);
    TEST(mlisp_decrease(symdef, &machine) == 0);
    // must be ghi.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_SYMBOL, value, &machine) == true);
    mlisp_symbol *symghi = mlisp_allocate_symbol0("ghi", &machine);
    TEST(symghi != NULL);
    TEST(symghi == value);
    TEST(mlisp_decrease(value, &machine) == 0);
    TEST(mlisp_decrease(symghi, &machine) == 0);
    // must be +.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_SYMBOL, value, &machine) == true);
    mlisp_symbol *symplus = mlisp_allocate_symbol0("+", &machine);
    TEST(symplus != NULL);
    TEST(symplus == value);
    TEST(mlisp_decrease(value, &machine) == 0);
    TEST(mlisp_decrease(symplus, &machine) == 0);
    // must be -.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
    TEST(mlisp_typep(MLISP_SYMBOL, value, &machine) == true);
    mlisp_symbol *symminus = mlisp_allocate_symbol0("-", &machine);
    TEST(symminus != NULL);
    TEST(symminus == value);
    TEST(mlisp_decrease(value, &machine) == 0);
    TEST(mlisp_decrease(symminus, &machine) == 0);
    // read eof.
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_EOF);
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
      TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(mlisp_typep(MLISP_CONS, value, &machine) == true);
      mlisp_cons *cons1 = value;
      TEST(mlisp_typep(MLISP_NUMBER, cons1->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons1->car) == 1.0);
      mlisp_cons *cons2 = cons1->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons2->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons2->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons2->car) == 2.0);
      mlisp_cons *cons3 = cons2->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons3->car, &machine) == true);
      TEST(cons3->cdr == NULL);
      TEST(*(mlisp_number*)(cons3->car) == 3.0);
      TEST(mlisp_decrease(value, &machine) == 0);
    }
    // must be (1 (2 (3)))
    {
      // (1 (2 (3)))
      void *value;
      TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(mlisp_typep(MLISP_CONS, value, &machine) == true);
      // (1 (2 (3)))
      mlisp_cons *cons1 = value;
      TEST(mlisp_typep(MLISP_NUMBER, cons1->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons1->car) == 1.0);
      // (2 (3))
      mlisp_cons *cons2 = cons1->cdr;
      TEST(mlisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (2 (3))
      mlisp_cons *cons21 = cons2->car;
      TEST(mlisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons21->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons21->car) == 2.0);
      // (3)
      mlisp_cons *cons22 = cons21->cdr;
      TEST(mlisp_typep(MLISP_CONS, cons22->car, &machine) == true);
      TEST(cons22->cdr == NULL);
      // (3)
      mlisp_cons *cons221 = cons22->car;
      TEST(mlisp_typep(MLISP_NUMBER, cons221->car, &machine) == true);
      TEST(cons221->cdr == NULL);
      TEST(*(mlisp_number*)(cons221->car) == 3.0);
    }
    // must be (1 2 . 3)
    {
      // (1 2 . 3)
      void *value;
      TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(mlisp_typep(MLISP_CONS, value, &machine) == true);
      // (1 2 . 3)
      mlisp_cons *cons1 = value;
      TEST(mlisp_typep(MLISP_NUMBER, cons1->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons1->car) == 1.0);
      // (2 . 3)
      mlisp_cons *cons2 = cons1->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons2->car, &machine) == true);
      TEST(mlisp_typep(MLISP_NUMBER, cons2->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons2->car) == 2.0);
      TEST(*(mlisp_number*)(cons2->cdr) == 3.0);
      TEST(mlisp_decrease(value, &machine) == 0);
    }
    // must be (1 (2 . 3))
    {
      // (1 (2 . 3))
      void *value;
      TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(mlisp_typep(MLISP_CONS, value, &machine) == true);
      // (1 (2 . 3))
      mlisp_cons *cons1 = value;
      TEST(mlisp_typep(MLISP_NUMBER, cons1->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons1->car) == 1.0);
      // (2 . 3)
      mlisp_cons *cons2 = cons1->cdr;
      TEST(mlisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (2 . 3)
      mlisp_cons *cons21 = cons2->car;
      TEST(mlisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(mlisp_typep(MLISP_NUMBER, cons21->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons21->car) == 2.0);
      TEST(*(mlisp_number*)(cons21->cdr) == 3.0);
      TEST(mlisp_decrease(value, &machine) == 0);
    }
    // read eof.
    void *value;
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_EOF);
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
      TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(mlisp_typep(MLISP_CONS, value, &machine) == true);
      mlisp_cons *cons1 = value;
      TEST(mlisp_typep(MLISP_SYMBOL, cons1->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      // ((97 98 99))
      mlisp_cons *cons2 = cons1->cdr;
      TEST(mlisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (97 98 99)
      mlisp_cons *cons21 = cons2->car;
      TEST(mlisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons21->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons21->car) == 97);
      // (98 99)
      mlisp_cons *cons22 = cons21->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons22->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons22->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons22->car) == 98);
      // (99)
      mlisp_cons *cons23 = cons22->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons23->car, &machine) == true);
      TEST(cons23->cdr == NULL);
      TEST(*(mlisp_number*)(cons23->car) == 99);
      TEST(mlisp_decrease(value, &machine) == 0);
    }
    // must be (quote (100 101 102))
    {
      // (quote (100 101 102))
      void *value;
      TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(mlisp_typep(MLISP_CONS, value, &machine) == true);
      mlisp_cons *cons1 = value;
      TEST(mlisp_typep(MLISP_SYMBOL, cons1->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      // ((100 101 102))
      mlisp_cons *cons2 = cons1->cdr;
      TEST(mlisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (100 101 102)
      mlisp_cons *cons21 = cons2->car;
      TEST(mlisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons21->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons21->car) == 100);
      // (101 102)
      mlisp_cons *cons22 = cons21->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons22->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons22->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons22->car) == 101);
      // (102)
      mlisp_cons *cons23 = cons22->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons23->car, &machine) == true);
      TEST(cons23->cdr == NULL);
      TEST(*(mlisp_number*)(cons23->car) == 102);
      TEST(mlisp_decrease(value, &machine) == 0);
    }
    // must be (quote (103 104 105))
    {
      // (quote (103 104 105))
      void *value;
      TEST(mlisp_read(file, &machine, &value) == MLISP_READ_SUCCESS);
      TEST(mlisp_typep(MLISP_CONS, value, &machine) == true);
      mlisp_cons *cons1 = value;
      TEST(mlisp_typep(MLISP_SYMBOL, cons1->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons1->cdr, &machine) == true);
      // ((103 104 105))
      mlisp_cons *cons2 = cons1->cdr;
      TEST(mlisp_typep(MLISP_CONS, cons2->car, &machine) == true);
      TEST(cons2->cdr == NULL);
      // (103 104 105)
      mlisp_cons *cons21 = cons2->car;
      TEST(mlisp_typep(MLISP_NUMBER, cons21->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons21->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons21->car) == 103);
      // (104 105)
      mlisp_cons *cons22 = cons21->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons22->car, &machine) == true);
      TEST(mlisp_typep(MLISP_CONS, cons22->cdr, &machine) == true);
      TEST(*(mlisp_number*)(cons22->car) == 104);
      // (105)
      mlisp_cons *cons23 = cons22->cdr;
      TEST(mlisp_typep(MLISP_NUMBER, cons23->car, &machine) == true);
      TEST(cons23->cdr == NULL);
      TEST(*(mlisp_number*)(cons23->car) == 105);
      TEST(mlisp_decrease(value, &machine) == 0);
    }
    // read eof.
    void *value;
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_EOF);
    // close file.
    TEST(fclose(file) == 0);
  }
  // read from test/comment.lisp
  {
    FILE *file = fopen("test/comment.lisp", "r");
    TEST(file != NULL);
    void *value;
    TEST(mlisp_read(file, &machine, &value) == MLISP_READ_EOF);
    TEST(fclose(file) == 0);
  }
  TEST(mlisp_close(&machine) == 0);
}

static void test_mlisp_print (){
  mlisp_machine machine;
  TEST(mlisp_open(&machine) == 0);
  // print 0.
  {
    // write to test/number.txt
    {
      FILE *file = fopen("test/number.txt", "w");
      TEST(file != NULL);
      mlisp_number *number = mlisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = 0;
      TEST(mlisp_print(number, file, &machine) == 0);
      TEST(mlisp_decrease(number, &machine) == 0);
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
      mlisp_number *number = mlisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = 1;
      TEST(mlisp_print(number, file, &machine) == 0);
      TEST(mlisp_decrease(number, &machine) == 0);
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
      mlisp_number *number = mlisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = 1.5;
      TEST(mlisp_print(number, file, &machine) == 0);
      TEST(mlisp_decrease(number, &machine) == 0);
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
      mlisp_number *number = mlisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = -1;
      TEST(mlisp_print(number, file, &machine) == 0);
      TEST(mlisp_decrease(number, &machine) == 0);
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
      mlisp_number *number = mlisp_allocate_number(&machine);
      TEST(number != NULL);
      *number = -1.5;
      TEST(mlisp_print(number, file, &machine) == 0);
      TEST(mlisp_decrease(number, &machine) == 0);
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
      mlisp_symbol *symbol = mlisp_allocate_symbol0("abc", &machine);
      TEST(symbol != NULL);
      TEST(mlisp_print(symbol, file, &machine) == 0);
      TEST(mlisp_decrease(symbol, &machine) == 0);
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
      mlisp_number *number3 = mlisp_allocate_number(&machine);
      TEST(number3 != NULL);
      *number3 = 3;
      mlisp_cons *cons3 = mlisp_allocate_cons(number3, NULL, &machine);
      TEST(cons3 != NULL);
      // (2 3)
      mlisp_number *number2 = mlisp_allocate_number(&machine);
      TEST(number2 != NULL);
      *number2 = 2;
      mlisp_cons *cons2 = mlisp_allocate_cons(number2, cons3, &machine);
      TEST(cons2 != NULL);
      // (1 2 3)
      mlisp_number *number1 = mlisp_allocate_number(&machine);
      TEST(number1 != NULL);
      *number1 = 1;
      mlisp_cons *cons1 = mlisp_allocate_cons(number1, cons2, &machine);
      TEST(cons1 != NULL);
      TEST(mlisp_print(cons1, file, &machine) == 0);
      TEST(mlisp_decrease(cons1, &machine) == 0);
      TEST(mlisp_decrease(cons2, &machine) == 0);
      TEST(mlisp_decrease(cons3, &machine) == 0);
      TEST(mlisp_decrease(number1, &machine) == 0);
      TEST(mlisp_decrease(number2, &machine) == 0);
      TEST(mlisp_decrease(number3, &machine) == 0);
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
      mlisp_number *number3 = mlisp_allocate_number(&machine);
      TEST(number3 != NULL);
      *number3 = 3;
      mlisp_cons *cons3 = mlisp_allocate_cons(number3, NULL, &machine);
      TEST(cons3 != NULL);
      // ((3))
      mlisp_cons *cons21 = mlisp_allocate_cons(cons3, NULL, &machine);
      TEST(cons21 != NULL);
      // (2 (3))
      mlisp_number *number2 = mlisp_allocate_number(&machine);
      TEST(number2 != NULL);
      *number2 = 2;
      mlisp_cons *cons2 = mlisp_allocate_cons(number2, cons21, &machine);
      TEST(cons2 != NULL);
      // ((2 (3)))
      mlisp_cons *cons11 = mlisp_allocate_cons(cons2, NULL, &machine);
      TEST(cons11 != NULL);
      mlisp_number *number1 = mlisp_allocate_number(&machine);
      TEST(number1 != NULL);
      *number1 = 1;
      // (1 (2 (3)))
      mlisp_cons *cons1 = mlisp_allocate_cons(number1, cons11, &machine);
      TEST(cons1 != NULL);
      TEST(mlisp_print(cons1, file, &machine) == 0);
      TEST(mlisp_decrease(cons1, &machine) == 0);
      TEST(mlisp_decrease(cons11, &machine) == 0);
      TEST(mlisp_decrease(cons2, &machine) == 0);
      TEST(mlisp_decrease(cons21, &machine) == 0);
      TEST(mlisp_decrease(cons3, &machine) == 0);
      TEST(mlisp_decrease(number1, &machine) == 0);
      TEST(mlisp_decrease(number2, &machine) == 0);
      TEST(mlisp_decrease(number3, &machine) == 0);
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
      mlisp_number *number3 = mlisp_allocate_number(&machine);
      TEST(number3 != NULL);
      *number3 = 3;
      mlisp_number *number2 = mlisp_allocate_number(&machine);
      TEST(number2 != NULL);
      *number2 = 2;
      mlisp_cons *cons2 = mlisp_allocate_cons(number2, number3, &machine);
      TEST(cons2 != NULL);
      // (1 2 . 3)
      mlisp_number *number1 = mlisp_allocate_number(&machine);
      TEST(number1 != NULL);
      *number1 = 1;
      mlisp_cons *cons1 = mlisp_allocate_cons(number1, cons2, &machine);
      TEST(cons1 != NULL);
      TEST(mlisp_print(cons1, file, &machine) == 0);
      TEST(mlisp_decrease(cons1, &machine) == 0);
      TEST(mlisp_decrease(cons2, &machine) == 0);
      TEST(mlisp_decrease(number1, &machine) == 0);
      TEST(mlisp_decrease(number2, &machine) == 0);
      TEST(mlisp_decrease(number3, &machine) == 0);
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
      mlisp_number *number3 = mlisp_allocate_number(&machine);
      TEST(number3 != NULL);
      *number3 = 3;
      mlisp_number *number2 = mlisp_allocate_number(&machine);
      TEST(number2 != NULL);
      *number2 = 2;
      mlisp_cons *cons21 = mlisp_allocate_cons(number2, number3, &machine);
      TEST(cons21 != NULL);
      // ((2 . 3))
      mlisp_cons *cons2 = mlisp_allocate_cons(cons21, NULL, &machine);
      TEST(cons2 != NULL);
      // (1 (2 . 3))
      mlisp_number *number1 = mlisp_allocate_number(&machine);
      TEST(number1 != NULL);
      *number1 = 1;
      mlisp_cons *cons1 = mlisp_allocate_cons(number1, cons2, &machine);
      TEST(cons1 != NULL);
      TEST(mlisp_print(cons1, file, &machine) == 0);
      TEST(mlisp_decrease(number1, &machine) == 0);
      TEST(mlisp_decrease(number2, &machine) == 0);
      TEST(mlisp_decrease(number3, &machine) == 0);
      TEST(mlisp_decrease(cons1, &machine) == 0);
      TEST(mlisp_decrease(cons2, &machine) == 0);
      TEST(mlisp_decrease(cons21, &machine) == 0);
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
  TEST(mlisp_close(&machine) == 0);
}

static void test_mlisp_load_library (){
  mlisp_machine machine;
  TEST(mlisp_open(&machine) == 0);
  TEST(mlisp_load_library(&machine) == 0);
  TEST(mlisp_close(&machine) == 0);
}

static void test_mlisp_eval (){
  mlisp_machine machine;
  TEST(mlisp_open(&machine) == 0);
  TEST(mlisp_load_library(&machine) == 0);
  // eval from test/eval.lisp (+ 1 2 3)
  {
    FILE *file = fopen("test/eval.lisp", "r");
    TEST(file != NULL);
    void *form;
    TEST(mlisp_read(file, &machine, &form) == 0);
    void *formevaluated;
    TEST(mlisp_eval(form, &machine, &formevaluated) == 0);
    TEST(mlisp_typep(MLISP_NUMBER, formevaluated, &machine));
    TEST(*(mlisp_number*)formevaluated == 6);
    TEST(fclose(file) == 0);
  }
  // eval from test/eval2.lisp (+ 1 (+ 2 3))
  {
    FILE *file = fopen("test/eval2.lisp", "r");
    TEST(file != NULL);
    void *form;
    TEST(mlisp_read(file, &machine, &form) == 0);
    void *formevaluated;
    TEST(mlisp_eval(form, &machine, &formevaluated) == 0);
    TEST(mlisp_typep(MLISP_NUMBER, formevaluated, &machine));
    TEST(*(mlisp_number*)formevaluated == 6);
    TEST(fclose(file) == 0);
  }
  TEST(mlisp_close(&machine) == 0);
}

int main (){
  test_mlisp_allocate();
  test_mlisp_scope_set();
  test_mlisp_read();
  test_mlisp_print();
  test_mlisp_load_library();
  test_mlisp_eval();
  return 0;
}
