
#pragma once
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "memnode.h"
#include "hashset.h"
#include "hashtable.h"
#include "cgcmemnode.h"

#define MLISP_NIL NULL
#define MLISP_T ((void*)~0)

#define MLISP_SYMBOL_MAX_LENGTH 32
#define MLISP_ERROR_INFO_MAX_LENGTH 256

struct mlisp_machine;

typedef double mlisp_number;

typedef struct mlisp_symbol {
  char characters[MLISP_SYMBOL_MAX_LENGTH];
  size_t length;
  size_t hash;
} mlisp_symbol;

typedef struct mlisp_cons {
  void *car;
  void *cdr;
} mlisp_cons;

typedef enum mlisp_cons_whence {
  MLISP_CONS_CAR,
  MLISP_CONS_CDR,
} mlisp_cons_whence;

typedef struct mlisp_cons_reference {
  mlisp_cons_whence whence;
  mlisp_cons *cons;
} mlisp_cons_reference;

typedef enum mlisp_function_type {
  MLISP_FUNCTION,
  MLISP_SYNTAX,
  MLISP_MACRO,
} mlisp_function_type;

typedef struct mlisp_function { 
  mlisp_function_type type;
} mlisp_function;

typedef int (*mlisp_c_function_main)(mlisp_cons*, struct mlisp_machine*, void**);

typedef struct mlisp_c_function {
  mlisp_function function;
  mlisp_c_function_main main;
} mlisp_c_function;

typedef struct mlisp_user_function {
  mlisp_function function;
  mlisp_cons *args;
  mlisp_cons *form;
} mlisp_user_function;

typedef struct mlisp_scope {
  hashtable hashtable;
  struct mlisp_scope *parent;
} mlisp_scope;

typedef struct mlisp_scope_reference { 
  mlisp_symbol *name;
  mlisp_scope *scope;
} mlisp_scope_reference;

typedef enum mlisp_memory_type {
  MLISP_NUMBER,
  MLISP_SYMBOL, //1
  MLISP_CONS,
  MLISP_CONS_REFERENCE,
  MLISP_C_FUNCTION,
  MLISP_USER_FUNCTION,
  MLISP_SCOPE, //6
  MLISP_SCOPE_REFERENCE,
  MLISP_HASHTABLE_ENTRY,
  MLISP_HASHSET_ENTRY,
} mlisp_memory_type;

typedef struct mlisp_memory {
  cgcmemnode *number;
  cgcmemnode *symbol;
  cgcmemnode *cons;
  cgcmemnode *consreference;
  cgcmemnode *cfunction;
  cgcmemnode *userfunction;
  cgcmemnode *scope;
  cgcmemnode *scopereference;
  cgcmemnode *hashtableentry;
  cgcmemnode *hashsetentry;
} mlisp_memory;

typedef struct mlisp_machine { 
  mlisp_memory memory;
  mlisp_scope *scope;
  hashset symbol;
} mlisp_machine;

typedef enum mlisp_error_type {
  MLISP_NOERROR = 0,
  MLISP_ERROR,
  MLISP_INTERNAL_ERROR,
  MLISP_TYPE_ERROR,
  MLISP_VALUE_ERROR,
  MLISP_SYNTAX_ERROR,
} mlisp_error_type;

typedef struct mlisp_error_info { 
  int code;
  char message[MLISP_ERROR_INFO_MAX_LENGTH];
} mlisp_error_info;

// error info 

extern _Thread_local mlisp_error_info mlisp_error;

extern void mlisp_error_info_set (int, char*, size_t);
extern void mlisp_error_info_set0 (int, char*);
extern void mlisp_error_info_get (int*, char*);

// number 

extern mlisp_number *mlisp_allocate_number (mlisp_machine*);

// symbol 

extern mlisp_symbol *mlisp_allocate_symbol (char*, size_t, mlisp_machine*);
extern mlisp_symbol *mlisp_allocate_symbol0 (char*, mlisp_machine*);
extern bool mlisp_symbol_equal (mlisp_symbol*, mlisp_symbol*);

// c-function

extern mlisp_c_function *mlisp_allocate_c_function (mlisp_function_type, mlisp_c_function_main, mlisp_machine*);

// user-function

extern mlisp_user_function *mlisp_allocate_user_function (mlisp_function_type, mlisp_cons*, mlisp_cons*, mlisp_machine*);

// function 

extern bool mlisp_functionp (void*, mlisp_machine*);

// quote 

extern mlisp_cons *mlisp_quote (void*, mlisp_machine*);

// cons 

extern mlisp_cons *mlisp_allocate_cons (void*, void*, mlisp_machine*);
extern int mlisp_cons_set (void*, mlisp_cons_whence, mlisp_cons*, mlisp_machine*);
extern int mlisp_cons_get (mlisp_cons_whence, mlisp_cons*, void**);
extern mlisp_cons_reference *mlisp_cons_get_reference (mlisp_cons_whence, mlisp_cons*, mlisp_machine*);
extern int mlisp_cons_reference_set (void*, mlisp_cons_reference*, mlisp_machine*);
extern int mlisp_cons_reference_get (mlisp_cons_reference*, void**);

// scope 

extern int mlisp_scope_set (void*, mlisp_symbol*, mlisp_machine*);
extern int mlisp_scope_get (mlisp_symbol*, mlisp_machine*, void**);
extern mlisp_scope_reference *mlisp_scope_get_reference (mlisp_symbol*, mlisp_machine*);
extern int mlisp_scope_reference_set (void*, mlisp_scope_reference*, mlisp_machine*);
extern int mlisp_scope_reference_get (mlisp_scope_reference*, void**);
extern int mlisp_scope_begin (mlisp_machine*);
extern int mlisp_scope_end (mlisp_machine*);

// reference 

extern bool mlisp_referencep (void*, mlisp_machine*);
extern int mlisp_reference_set (void*, void*, mlisp_machine*);
extern int mlisp_reference_get (void*, mlisp_machine*, void**);

// machine 

extern bool mlisp_typep (mlisp_memory_type, void*, mlisp_machine*);
extern void *mlisp_allocate (mlisp_memory_type, size_t, mlisp_machine*);
extern int mlisp_increase (void*, mlisp_machine*);
extern int mlisp_decrease (void*, mlisp_machine*);

// lisp 

extern int mlisp_print (void*, FILE*, mlisp_machine*);
extern int mlisp_println (void*, FILE*, mlisp_machine*);

// read 

#define MLISP_READ_SUCCESS 0 
#define MLISP_READ_ERROR 1 
#define MLISP_READ_EOF 2 
#define MLISP_READ_CLOSE_PAREN 3 
#define MLISP_READ_DOT 4

extern int mlisp_read (FILE*, mlisp_machine*, void**);

// eval

extern int mlisp_eval (void*, mlisp_machine*, void**);

// repl 

extern int mlisp_repl (FILE*, FILE*, FILE*, mlisp_machine*);

// mlisp 

extern int mlisp_open (mlisp_machine*);
extern int mlisp_load_library (mlisp_machine*);
extern int mlisp_close (mlisp_machine*);
