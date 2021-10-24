
#pragma once
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "memnode.h"
#include "hashset.h"
#include "hashtable.h"
#include "cgcmemnode.h"

#define MICOLISP_NIL NULL
#define MICOLISP_T ((void*)~0)

#define MICOLISP_SYMBOL_MAX_LENGTH 32
#define MICOLISP_ERROR_INFO_MAX_LENGTH 256

struct micolisp_machine;

typedef double micolisp_number;

typedef struct micolisp_symbol {
  char characters[MICOLISP_SYMBOL_MAX_LENGTH];
  size_t length;
  size_t hash;
} micolisp_symbol;

typedef struct micolisp_cons {
  void *car;
  void *cdr;
} micolisp_cons;

typedef enum micolisp_cons_whence {
  MICOLISP_CONS_CAR,
  MICOLISP_CONS_CDR,
} micolisp_cons_whence;

typedef struct micolisp_cons_reference {
  micolisp_cons_whence whence;
  micolisp_cons *cons;
} micolisp_cons_reference;

typedef enum micolisp_function_type {
  MICOLISP_FUNCTION,
  MICOLISP_SYNTAX,
  MICOLISP_MACRO,
} micolisp_function_type;

typedef struct micolisp_function { 
  micolisp_function_type type;
} micolisp_function;

typedef int (*micolisp_c_function_main)(micolisp_cons*, struct micolisp_machine*, void**);

typedef struct micolisp_c_function {
  micolisp_function function;
  micolisp_c_function_main main;
} micolisp_c_function;

typedef struct micolisp_user_function {
  micolisp_function function;
  micolisp_cons *args;
  micolisp_cons *form;
} micolisp_user_function;

typedef struct micolisp_scope {
  hashtable hashtable;
  struct micolisp_scope *parent;
} micolisp_scope;

typedef struct micolisp_scope_reference { 
  micolisp_symbol *name;
  micolisp_scope *scope;
} micolisp_scope_reference;

typedef enum micolisp_memory_type {
  MICOLISP_NUMBER,
  MICOLISP_SYMBOL,
  MICOLISP_CONS,
  MICOLISP_CONS_REFERENCE,
  MICOLISP_C_FUNCTION,
  MICOLISP_USER_FUNCTION,
  MICOLISP_SCOPE,
  MICOLISP_SCOPE_REFERENCE,
  MICOLISP_HASHTABLE_ENTRY,
  MICOLISP_HASHSET_ENTRY,
} micolisp_memory_type;

typedef struct micolisp_memory {
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
} micolisp_memory;

typedef struct micolisp_machine { 
  micolisp_memory memory;
  micolisp_scope *scope;
  hashset symbol;
} micolisp_machine;

typedef enum micolisp_error_type {
  MICOLISP_NOERROR = 0,
  MICOLISP_ERROR,
  MICOLISP_INTERNAL_ERROR,
  MICOLISP_TYPE_ERROR,
  MICOLISP_VALUE_ERROR,
  MICOLISP_SYNTAX_ERROR,
} micolisp_error_type;

typedef struct micolisp_error_info { 
  int code;
  char message[MICOLISP_ERROR_INFO_MAX_LENGTH];
} micolisp_error_info;

// error info 

extern _Thread_local micolisp_error_info micolisp_error;

extern void micolisp_error_set (int, char*, size_t);
extern void micolisp_error_set0 (int, char*);
extern void micolisp_error_get (int*, char*);

// number 

extern micolisp_number *micolisp_allocate_number (micolisp_machine*);

// symbol 

extern micolisp_symbol *micolisp_allocate_symbol (char*, size_t, micolisp_machine*);
extern micolisp_symbol *micolisp_allocate_symbol0 (char*, micolisp_machine*);

// c-function

extern micolisp_c_function *micolisp_allocate_c_function (micolisp_function_type, micolisp_c_function_main, micolisp_machine*);

// user-function

extern micolisp_user_function *micolisp_allocate_user_function (micolisp_function_type, micolisp_cons*, micolisp_cons*, micolisp_machine*);

// function 

extern bool micolisp_functionp (void*, micolisp_machine*);

// quote 

extern micolisp_cons *micolisp_quote (void*, micolisp_machine*);

// cons 

extern micolisp_cons *micolisp_allocate_cons (void*, void*, micolisp_machine*);
extern int micolisp_cons_set (void*, micolisp_cons_whence, micolisp_cons*, micolisp_machine*);
extern int micolisp_cons_get (micolisp_cons_whence, micolisp_cons*, void**);
extern micolisp_cons_reference *micolisp_cons_get_reference (micolisp_cons_whence, micolisp_cons*, micolisp_machine*);
extern int micolisp_cons_reference_set (void*, micolisp_cons_reference*, micolisp_machine*);
extern int micolisp_cons_reference_get (micolisp_cons_reference*, void**);

// scope 

extern int micolisp_scope_set (void*, micolisp_symbol*, micolisp_machine*);
extern int micolisp_scope_get (micolisp_symbol*, micolisp_machine*, void**);
extern micolisp_scope_reference *micolisp_scope_get_reference (micolisp_symbol*, micolisp_machine*);
extern int micolisp_scope_reference_set (void*, micolisp_scope_reference*, micolisp_machine*);
extern int micolisp_scope_reference_get (micolisp_scope_reference*, void**);

// reference 

extern bool micolisp_referencep (void*, micolisp_machine*);
extern int micolisp_reference_set (void*, void*, micolisp_machine*);
extern int micolisp_reference_get (void*, micolisp_machine*, void**);

// machine 

extern bool micolisp_typep (micolisp_memory_type, void*, micolisp_machine*);
extern void *micolisp_allocate (micolisp_memory_type, size_t, micolisp_machine*);
extern int micolisp_increase (void*, micolisp_machine*);
extern int micolisp_decrease (void*, micolisp_machine*);

// lisp 

extern int micolisp_print (void*, FILE*, micolisp_machine*);
extern int micolisp_println (void*, FILE*, micolisp_machine*);

// read 

#define MICOLISP_READ_SUCCESS 0 
#define MICOLISP_READ_ERROR 1 
#define MICOLISP_READ_EOF 2 
#define MICOLISP_READ_CLOSE_PAREN 3 
#define MICOLISP_READ_DOT 4

extern int micolisp_read (FILE*, micolisp_machine*, void**);

// eval

extern int micolisp_eval (void*, micolisp_machine*, void**);

// repl 

extern int micolisp_repl (FILE*, FILE*, FILE*, micolisp_machine*);

// micolisp 

extern int micolisp_open (micolisp_machine*);
extern int micolisp_load_library (micolisp_machine*);
extern int micolisp_close (micolisp_machine*);
