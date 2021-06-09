
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "memnode.h"
#include "hashset.h"
#include "hashtable.h"
#include "cgcmemnode.h"
#include "mlisp.h"

#ifndef MAX 
#define MAX(a, b) ((a)<(b)?(b):(a))
#endif 

// hashset

static size_t __hashset_hash (void *symbol, void *arg){
  return ((mlisp_symbol*)symbol)->hash;
}

static bool __hashset_compare (void *symbol1, void *symbol2, void *arg){
  return mlisp_symbol_equal(symbol1, symbol2);
}

static const hashset_class __MLISP_HASHSET_CLASS = { __hashset_hash, __hashset_compare, NULL, NULL };
static const hashset_class *MLISP_HASHSET_CLASS = &__MLISP_HASHSET_CLASS;

// hashtable

static size_t __hashtable_hash (void *symbol, void *arg){
  return ((mlisp_symbol*)symbol)->hash;
}

static bool __hashtable_compare (void *symbol1, void *symbol2, void *arg){
  return symbol1 == symbol2;
}

static const hashtable_class __MLISP_HASHTABLE_CLASS = { __hashtable_hash, __hashtable_compare, NULL, NULL };
static const hashtable_class *MLISP_HASHTABLE_CLASS = &__MLISP_HASHTABLE_CLASS;

// string utility 

static bool string_find0 (char character, char *characters){
  for (char *c = characters; *c != '\0'; c++){
    if (*c == character){ return true; }
  }
  return false;
}

static size_t string_length (char *characters){
  char *c = characters;
  while (*c != '\0'){ c++; }
  return c - characters;
}

static void copy (char *characters, size_t size, char *charactersto){
  for (size_t index = 0; index < size; index++){
    charactersto[index] = characters[index];
  }
}

// list 

static int listp (mlisp_cons *list, mlisp_machine *machine){
  for (mlisp_cons *cons = list; cons != NULL; cons = cons->cdr){
    if (!mlisp_typep(MLISP_CONS, cons, machine)){ return false; }
  }
  return true;
}

static int list_nth (size_t index, mlisp_cons *list, void **valuep){
  size_t i;
  mlisp_cons *cons;
  for (i = 0, cons = list; i < index && cons != NULL; i++, cons = cons->cdr);
  if (i == index && cons != NULL){
    *valuep = cons->car;
    return 0;
  }
  else {
    return 1;
  }
}

static size_t list_length (mlisp_cons *cons){
  size_t length = 0;
  for (mlisp_cons *cn = cons; cn != NULL; cn = cn->cdr){
    length++;
  }
  return length;
}

static mlisp_cons *list_nreverse (mlisp_cons *list){
  mlisp_cons *consprevious = NULL;
  while (list != NULL){
    mlisp_cons *consnext = list->cdr;
    list->cdr = consprevious;
    consprevious = list;
    list = consnext;
  }
  return consprevious;
}

// error info 

_Thread_local mlisp_error_info mlisp_error = { MLISP_NOERROR, {} };

void mlisp_error_set (int code, char *message, size_t messagelen){
  mlisp_error.code = code;
  if (MLISP_ERROR_INFO_MAX_LENGTH <= messagelen){
    copy(message, MLISP_ERROR_INFO_MAX_LENGTH, mlisp_error.message);
    mlisp_error.message[MLISP_ERROR_INFO_MAX_LENGTH -1] = '\0';
    mlisp_error.message[MLISP_ERROR_INFO_MAX_LENGTH -2] = '.';
    mlisp_error.message[MLISP_ERROR_INFO_MAX_LENGTH -3] = '.';
    mlisp_error.message[MLISP_ERROR_INFO_MAX_LENGTH -4] = '.';
  }
  else {
    copy(message, messagelen, mlisp_error.message);
    mlisp_error.message[messagelen] = '\0';
  }
}

void mlisp_error_set0 (int code, char *message){
  size_t length = string_length(message);
  mlisp_error_set(code, message, length);
}

void mlisp_error_get (int *codep, char *messagep){
  if (mlisp_error.code == MLISP_NOERROR){
    *codep = MLISP_NOERROR;
    char noerrormessage[] = "no error recorded.";
    copy(noerrormessage, sizeof(noerrormessage), messagep);
  }
  else {
    *codep = mlisp_error.code;
    copy(mlisp_error.message, MLISP_ERROR_INFO_MAX_LENGTH, messagep);
  }
}

// number 

mlisp_number *mlisp_allocate_number (mlisp_machine *machine){
  return mlisp_allocate(MLISP_NUMBER, sizeof(mlisp_number), machine);
}

// symbol 

static size_t calculate_hash (char *characters, size_t length){
  size_t hash = 0;
  for (size_t index = 0; index < length; index++){
    size_t trueindex = index % sizeof(hash);
    ((uint8_t*)&hash)[trueindex] ^= characters[index];
  }
  return hash;
}

static int mlisp_symbol_init (char *characters, size_t length, mlisp_symbol *symbol){
  if (length <= MLISP_SYMBOL_MAX_LENGTH){
    copy(characters, length, symbol->characters);
    symbol->length = length;
    symbol->hash = calculate_hash(characters, length);
    return 0;
  }
  else {
    mlisp_error_set0(MLISP_VALUE_ERROR, "symbol name's length must be less than MLISP_ERROR_INFO_MAX_LENGTH."); 
    return 1; 
  }
}

mlisp_symbol *mlisp_allocate_symbol (char *characters, size_t length, mlisp_machine *machine){
  mlisp_symbol symbol;
  if (mlisp_symbol_init(characters, length, &symbol) != 0){ return NULL; }
  void *foundsymbol;
  if (hashset_get(&symbol, &(machine->symbol), &foundsymbol) != 0){
    mlisp_symbol *sym = mlisp_allocate(MLISP_SYMBOL, sizeof(mlisp_symbol), machine);
    if (sym == NULL){ return NULL; }
    *sym = symbol;
    if (hashset_add(sym, &(machine->symbol)) != 0){
      size_t newlen = MAX(8, machine->symbol.length * 2);
      hashset_entry *newentries = mlisp_allocate(MLISP_HASHSET_ENTRY, newlen * sizeof(hashset_entry), machine);
      if (newentries == NULL){ return NULL; }
      if (hashset_stretch(newentries, newlen, &(machine->symbol)) != 0){ 
        mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function hashset_stretch() was failed."); 
        return NULL; 
      }
      if (hashset_add(sym, &(machine->symbol)) != 0){ 
        mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function hashset_add() was failed."); 
        return NULL; 
      }
      return sym;
    }
    else {
      return sym;
    }
  }
  else {
    if (mlisp_increase(foundsymbol, machine) != 0){ return NULL; }
    return foundsymbol;
  }
}

mlisp_symbol *mlisp_allocate_symbol0 (char *characters, mlisp_machine *machine){
  size_t length = string_length(characters);
  return mlisp_allocate_symbol(characters, length, machine);
}

bool mlisp_symbol_equal (mlisp_symbol *symbol1, mlisp_symbol *symbol2){
  if (symbol1->hash == symbol2->hash && symbol1->length == symbol2->length){
    for (size_t index = 0; index < symbol1->length; index++){
      if (symbol1->characters[index] != symbol2->characters[index]){ return false; }
    }
    return true;
  }
  else {
    return false;
  }
}

// c-function

static void mlisp_function_init (mlisp_function_type, mlisp_function*);

mlisp_c_function *mlisp_allocate_c_function (mlisp_function_type type, mlisp_c_function_main main, mlisp_machine *machine){
  mlisp_c_function *function = mlisp_allocate(MLISP_C_FUNCTION, sizeof(mlisp_c_function), machine);
  if (function == NULL){ return NULL; }
  mlisp_function_init(type, &(function->function));
  function->main = main;
  return function;
}

static int mlisp_c_function_call (mlisp_cons *args, mlisp_c_function *function, mlisp_machine *machine, void **valuep){
  return function->main(args, machine, valuep);
}

// user-function

static void mlisp_function_init (mlisp_function_type, mlisp_function*);

mlisp_user_function *mlisp_allocate_user_function (mlisp_function_type type, mlisp_cons *args, mlisp_cons *form, mlisp_machine *machine){
  void *argsdereferenced;
  void *formdereferenced;
  if (mlisp_reference_get(args, machine, &argsdereferenced) != 0){ return NULL; }
  if (mlisp_reference_get(form, machine, &formdereferenced) != 0){ return NULL; }
  if (!listp(argsdereferenced, machine)){
    mlisp_error_set0(MLISP_TYPE_ERROR, "args must be a list.");
    return NULL; 
  }
  if (mlisp_increase(argsdereferenced, machine) != 0){ return NULL; }
  if (mlisp_increase(formdereferenced, machine) != 0){ return NULL; }
  mlisp_user_function *function = mlisp_allocate(MLISP_USER_FUNCTION, sizeof(mlisp_user_function), machine);
  if (function == NULL){ return NULL; }
  mlisp_function_init(type, &(function->function));
  function->args = argsdereferenced;
  function->form = formdereferenced;
  return function;
}

static int mlisp_user_function_call (mlisp_cons *args, mlisp_user_function *function, mlisp_machine *machine, void **valuep){
  if (mlisp_scope_begin(machine) != 0){ return 1; }
  mlisp_symbol *andrest = mlisp_allocate_symbol0("&rest", machine);
  if (andrest == NULL){ return 1; }
  mlisp_cons *afrom;
  mlisp_cons *ato;
  for (afrom = args, ato = function->args; ato != NULL; afrom = afrom->cdr, ato = ato->cdr){
    if (afrom != NULL){ 
      if (ato->car == andrest){
        if (ato->cdr != NULL){
          if (mlisp_scope_set(afrom, ((mlisp_cons*)(ato->cdr))->car, machine) != 0){ return 1; }
          break;
        }
        else {
          mlisp_error_set0(MLISP_VALUE_ERROR, "need a symbol after &rest keyword."); 
          return 1;
        }
      }
      else {
        if (mlisp_scope_set(afrom->car, ato->car, machine) != 0){ return 1; }
      }
    }
    else {
      mlisp_error_set0(MLISP_ERROR, "given not enough argument."); 
      return 1; 
    }
  }
  if (mlisp_decrease(andrest, machine) != 0){ return 1; }
  int status = mlisp_eval(function->form, machine, valuep);
  if (mlisp_scope_end(machine) != 0){ return 1; }
  return status;
}

// function 

static void mlisp_function_init (mlisp_function_type type, mlisp_function *function){
  function->type = type;
}

bool mlisp_functionp (void *function, mlisp_machine *machine){
  return mlisp_typep(MLISP_C_FUNCTION, function, machine) || mlisp_typep(MLISP_USER_FUNCTION, function, machine);
}

static int eval_args (mlisp_cons *args, mlisp_machine *machine, mlisp_cons **argsevaluatedp){
  mlisp_cons *argsevaluated = NULL;
  for (mlisp_cons *cons = args; cons != NULL; cons = cons->cdr){
    void *evaluated;
    if (mlisp_eval(cons->car, machine, &evaluated) != 0){ return 1; }
    mlisp_cons *cn = mlisp_allocate_cons(evaluated, argsevaluated, machine);
    if (cn == NULL){ return 1; }
    if (mlisp_decrease(evaluated, machine) != 0){ return 1; }
    if (mlisp_decrease(argsevaluated, machine) != 0){ return 1; } 
    argsevaluated = cn;
  }
  *argsevaluatedp = list_nreverse(argsevaluated);
  return 0;
}

static int mlisp_function_call (mlisp_cons *args, void *function, mlisp_machine *machine, void **valuep){
  if (mlisp_functionp(function, machine)){
    mlisp_cons *newargs;
    if (((mlisp_function*)function)->type == MLISP_FUNCTION){
      if (eval_args(args, machine, &newargs) != 0){ return 1; }
    }
    else {
      if (mlisp_increase(args, machine) != 0){ return 1; }
      newargs = args;
    }
    int status;
    void *value;
    if (mlisp_typep(MLISP_C_FUNCTION, function, machine)){
      status = mlisp_c_function_call(newargs, function, machine, &value);
    }
    else 
    if (mlisp_typep(MLISP_USER_FUNCTION, function, machine)){
      status = mlisp_user_function_call(newargs, function, machine, &value);
    }
    else {
      mlisp_error_set0(MLISP_TYPE_ERROR, "tried calling a non function."); 
      return 1; 
    }
    void *newvalue;
    if (((mlisp_function*)function)->type == MLISP_MACRO){
      if (mlisp_eval(value, machine, &newvalue) != 0){ return 1; }
    }
    else {
      if (mlisp_increase(value, machine) != 0){ return 1; }
      newvalue = value;
    }
    if (mlisp_decrease(newargs, machine) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
    *valuep = newvalue;
    return status;
  }
  else {
    mlisp_error_set0(MLISP_TYPE_ERROR, "tried calling a non function."); 
    return 1;
  }
}

// quote 

mlisp_cons *mlisp_quote (void *value, mlisp_machine *machine){
  mlisp_cons *cons2 = mlisp_allocate_cons(value, NULL, machine);
  if (cons2 == NULL){ return NULL; }
  mlisp_symbol *quote = mlisp_allocate_symbol0("quote", machine);
  if (quote == NULL){ return NULL; }
  mlisp_cons *cons1 = mlisp_allocate_cons(quote, cons2, machine);
  if (cons1 == NULL){ return NULL; }
  if (mlisp_decrease(cons2, machine) != 0){ return NULL; }
  if (mlisp_decrease(quote, machine) != 0){ return NULL; }
  return cons1;
}

// cons 

mlisp_cons *mlisp_allocate_cons (void *car, void *cdr, mlisp_machine *machine){
  void *cardereferenced;
  void *cdrdereferenced;
  if (mlisp_reference_get(car, machine, &cardereferenced) != 0){ return NULL; }
  if (mlisp_reference_get(cdr, machine, &cdrdereferenced) != 0){ return NULL; }
  if (mlisp_increase(cardereferenced, machine) != 0){ return NULL; }
  if (mlisp_increase(cdrdereferenced, machine) != 0){ return NULL; }
  mlisp_cons *cons = mlisp_allocate(MLISP_CONS, sizeof(mlisp_cons), machine);
  if (cons == NULL){ return NULL; }
  cons->car = cardereferenced;
  cons->cdr = cdrdereferenced;
  return cons;
}

int mlisp_cons_set (void *value, mlisp_cons_whence whence, mlisp_cons *cons, mlisp_machine *machine){
  void *valuedereferenced;
  if (mlisp_reference_get(value, machine, &valuedereferenced) != 0){ return 1; }
  switch (whence){
    case MLISP_CONS_CAR:
      if (mlisp_increase(valuedereferenced, machine) != 0){ return 1; }
      if (mlisp_decrease(cons->car, machine) != 0){ return 1; }
      cons->car = value;
      return 0;
    case MLISP_CONS_CDR:
      if (mlisp_increase(valuedereferenced, machine) != 0){ return 1; }
      if (mlisp_decrease(cons->cdr, machine) != 0){ return 1; }
      cons->cdr = value;
      return 0;
    default:
      mlisp_error_set0(MLISP_VALUE_ERROR, "given an unknown whence."); 
      return 1;
  }
}

int mlisp_cons_get (mlisp_cons_whence whence, mlisp_cons *cons, void **valuep){
  switch (whence){
    case MLISP_CONS_CAR: 
      *valuep = cons->car; 
      return 0;
    case MLISP_CONS_CDR: 
      *valuep = cons->cdr; 
      return 0;
    default: 
      mlisp_error_set0(MLISP_VALUE_ERROR, "given an unknown whence."); 
      return 1;
  }
}

mlisp_cons_reference *mlisp_cons_get_reference (mlisp_cons_whence whence, mlisp_cons *cons, mlisp_machine *machine){
  if (mlisp_increase(cons, machine) != 0){ return NULL; }
  mlisp_cons_reference *reference = mlisp_allocate(MLISP_CONS_REFERENCE, sizeof(mlisp_cons_reference), machine);
  if (reference == NULL){ return NULL; }
  reference->whence = whence;
  reference->cons = cons;
  return reference;
}

int mlisp_cons_reference_set (void *value, mlisp_cons_reference *reference, mlisp_machine *machine){
  return mlisp_cons_set(value, reference->whence, reference->cons, machine);
}

int mlisp_cons_reference_get (mlisp_cons_reference *reference, void **valuep){
  return mlisp_cons_get(reference->whence, reference->cons, valuep);
}

// scope 

int mlisp_scope_set (void *value, mlisp_symbol *name, mlisp_machine *machine){
  void *valuedereferenced;
  void *namedereferenced;
  if (mlisp_reference_get(value, machine, &valuedereferenced) != 0){ return 1; }
  if (mlisp_reference_get(name, machine, &namedereferenced) != 0){ return 1; }
  if (!mlisp_typep(MLISP_SYMBOL, namedereferenced, machine)){ 
    mlisp_error_set0(MLISP_TYPE_ERROR, "name must be a symbol.");
    return 1; 
  }
  if (machine->scope != NULL){
    if (mlisp_increase(valuedereferenced, machine) != 0){ return 1; }
    void *foundvalue;
    if (hashtable_get(namedereferenced, &(machine->scope->hashtable), &foundvalue) == 0){
      if (mlisp_decrease(foundvalue, machine) != 0){ return 1; }
      if (hashtable_set(valuedereferenced, namedereferenced, &(machine->scope->hashtable)) != 0){ 
        mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function hashtable_set() was failed.");
        return 1; 
      }
      return 0;
    }
    else {
      if (mlisp_increase(namedereferenced, machine) != 0){ return 1; }
      if (hashtable_set(valuedereferenced, namedereferenced, &(machine->scope->hashtable)) != 0){
        size_t newlen = MAX(8, machine->scope->hashtable.length * 2);
        hashtable_entry *newentries = mlisp_allocate(MLISP_HASHTABLE_ENTRY, newlen * sizeof(hashtable_entry), machine);
        if (newentries == NULL){ return 1; }
        if (hashtable_stretch(newentries, newlen, &(machine->scope->hashtable)) != 0){ 
          mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function hashtable_stretch() was failed.");
          return 1; 
        }
        if (hashtable_set(valuedereferenced, namedereferenced, &(machine->scope->hashtable)) != 0){
          mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function hashtable_set() was failed.");
          return 1; 
        }
        return 0;
      }
      else {
        return 0;
      }
    }
  }
  else {
    mlisp_error_set0(MLISP_ERROR, "no assignable scope, because scope is nil.");
    return 1;
  }
}

int mlisp_scope_get (mlisp_symbol *name, mlisp_machine *machine, void **valuep){
  void *namedereferenced;
  if (mlisp_reference_get(name, machine, &namedereferenced) != 0){ return 1; }
  if (!mlisp_typep(MLISP_SYMBOL, namedereferenced, machine)){ 
    mlisp_error_set0(MLISP_TYPE_ERROR, "name must be a symbol.");
    return 1; 
  }
  for (mlisp_scope *scope = machine->scope; scope != NULL; scope = scope->parent){
    if (hashtable_get(namedereferenced, &(scope->hashtable), valuep) == 0){ return 0; }
  }
  mlisp_error_set0(MLISP_ERROR, "could not find name in the scope.");
  return 1;
}

mlisp_scope_reference *mlisp_scope_get_reference (mlisp_symbol *name, mlisp_machine *machine){
  void *namedereferenced;
  if (mlisp_scope_get(name, machine, &namedereferenced) != 0){ return NULL; }
  if (!mlisp_typep(MLISP_SYMBOL, namedereferenced, machine)){ 
    mlisp_error_set0(MLISP_TYPE_ERROR, "name must be a symbol.");
    return NULL; 
  }
  if (mlisp_increase(namedereferenced, machine) != 0){ return NULL; }
  if (mlisp_increase(machine->scope, machine) != 0){ return NULL; }
  mlisp_scope_reference *reference = mlisp_allocate(MLISP_SCOPE_REFERENCE, sizeof(mlisp_scope_reference), machine);
  if (reference == NULL){ return NULL; }
  reference->name = namedereferenced;
  reference->scope = machine->scope;
  return reference;
}

int mlisp_scope_reference_set (void *value, mlisp_scope_reference *reference, mlisp_machine *machine){
  void *valuedereferenced;
  if (mlisp_reference_get(value, machine, &valuedereferenced) != 0){ return 1; }
  if (mlisp_increase(valuedereferenced, machine) != 0){ return 1; }
  for (mlisp_scope *scope = reference->scope; scope != NULL; scope = scope->parent){
    void *foundvalue;
    if (hashtable_get(reference->name, &(scope->hashtable), &foundvalue) == 0){
      if (mlisp_decrease(foundvalue, machine) != 0){ return 1; }
      if (hashtable_set(valuedereferenced, reference->name, &(scope->hashtable)) != 0){ 
        mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function hashtable_set() was failed.");
        return 1; 
      }
      return 0;
    }
  }
  if (mlisp_increase(reference->name, machine) != 0){ return 1; }
  if (hashtable_set(valuedereferenced, reference->name, &(reference->scope->hashtable)) != 0){
    size_t newlen = machine->scope->hashtable.length * 2;
    hashtable_entry *newentries = mlisp_allocate(MLISP_HASHTABLE_ENTRY, newlen * sizeof(MLISP_HASHTABLE_ENTRY), machine);
    if (newentries == NULL){ return 1; }
    if (hashtable_stretch(newentries, newlen, &(reference->scope->hashtable)) != 0){ 
      mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function hashtable_stretch() was failed.");
      return 1;
    }
    if (hashtable_set(valuedereferenced, reference->name, &(reference->scope->hashtable)) != 0){ 
      mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function hashtable_set() was failed.");
      return 1;
    }
    return 0;
  }
  else {
    return 0;
  }
}

int mlisp_scope_reference_get (mlisp_scope_reference *reference, void **valuep){
  for (mlisp_scope *scope = reference->scope; scope != NULL; scope = scope->parent){
    if (hashtable_get(reference->name, &(scope->hashtable), valuep) == 0){ return 0; }
  }
  mlisp_error_set0(MLISP_ERROR, "could not find name in the scope.");
  return 1;
}

int mlisp_scope_begin (mlisp_machine *machine){
  if (mlisp_increase(machine->scope, machine) != 0){ return 1; }
  mlisp_scope *scope = mlisp_allocate(MLISP_SCOPE, sizeof(mlisp_scope), machine);
  if (scope == NULL){ return 1; }
  hashtable_entry *scopeentries = mlisp_allocate(MLISP_HASHTABLE_ENTRY, 8 * sizeof(hashtable_entry), machine);
  if (scopeentries == NULL){ return 1; }
  hashtable_init(scopeentries, 8, MLISP_HASHTABLE_CLASS, &(scope->hashtable));
  scope->parent = machine->scope;
  machine->scope = scope;
  return 0;
}

int mlisp_scope_end (mlisp_machine *machine){
  if (machine->scope != NULL){
    mlisp_scope *parent = machine->scope->parent;
    if (mlisp_decrease(machine->scope, machine) != 0){ return 1; }
    machine->scope = parent;
    return 0;
  }
  else {
    mlisp_error_set0(MLISP_ERROR, "could not back to previous scope, because scope is nil.");
    return 1;
  }
}

// reference 

bool mlisp_referencep (void *address, mlisp_machine *machine){
  return mlisp_typep(MLISP_CONS_REFERENCE, address, machine) || mlisp_typep(MLISP_SCOPE_REFERENCE, address, machine);
}

int mlisp_reference_set (void *value, void *reference, mlisp_machine *machine){
  if (mlisp_typep(MLISP_CONS_REFERENCE, reference, machine)){
    return mlisp_cons_reference_set(value, reference, machine);
  }
  else 
  if (mlisp_typep(MLISP_SCOPE_REFERENCE, reference, machine)){
    return mlisp_scope_reference_set(value, reference, machine);
  }
  else {
    mlisp_error_set0(MLISP_VALUE_ERROR, "tried assigning value to a non reference.");
    return 1;
  }
}

int mlisp_reference_get (void *mayreference, mlisp_machine *machine, void **valuep){
  if (mlisp_typep(MLISP_CONS_REFERENCE, mayreference, machine)){
    return mlisp_cons_reference_get(mayreference, valuep);
  }
  else 
  if (mlisp_typep(MLISP_SCOPE_REFERENCE, mayreference, machine)){
    return mlisp_scope_reference_get(mayreference, valuep);
  }
  else {
    *valuep = mayreference;
    return 0;
  }
}

// memory

static void mlisp_memory_init (mlisp_memory *memory){
  memory->number = NULL;
  memory->symbol = NULL;
  memory->cons = NULL;
  memory->consreference = NULL;
  memory->cfunction = NULL;
  memory->userfunction = NULL;
  memory->scope = NULL;
  memory->scopereference = NULL;
  memory->hashtableentry = NULL;
  memory->hashsetentry = NULL;
} 

static void mlisp_memory_free (mlisp_memory *memory){
  free_cgcmemnode(memory->number);
  free_cgcmemnode(memory->symbol);
  free_cgcmemnode(memory->cons);
  free_cgcmemnode(memory->consreference);
  free_cgcmemnode(memory->cfunction);
  free_cgcmemnode(memory->userfunction);
  free_cgcmemnode(memory->scope);
  free_cgcmemnode(memory->scopereference);
  free_cgcmemnode(memory->hashtableentry);
  free_cgcmemnode(memory->hashsetentry);
}

static mlisp_memory_type mlisp_memory_info (mlisp_memory_type type, mlisp_memory *memory, size_t *sizep, cgcmemnode **cmemnodep, cgcmemnode ***cmemnodepp){
  switch (type){
    case MLISP_NUMBER:
      *cmemnodep = memory->number;
      *cmemnodepp = &(memory->number);
      *sizep = sizeof(mlisp_number);
      return 0;
    case MLISP_SYMBOL:
      *cmemnodep = memory->symbol;
      *cmemnodepp = &(memory->symbol);
      *sizep = sizeof(mlisp_symbol);
      return 0;
    case MLISP_CONS:
      *cmemnodep = memory->cons;
      *cmemnodepp = &(memory->cons);
      *sizep = sizeof(mlisp_cons);
      return 0;
    case MLISP_CONS_REFERENCE:
      *cmemnodep = memory->consreference;
      *cmemnodepp = &(memory->consreference);
      *sizep = sizeof(mlisp_cons_reference);
      return 0;
    case MLISP_C_FUNCTION:
      *cmemnodep = memory->cfunction;
      *cmemnodepp = &(memory->cfunction);
      *sizep = sizeof(mlisp_c_function);
      return 0;
    case MLISP_USER_FUNCTION:
      *cmemnodep = memory->userfunction;
      *cmemnodepp = &(memory->userfunction);
      *sizep = sizeof(mlisp_user_function);
      return 0;
    case MLISP_SCOPE:
      *cmemnodep = memory->scope;
      *cmemnodepp = &(memory->scope);
      *sizep = sizeof(mlisp_scope);
      return 0;
    case MLISP_SCOPE_REFERENCE:
      *cmemnodep = memory->scopereference;
      *cmemnodepp = &(memory->scopereference);
      *sizep = sizeof(mlisp_scope_reference);
      return 0;
    case MLISP_HASHTABLE_ENTRY:
      *cmemnodep = memory->hashtableentry;
      *cmemnodepp = &(memory->hashtableentry);
      *sizep = sizeof(hashtable_entry);
      return 0;
    case MLISP_HASHSET_ENTRY:
      *cmemnodep = memory->hashsetentry;
      *cmemnodepp = &(memory->hashsetentry);
      *sizep = sizeof(hashset_entry);
      return 0;
    default:
      mlisp_error_set0(MLISP_VALUE_ERROR, "given an unknown type.");
      return 1;
  }
}

static bool mlisp_memory_typep (mlisp_memory_type type, void *address, mlisp_memory *memory){
  size_t basesize;
  cgcmemnode *cmemnode;
  cgcmemnode **cmemnodep;
  if (mlisp_memory_info(type, memory, &basesize, &cmemnode, &cmemnodep) != 0){ return false; }
  return memnode_find(address, cgcmemnode_memnode(cmemnode)) != NULL;
}

static void *mlisp_memory_allocate (mlisp_memory_type type, size_t size, mlisp_memory *memory){
  size_t basesize;
  cgcmemnode *cmemnode;
  cgcmemnode **cmemnodep;
  if (mlisp_memory_info(type, memory, &basesize, &cmemnode, &cmemnodep) != 0){ return NULL; }
  void *address = cgcmemnode_allocate(size, cmemnode);
  if (address == NULL){
    mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function cgcmemnode_allocate() was failed.");
    return NULL;
  }
  return address;
}

static int mlisp_memory_increase (mlisp_memory_type type, void *address, size_t size, mlisp_memory *memory){
  size_t basesize;
  cgcmemnode *cmemnode;
  cgcmemnode **cmemnodep;
  if (mlisp_memory_info(type, memory, &basesize, &cmemnode, &cmemnodep) != 0){ return 1; }
  if (cgcmemnode_increase(address, size, cmemnode) != 0){
    mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function cgcmemnode_increase() was failed.");
    return 1;
  }
  return 0;
}

static int mlisp_memory_decrease (mlisp_memory_type type, void *address, size_t size, mlisp_memory *memory){
  size_t basesize;
  cgcmemnode *cmemnode;
  cgcmemnode **cmemnodep;
  if (mlisp_memory_info(type, memory, &basesize, &cmemnode, &cmemnodep) != 0){ return 1; }
  if (cgcmemnode_decrease(address, size, cmemnode) != 0){
    mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function cgcmemnode_decrease() was failed.");
    return 1;
  }
  return 0;
}

// machine

void mlisp_init (mlisp_machine *machine){
  mlisp_memory_init(&(machine->memory));
  machine->scope = NULL;
  hashset_init(NULL, 0, MLISP_HASHSET_CLASS, &(machine->symbol));
} 

bool mlisp_typep (mlisp_memory_type type, void *address, mlisp_machine *machine){
  return mlisp_memory_typep(type, address, &(machine->memory));
}

static size_t align_size (size_t size, size_t alignment){
  return (size / alignment * alignment) + (0 < size % alignment? alignment: 0);
}

void *mlisp_allocate (mlisp_memory_type type, size_t size, mlisp_machine *machine){
  void *address = mlisp_memory_allocate(type, size, &(machine->memory));
  if (address == NULL){
    size_t basesize;
    cgcmemnode *cmemnode;
    cgcmemnode **cmemnodep;
    if (mlisp_memory_info(type, &(machine->memory), &basesize, &cmemnode, &cmemnodep) != 0){ return NULL; }
    size_t newsize = align_size(size, 4096);
    cgcmemnode *newcmemnode = make_cgcmemnode(newsize, basesize, cmemnode);
    if (newcmemnode == NULL){ 
      mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function make_cgcmemnode() was failed.");
      return NULL; 
    }
    *cmemnodep = newcmemnode;
    void *address = mlisp_memory_allocate(type, size, &(machine->memory)); 
    if (address == NULL){ return NULL; }
    return address;
  }
  return address;
}

static int __mlisp_increase (void *address, mlisp_machine *machine, cgcmemnode_history *history){
  if (cgcmemnode_history_recordp(address, history)){
    return 0; 
  }
  if (address == MLISP_NIL){
    return 0;
  }
  else 
  if (address == MLISP_T){
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_NUMBER, address, machine)){
    if (mlisp_memory_increase(MLISP_NUMBER, address, sizeof(mlisp_number), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_SYMBOL, address, machine)){
    if (mlisp_memory_increase(MLISP_SYMBOL, address, sizeof(mlisp_symbol), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_CONS, address, machine)){
    if (mlisp_memory_increase(MLISP_CONS, address, sizeof(mlisp_cons), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__mlisp_increase(((mlisp_cons*)address)->car, machine, history) != 0){ return 1; }
    if (__mlisp_increase(((mlisp_cons*)address)->cdr, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_CONS_REFERENCE, address, machine)){
    if (mlisp_memory_increase(MLISP_CONS_REFERENCE, address, sizeof(mlisp_cons_reference), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__mlisp_increase(((mlisp_cons_reference*)address)->cons, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_C_FUNCTION, address, machine)){
    if (mlisp_memory_increase(MLISP_C_FUNCTION, address, sizeof(mlisp_c_function), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_USER_FUNCTION, address, machine)){
    if (mlisp_memory_increase(MLISP_USER_FUNCTION, address, sizeof(mlisp_user_function), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__mlisp_increase(((mlisp_user_function*)address)->args, machine, history) != 0){ return 1; }
    if (__mlisp_increase(((mlisp_user_function*)address)->form, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_SCOPE, address, machine)){
    if (mlisp_memory_increase(MLISP_SCOPE, address, sizeof(mlisp_scope), &(machine->memory)) != 0){ return 1; }
    if (mlisp_memory_increase(MLISP_HASHTABLE_ENTRY, ((mlisp_scope*)address)->hashtable.entries, ((mlisp_scope*)address)->hashtable.length * sizeof(hashtable_entry), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    hashtable_iterator iterator = hashtable_iterate(&(((mlisp_scope*)address)->hashtable));
    hashtable_entry entry;
    while (hashtable_iterator_next(&iterator, &(((mlisp_scope*)address)->hashtable), &entry) == 0){
      if (__mlisp_increase(entry.key, machine, history) != 0){ return 1; }
      if (__mlisp_increase(entry.value, machine, history) != 0){ return 1; }
    }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_SCOPE_REFERENCE, address, machine)){
    if (mlisp_memory_increase(MLISP_SCOPE_REFERENCE, address, sizeof(mlisp_scope_reference), &(machine->memory))){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__mlisp_increase(((mlisp_scope_reference*)address)->name, machine, history) != 0){ return 1; }
    if (__mlisp_increase(((mlisp_scope_reference*)address)->scope, machine, history) != 0){ return 1; }
    return 0;
  }
  else {
    mlisp_error_set0(MLISP_TYPE_ERROR, "given unmanaged address.");
    return 1; 
  }
}

static int __mlisp_decrease (void *address, mlisp_machine *machine, cgcmemnode_history *history){
  if (cgcmemnode_history_recordp(address, history)){
    return 0; 
  }
  if (address == MLISP_NIL){
    return 0;
  }
  else 
  if (address == MLISP_T){
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_NUMBER, address, machine)){
    if (mlisp_memory_decrease(MLISP_NUMBER, address, sizeof(mlisp_number), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_SYMBOL, address, machine)){
    if (mlisp_memory_decrease(MLISP_SYMBOL, address, sizeof(mlisp_symbol), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_CONS, address, machine)){
    if (mlisp_memory_decrease(MLISP_CONS, address, sizeof(mlisp_cons), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__mlisp_decrease(((mlisp_cons*)address)->car, machine, history) != 0){ return 1; }
    if (__mlisp_decrease(((mlisp_cons*)address)->cdr, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_CONS_REFERENCE, address, machine)){
    if (mlisp_memory_decrease(MLISP_CONS_REFERENCE, address, sizeof(mlisp_cons_reference), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__mlisp_decrease(((mlisp_cons_reference*)address)->cons, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_C_FUNCTION, address, machine)){
    if (mlisp_memory_decrease(MLISP_C_FUNCTION, address, sizeof(mlisp_c_function), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_USER_FUNCTION, address, machine)){
    if (mlisp_memory_decrease(MLISP_USER_FUNCTION, address, sizeof(mlisp_user_function), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__mlisp_decrease(((mlisp_user_function*)address)->args, machine, history) != 0){ return 1; }
    if (__mlisp_decrease(((mlisp_user_function*)address)->form, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_SCOPE, address, machine)){
    if (mlisp_memory_decrease(MLISP_SCOPE, address, sizeof(mlisp_scope), &(machine->memory)) != 0){ return 1; }
    if (mlisp_memory_decrease(MLISP_HASHTABLE_ENTRY, ((mlisp_scope*)address)->hashtable.entries, ((mlisp_scope*)address)->hashtable.length * sizeof(hashtable_entry), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    hashtable_iterator iterator = hashtable_iterate(&(((mlisp_scope*)address)->hashtable));
    hashtable_entry entry;
    while (hashtable_iterator_next(&iterator, &(((mlisp_scope*)address)->hashtable), &entry) == 0){
      if (__mlisp_decrease(entry.key, machine, history) != 0){ return 1; }
      if (__mlisp_decrease(entry.value, machine, history) != 0){ return 1; }
    }
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_SCOPE_REFERENCE, address, machine)){
    if (mlisp_memory_decrease(MLISP_SCOPE_REFERENCE, address, sizeof(mlisp_scope_reference), &(machine->memory))){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__mlisp_decrease(((mlisp_scope_reference*)address)->name, machine, history) != 0){ return 1; }
    if (__mlisp_decrease(((mlisp_scope_reference*)address)->scope, machine, history) != 0){ return 1; }
    return 0;
  }
  else {
    mlisp_error_set0(MLISP_TYPE_ERROR, "given unmanaged address.");
    return 1; 
  }
}

int mlisp_increase (void *address, mlisp_machine *machine){
  MAKE_CGCMEMNODE_HISTORY(history);
  return __mlisp_increase(address, machine, history);
}

int mlisp_decrease (void *address, mlisp_machine *machine){
  MAKE_CGCMEMNODE_HISTORY(history);
  return __mlisp_decrease(address, machine, history);
}

// lisp 

static int mlisp_print_number (mlisp_number *number, FILE *file, mlisp_machine *machine){
  double integerpart;
  double decimalpart = modf(*number, &integerpart);
  (void)decimalpart;
  if (decimalpart == 0.0 || decimalpart == -0.0){
    fprintf(file, "%ld", (long)*number);
    return 0;
  }
  else {
    fprintf(file, "%lg", (double)*number);
    return 0;
  }
}

static int mlisp_print_symbol (mlisp_symbol *symbol, FILE *file, mlisp_machine *machine){
  for (size_t index = 0; index < symbol->length; index++){
    putc(symbol->characters[index], file);
  }
  return 0;
}

static int mlisp_print_list (mlisp_cons *cons, FILE *file, mlisp_machine *machine){
  fputs("(", file);
  for (mlisp_cons *cn = cons; cn != NULL; cn = cn->cdr){
    if (mlisp_typep(MLISP_CONS, cn, machine)){
      if (cn != cons){ fputs(" ", file); }
      if (mlisp_print(cn->car, file, machine) != 0){ return 1; }
    }
    else {
      if (cn != cons){ fputs(" . ", file); }
      if (mlisp_print(cn, file, machine) != 0){ return 1; }
      break;
    }
  }
  fputs(")", file);
  return 0;
}

int mlisp_print (void *value, FILE *file, mlisp_machine *machine){
  void *valuedereferenced;
  if (mlisp_reference_get(value, machine, &valuedereferenced) != 0){ 
    return 1; 
  }
  if (valuedereferenced == MLISP_T){ fprintf(file, "t"); 
    return 0;
  }
  else 
  if (valuedereferenced == MLISP_NIL){ fprintf(file, "nil"); 
    return 0;
  }
  else 
  if (mlisp_typep(MLISP_NUMBER, valuedereferenced, machine)){ 
    return mlisp_print_number(valuedereferenced, file, machine);
  }
  else 
  if (mlisp_typep(MLISP_SYMBOL, valuedereferenced, machine)){ 
    return mlisp_print_symbol(valuedereferenced, file, machine);
  }
  else 
  if (mlisp_typep(MLISP_CONS, valuedereferenced, machine)){ 
    return mlisp_print_list(valuedereferenced, file, machine);
  }
  else 
  if (mlisp_typep(MLISP_C_FUNCTION, valuedereferenced, machine)){ 
    fprintf(file, "<function #%p>", valuedereferenced); 
    return 0; 
  }
  else 
  if (mlisp_typep(MLISP_USER_FUNCTION, valuedereferenced, machine)){ 
    fprintf(file, "<function #%p>", valuedereferenced); 
    return 0; 
  }
  else {
    mlisp_error_set0(MLISP_TYPE_ERROR, "given an unknown type.");
    return 1;
  }
}

int mlisp_println (void *value, FILE *file, mlisp_machine *machine){
  if (mlisp_print(value, file, machine) != 0){ return 1; }
  putc('\n', file);
  return 0;
}

// read 

#define MLISP_READ_SUCCESS 0 
#define MLISP_READ_ERROR 1 
#define MLISP_READ_EOF 2 
#define MLISP_READ_CLOSE_PAREN 3 
#define MLISP_READ_DOT 4
#define TOKEN_CHARACTERS "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "!#$%&*+-/:<=>?@^_|~."
#define WHITESPACE_CHARACTERS " \t\r\n"

static bool parse_as_tp (char *buffer, size_t size){
  return size == 1 && buffer[0] == 't';
}

static bool parse_as_nilp (char *buffer, size_t size){
  return size == 3 && buffer[0] == 'n' && buffer[1] == 'i' && buffer[2] == 'l';
}

static bool parse_as_numberp (char *buffer, size_t size){
  if (0 < size){
    size_t index = 0;
    switch (buffer[index]){
      case '-': index += 1; break;
      case '+': index += 1; break;
      default: break;
    }
    if (index < size){
      for (; index < size; index++){
        if ('0' <= buffer[index] && buffer[index] <= '9'){
          continue;
        }
        else 
        if ('.' == buffer[index]){
          index += 1;
          if (index < size){
            for (; index < size; index++){
              if ('0' <= buffer[index] && buffer[index] <= '9'){
                continue;
              }
              else {
                return false;
              }
            }
            return true;
          }
          return false;
        }
        else {
          return false;
        }
      }
      return true;
    }
    return false;
  }
  return false;
}

static int parse_as_number (char *buffer, size_t size, mlisp_machine *machine, void **valuep){
  size_t index = 0; 
  double sign;
  if (0 < size){
    switch (buffer[0]){
      case '+': sign = 1.0; index += 1; break;
      case '-': sign = -1.0; index += 1; break;
      default: sign = 1.0; break;
    }
  }
  double integerpart = 0.0;
  for (; index < size; index++){
    if ('0' <= buffer[index] && buffer[index] <= '9'){
      integerpart *= 10.0;
      integerpart += buffer[index] - '0';
    }
    else 
    if ('.' == buffer[index]){
      index += 1;
      break;
    }
    else {
      mlisp_error_set0(MLISP_SYNTAX_ERROR, "given a non digit character.");
      return 1;
    }
  }
  double decimalpart = 0.0;
  for (double decimalbase = 0.1; index < size; index++, decimalbase /= 10.0){
    if ('0' <= buffer[index] && buffer[index] <= '9'){
      decimalpart += decimalbase * (buffer[index] - '0');
    }
    else {
      mlisp_error_set0(MLISP_SYNTAX_ERROR, "given a non digit character.");
      return 1;
    }
  }
  mlisp_number *number = mlisp_allocate_number(machine);
  if (number == NULL){ return 1; }
  *number = sign * (integerpart + decimalpart);
  *valuep = number;
  return 0;
}

static int parse_as_symbol (char *buffer, size_t size, mlisp_machine *machine, void **valuep){
  mlisp_symbol *symbol = mlisp_allocate_symbol(buffer, size, machine);
  if (symbol == NULL){ return 1; }
  *valuep = symbol;
  return 0;
}

static int mlisp_read_token (FILE *file, mlisp_machine *machine, void **valuep){
  char buffer[128] = {};
  size_t index = 0;
  int character;
  while ((character = getc(file)) != EOF){
    if (index < sizeof(buffer)){
      if (string_find0(character, TOKEN_CHARACTERS)){
        buffer[index] = character;
        index += 1;
      }
      else {
        ungetc(character, file);
        break;
      }
    }
    else {
      mlisp_error_set0(MLISP_SYNTAX_ERROR, "read token is too much long, so buffer was overflow.");
      return 1; 
    }
  }
  if (parse_as_tp(buffer, index)){
    *valuep = MLISP_T;
    return 0;
  }
  else 
  if (parse_as_nilp(buffer, index)){
    *valuep = MLISP_NIL;
    return 0;
  }
  else 
  if (parse_as_numberp(buffer, index)){
    return parse_as_number(buffer, index, machine, valuep);
  }
  else {
    return parse_as_symbol(buffer, index, machine, valuep);
  }
}

static int mlisp_read_quote (FILE *file, mlisp_machine *machine, void **valuep){
  if (getc(file) != '\''){
    mlisp_error_set0(MLISP_SYNTAX_ERROR, "first character must be '\''.");
    return 1; 
  }
  void *value;
  if (mlisp_read(file, machine, &value) != 0){ return 1; }
  mlisp_cons *quoted = mlisp_quote(value, machine);
  if (quoted == NULL){ return 1; }
  if (mlisp_decrease(value, machine) != 0){ return 1; } 
  *valuep = quoted;
  return 0;
}

static int mlisp_read_list (FILE *file, mlisp_machine *machine, void **valuep){
  if (getc(file) != '('){ 
    mlisp_error_set0(MLISP_SYNTAX_ERROR, "first character must be '('.");
    return 1; 
  }
  mlisp_cons *list = NULL;
  void *value;
  while (true){
    int status = mlisp_read(file, machine, &value);
    if (status == MLISP_READ_SUCCESS){
      mlisp_cons *cons = mlisp_allocate_cons(value, list, machine);
      if (cons == NULL){ return 1; }
      if (mlisp_decrease(value, machine) != 0){ return 1; } 
      if (mlisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
    }
    else 
    if (status == MLISP_READ_CLOSE_PAREN){
      *valuep = list_nreverse(list);
      return 0;
    }
    else 
    if (status == MLISP_READ_DOT){
      void *value1;
      void *value2;
      if (mlisp_read(file, machine, &value1) != MLISP_READ_SUCCESS){ return 1; }
      if (mlisp_read(file, machine, &value2) != MLISP_READ_CLOSE_PAREN){ 
        mlisp_error_set0(MLISP_SYNTAX_ERROR, "must exist close paren after value after dot.");
        return 1; 
      }
      mlisp_cons *lastcons = list;
      mlisp_cons *cons = list_nreverse(list);
      if (mlisp_cons_set(value1, MLISP_CONS_CDR, lastcons, machine) != 0){ return 1; }
      if (mlisp_decrease(value1, machine) != 0){ return 1; }
      *valuep = cons;
      return 0;
    }
    else {
      return 1;
    }
  }
  return 1; //unreachable!
}

static int unescape (FILE *file, char *characterp){
  int character = getc(file);
  switch (character){
    case EOF: 
      mlisp_error_set0(MLISP_SYNTAX_ERROR, "read eof.");
      return 1; 
    case '0': 
      *characterp = '\0'; 
      return 0;
    case 'a': 
      *characterp = '\a'; 
      return 0;
    case 'b': 
      *characterp = '\b'; 
      return 0;
    case 'f': 
      *characterp = '\f'; 
      return 0;
    case 'n': 
      *characterp = '\n'; 
      return 0;
    case 'r': 
      *characterp = '\r'; 
      return 0;
    case 't': 
      *characterp = '\t'; 
      return 0;
    case 'v': 
      *characterp = '\v'; 
      return 0;
    default: 
      *characterp = character; 
      return 0;
  }
}

static int mlisp_read_string (FILE *file, mlisp_machine *machine, void **valuep){
  if (getc(file) != '"'){ 
    mlisp_error_set0(MLISP_SYNTAX_ERROR, "first character must be '\"'.");
    return 1; 
  }
  mlisp_cons *list = NULL;
  int character;
  while ((character = getc(file)) != EOF){
    if (character == '"'){
      break;
    }
    else 
    if (character == '\\'){
      char unescaped;
      if (unescape(file, &unescaped) != 0){ return 1; }
      mlisp_number *number = mlisp_allocate_number(machine);
      if (number == NULL){ return 1; }
      *number = unescaped;
      mlisp_cons *cons = mlisp_allocate_cons(number, list, machine);
      if (cons == NULL){ return 1; }
      if (mlisp_decrease(number, machine) != 0){ return 1; } 
      if (mlisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
    }
    else {
      mlisp_number *number = mlisp_allocate_number(machine);
      if (number == NULL){ return 1; }
      *number = character;
      mlisp_cons *cons = mlisp_allocate_cons(number, list, machine);
      if (cons == NULL){ return 1; }
      if (mlisp_decrease(number, machine) != 0){ return 1; } 
      if (mlisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
    }
  }
  *valuep = mlisp_quote(list_nreverse(list), machine);
  return 0;
}

static void mlisp_read_comment (FILE *file){
  int character;
  while ((character = getc(file)) != EOF){
    if (character == '\n'){
      break;
    }
  }
}

int mlisp_read (FILE *file, mlisp_machine *machine, void **valuep){
  int character;
  while ((character = getc(file)) != EOF){
    if (character == ';'){
      mlisp_read_comment(file);
      continue;
    }
    else 
    if (character == '"'){
      ungetc(character, file);
      return mlisp_read_string(file, machine, valuep);
    }
    else 
    if (character == '('){
      ungetc(character, file);
      return mlisp_read_list(file, machine, valuep);
    }
    else 
    if (character == ')'){
      return MLISP_READ_CLOSE_PAREN;
    }
    else 
    if (character == '.'){
      return MLISP_READ_DOT;
    }
    else 
    if (character == '\''){
      ungetc(character, file);
      return mlisp_read_quote(file, machine, valuep);
    }
    else 
    if (string_find0(character, WHITESPACE_CHARACTERS)){
      continue;
    }
    else 
    if (string_find0(character, TOKEN_CHARACTERS)){
      ungetc(character, file);
      return mlisp_read_token(file, machine, valuep);
    }
    else {
      return MLISP_READ_ERROR; 
    }
  }
  return MLISP_READ_EOF;
}

int mlisp_eval (void *form, mlisp_machine *machine, void **valuep){
  void *formdereferenced;
  if (mlisp_reference_get(form, machine, &formdereferenced) != 0){ return 1; }
  if (mlisp_typep(MLISP_CONS, formdereferenced, machine)){
    if (!listp(formdereferenced, machine)){
      mlisp_error_set0(MLISP_VALUE_ERROR, "formula is an incomplete list.");
      return 1;
    }
    void *function;
    if (mlisp_eval(((mlisp_cons*)formdereferenced)->car, machine, &function) != 0){ return 1; }
    if (mlisp_functionp(function, machine)){
      if (mlisp_function_call(((mlisp_cons*)formdereferenced)->cdr, function, machine, valuep) != 0){ return 1; }
      if (mlisp_decrease(function, machine) != 0){ return 1; }
      return 0;
    }
    else {
      mlisp_error_set0(MLISP_VALUE_ERROR, "operator in formula is non function.");
      return 1;
    }
  }
  else 
  if (mlisp_typep(MLISP_SYMBOL, formdereferenced, machine)){
    void *value;
    if (mlisp_scope_get(formdereferenced, machine, &value) != 0){ return 1; }
    if (mlisp_increase(value, machine) != 0){ return 1; }
    *valuep = value;
    return 0;
  }
  else {
    if (mlisp_increase(formdereferenced, machine) != 0){ return 1; }
    *valuep = formdereferenced;
    return 0;
  }
}

int mlisp_eval_string (char *sequence, size_t size, mlisp_machine *machine, void **valuep){
  FILE *file = tmpfile();
  if (file == NULL){ 
    mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function tmpfile() was failed.");
    return 1; 
  }
  size_t wrotesize = fwrite(sequence, sizeof(char), size, file);
  if (wrotesize < size){ 
    mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function fwrite() was failed.");
    return 1; 
  }
  if (fseek(file, 0, SEEK_SET) != 0){ 
    mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function fseek() was failed.");
    return 1; 
  }
  void *value;
  if (mlisp_read(file, machine, &value) != 0){ return 1; }
  if (mlisp_eval(value, machine, valuep) != 0){ return 1; }
  if (mlisp_decrease(value, machine) != 0){ return 1; }
  if (fclose(file) != 0){ 
    mlisp_error_set0(MLISP_INTERNAL_ERROR, "internal function fclose() was failed.");
    return 1; 
  }
  return 0;
}

int mlisp_eval_string0 (char *sequence, mlisp_machine *machine, void **valuep){
  size_t length = string_length(sequence);
  return mlisp_eval_string(sequence, length, machine, valuep);
}

static void print_error (FILE *file){
  int errorcode;
  char errormessage[MLISP_ERROR_INFO_MAX_LENGTH];
  mlisp_error_get(&errorcode, errormessage);
  fprintf(file, ">> REPL is aborted, because error was detected. <<\n");
  fprintf(file, ">> error code = %d: %s <<\n", errorcode, errormessage); 
}

int mlisp_repl (FILE *input, FILE *output, FILE *error, mlisp_machine *machine){
  while (true){
    void *value;
    int status = mlisp_read(input, machine, &value);
    if (status == MLISP_READ_SUCCESS){
      void *valueevaluated;
      if (mlisp_eval(value, machine, &valueevaluated) != 0){ print_error(error); return 1; }
      if (mlisp_println(valueevaluated, output, machine) != 0){ print_error(error); return 1; }
      if (mlisp_decrease(value, machine) != 0){ print_error(error); return 1; }
      if (mlisp_decrease(valueevaluated, machine) != 0){ print_error(error); return 1; }
    }
    else 
    if (status == MLISP_READ_EOF){
      return 0;
    }
    else 
    if (status == MLISP_READ_DOT){
      mlisp_error_set0(MLISP_ERROR, "read cons dot before open paren.");
      print_error(error);
      return 1;
    }
    else 
    if (status == MLISP_READ_CLOSE_PAREN){
      mlisp_error_set0(MLISP_ERROR, "read close paren before open paren.");
      print_error(error);
      return 1;
    }
    else {
      return 1;
    }
  }
  return 1; //unreachable!
}

// builtin syntax

static int make_function (mlisp_function_type type, mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args == NULL){ return 1; }
  void *a = args->car;
  void *forms = args->cdr;
  if (!listp(a, machine)){ return 1; }
  mlisp_symbol *progn = mlisp_allocate_symbol0("progn", machine);
  if (progn == NULL){ return 1; }
  mlisp_cons *prognforms = mlisp_allocate_cons(progn, forms, machine);
  if (prognforms == NULL){ return 1; }
  mlisp_user_function *function = mlisp_allocate_user_function(type, a, prognforms, machine);
  if (function == NULL){ return 1; }
  if (mlisp_decrease(progn, machine) != 0){ return 1; }
  if (mlisp_decrease(prognforms, machine) != 0){ return 1; }
  *valuep = function;
  return 0;
}

static int try_define_function (mlisp_function_type type, mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args == NULL){ return 1; }
  void *name = args->car;
  void *definations = args->cdr;
  if (mlisp_typep(MLISP_SYMBOL, name, machine)){
    void *function;
    if (make_function(type, definations, machine, &function) != 0){ return 1; }
    if (mlisp_scope_set(function, name, machine) != 0){ return 1; }
    *valuep = function;
    return 0;
  }
  else {
    return make_function(type, args, machine, valuep);
  }
}

static int __mlisp_function (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  return try_define_function(MLISP_FUNCTION, args, machine, valuep);
}

static int __mlisp_syntax (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  return try_define_function(MLISP_SYNTAX, args, machine, valuep);
}

static int __mlisp_macro (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  return try_define_function(MLISP_MACRO, args, machine, valuep);
}

static int __mlisp_progn (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *lastvalue = NULL;
  for (mlisp_cons *cons = args; cons != NULL; cons = cons->cdr){
    if (mlisp_decrease(lastvalue, machine) != 0){ return 1; }
    if (mlisp_eval(((mlisp_cons*)cons)->car, machine, &lastvalue) != 0){ return 1; }
  }
  *valuep = lastvalue;
  return 0;
}

static int __mlisp_if (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *cond;
  void *then;
  void *els;
  if (list_nth(0, args, &cond) != 0){ return 1; }
  if (list_nth(1, args, &then) != 0){ return 1; }
  if (list_nth(2, args, &els) != 0){ return 1; }
  void *condevaluated;
  void *conddereferenced;
  if (mlisp_eval(cond, machine, &condevaluated) != 0){ return 1; }
  if (mlisp_reference_get(condevaluated, machine, &conddereferenced) != 0){ return 1; }
  if (mlisp_decrease(conddereferenced, machine) != 0){ return 1; }
  return mlisp_eval(conddereferenced != NULL? then: els, machine, valuep);
}

static int __mlisp_while (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args == NULL){ return 1; }
  void *cond = args->car;
  void *forms = args->cdr;
  while (true){
    void *condevaluated;
    if (mlisp_eval(cond, machine, &condevaluated) != 0){ return 1; }
    if (mlisp_decrease(condevaluated, machine) != 0){ return 1; }
    if (condevaluated != NULL){
      void *formsevaluated;
      if (__mlisp_progn(forms, machine, &formsevaluated) != 0){ return 1; }
      if (mlisp_decrease(formsevaluated, machine) != 0){ return 1; }
    }
    else {
      break;
    }
  }
  *valuep = NULL;
  return 0;
}

static int __mlisp_var (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *name;
  void *form;
  void *formevaluated;
  if (list_nth(0, args, &name) != 0){ return 1; }
  if (list_nth(1, args, &form) != 0){ return 1; }
  if (mlisp_eval(form, machine, &formevaluated) != 0){ return 1; }
  if (!mlisp_typep(MLISP_SYMBOL, name, machine)){ return 1; }
  if (mlisp_scope_set(formevaluated, name, machine) != 0){ return 1; }
  if (mlisp_increase(formevaluated, machine) != 0){ return 1; }
  *valuep = formevaluated;
  return 0;
}

static int __mlisp_set (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *reference;
  void *form;
  void *referenceevaluated;
  void *formevaluated;
  if (list_nth(0, args, &reference) != 0){ return 1; }
  if (list_nth(1, args, &form) != 0){ return 1; }
  if (mlisp_eval(reference, machine, &referenceevaluated) != 0){ return 1; }
  if (mlisp_eval(form, machine, &formevaluated) != 0){ return 1; }
  if (!mlisp_referencep(referenceevaluated, machine)){ return 1; }
  if (mlisp_reference_set(formevaluated, referenceevaluated, machine) != 0){ return 1; }
  if (mlisp_decrease(referenceevaluated, machine) != 0){ return 1; }
  *valuep = formevaluated;
  return 0;
}

static int __mlisp_quote (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (mlisp_increase(value, machine) != 0){ return 1; }
  *valuep = value;
  return 0;
}

// builtin function

static int __mlisp_symbol (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (!listp(value, machine)){ return 1; }
  char buffer[MLISP_SYMBOL_MAX_LENGTH];
  size_t index;
  mlisp_cons *cons;
  for (index = 0, cons = value; index < sizeof(buffer) && cons != NULL; index++, cons = cons->cdr){
    if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
    buffer[index] = (int)*(mlisp_number*)(cons->car);
  }
  mlisp_symbol *symbol = mlisp_allocate_symbol(buffer, index, machine);
  if (symbol == NULL){ return 1; }
  *valuep = symbol;
  return 0;
}

static int __mlisp_symbol_name (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *symbol;
  if (list_nth(0, args, &symbol) != 0){ return 1; }
  if (!mlisp_typep(MLISP_SYMBOL, symbol, machine)){ return 1; }
  mlisp_cons *list = NULL;
  for (size_t index = 0; index < ((mlisp_symbol*)symbol)->length; index++){
    mlisp_number *number = mlisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = ((mlisp_symbol*)symbol)->characters[index];
    mlisp_cons *cons = mlisp_allocate_cons(number, list, machine);
    if (cons == NULL){ return 1; }
    if (mlisp_decrease(number, machine) != 0){ return 1; } 
    if (mlisp_decrease(list, machine) != 0){ return 1; } 
    list = cons;
  }
  *valuep = list_nreverse(list);
  return 0;
}

static int __mlisp_symbol_value (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *symbol;
  if (list_nth(0, args, &symbol) != 0){ return 1; }
  if (!mlisp_typep(MLISP_SYMBOL, symbol, machine)){ return 1; }
  mlisp_scope_reference *reference = mlisp_scope_get_reference(symbol, machine);
  if (reference == NULL){ return 1; }
  *valuep = reference;
  return 0;
}

static int __mlisp_cons (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *car;
  void *cdr;
  if (list_nth(0, args, &car) != 0){ return 1; }
  if (list_nth(1, args, &cdr) != 0){ return 1; }
  mlisp_cons *cons = mlisp_allocate_cons(car, cdr, machine);
  if (cons == NULL){ return 1; }
  *valuep = cons;
  return 0;
}

static int __mlisp_list (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  mlisp_cons *list = NULL;
  for (mlisp_cons *cons = args; cons != NULL; cons = cons->cdr){
    mlisp_cons *cn = mlisp_allocate_cons(cons->car, list, machine);
    if (cn == NULL){ return 1; }
    if (mlisp_decrease(list, machine) != 0){ return 1; } 
    list = cn;
  }
  *valuep = list_nreverse(list);
  return 0;
}

static int __mlisp_car (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *cons;
  if (list_nth(0, args, &cons) != 0){ return 1; }
  if (!mlisp_typep(MLISP_CONS, cons, machine)){ return 1; }
  mlisp_cons_reference *reference = mlisp_cons_get_reference(MLISP_CONS_CAR, cons, machine);
  if (reference == NULL){ return 1; }
  *valuep = reference;
  return 0;
}

static int __mlisp_cdr (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *cons;
  if (list_nth(0, args, &cons) != 0){ return 1; }
  if (!mlisp_typep(MLISP_CONS, cons, machine)){ return 1; }
  mlisp_cons_reference *reference = mlisp_cons_get_reference(MLISP_CONS_CDR, cons, machine);
  if (reference == NULL){ return 1; }
  *valuep = reference;
  return 0;
}

static int __mlisp_add (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  mlisp_number result = 0;
  for (mlisp_cons *cons = args; cons != NULL; cons = cons->cdr){
    if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
    result += *(mlisp_number*)(cons->car);
  }
  mlisp_number *resultp = mlisp_allocate_number(machine);
  if (resultp == NULL){ return 1; }
  *resultp = result;
  *valuep = resultp;
  return 0;
}

static int __mlisp_sub (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!mlisp_typep(MLISP_NUMBER, args->car, machine)){ return 1; }
    mlisp_number result = *(mlisp_number*)(args->car);
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
      result -= *(mlisp_number*)(cons->car);
    }
    mlisp_number *resultp = mlisp_allocate_number(machine);
    if (resultp == NULL){ return 1; }
    *resultp = result;
    *valuep = resultp;
    return 0;
  }
  else {
    return 1;
  }
}

static int __mlisp_mul (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!mlisp_typep(MLISP_NUMBER, args->car, machine)){ return 1; }
    mlisp_number result = *(mlisp_number*)(args->car);
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
      result *= *(mlisp_number*)(cons->car);
    }
    mlisp_number *resultp = mlisp_allocate_number(machine);
    if (resultp == NULL){ return 1; }
    *resultp = result;
    *valuep = resultp;
    return 0;
  }
  else {
    return 1;
  }
}

static int __mlisp_div (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!mlisp_typep(MLISP_NUMBER, args->car, machine)){ return 1; }
    mlisp_number result = *(mlisp_number*)(args->car);
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
      result /= *(mlisp_number*)(cons->car);
    }
    mlisp_number *resultp = mlisp_allocate_number(machine);
    if (resultp == NULL){ return 1; }
    *resultp = result;
    *valuep = resultp;
    return 0;
  }
  else {
    return 1;
  }
}

static int __mlisp_mod (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!mlisp_typep(MLISP_NUMBER, args->car, machine)){ return 1; }
    mlisp_number result = *(mlisp_number*)(args->car);
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
      result = result - floor(result / *(mlisp_number*)(cons->car));
    }
    mlisp_number *resultp = mlisp_allocate_number(machine);
    if (resultp == NULL){ return 1; }
    *resultp = result;
    *valuep = resultp;
    return 0;
  }
  else {
    return 1;
  }
}

static int __mlisp_lshift (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value1, machine)){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(mlisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(mlisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    mlisp_number *number = mlisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 << (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_rshift (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value1, machine)){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(mlisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(mlisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    mlisp_number *number = mlisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 >> (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_log_rshift (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value1, machine)){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(mlisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(mlisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    mlisp_number *number = mlisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (uintmax_t)integerpart1 >> (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_log_not (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value, machine)){ return 1; }
  double integerpart;
  double decimalpart = modf(*(mlisp_number*)value, &integerpart);
  if (abs(decimalpart) <= 0.0){
    mlisp_number *number = mlisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = ~(intmax_t)integerpart;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_log_and (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value1, machine)){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(mlisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(mlisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    mlisp_number *number = mlisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 & (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_log_or (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value1, machine)){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(mlisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(mlisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    mlisp_number *number = mlisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 | (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_log_xor (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value1, machine)){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(mlisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(mlisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    mlisp_number *number = mlisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 ^ (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_floor (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *number;
  if (list_nth(0, args, &number) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, number, machine)){ return 1; }
  mlisp_number *newnumber = mlisp_allocate_number(machine);
  if (newnumber == NULL){ return 1; }
  *newnumber = floor(*(mlisp_number*)number);
  *valuep = newnumber;
  return 0;
}

static int __mlisp_ceil (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *number;
  if (list_nth(0, args, &number) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, number, machine)){ return 1; }
  mlisp_number *newnumber = mlisp_allocate_number(machine);
  if (newnumber == NULL){ return 1; }
  *newnumber = ceil(*(mlisp_number*)number);
  *valuep = newnumber;
  return 0;
}

static int __mlisp_round (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *number;
  if (list_nth(0, args, &number) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, number, machine)){ return 1; }
  mlisp_number *newnumber = mlisp_allocate_number(machine);
  if (newnumber == NULL){ return 1; }
  *newnumber = round(*(mlisp_number*)number);
  *valuep = newnumber;
  return 0;
}

static bool equal (void*, void*, mlisp_machine*);

static bool equal_cons (mlisp_cons *cons1, mlisp_cons *cons2, mlisp_machine *machine){
  return equal(cons1->car, cons2->car, machine) && equal(cons1->cdr, cons2->cdr, machine);
}

static bool equal (void *value1, void *value2, mlisp_machine *machine){
  if (mlisp_typep(MLISP_NUMBER, value1, machine) && mlisp_typep(MLISP_NUMBER, value2, machine)){
    return *(mlisp_number*)value1 == *(mlisp_number*)value2;
  }
  else 
  if (mlisp_typep(MLISP_CONS, value1, machine) && mlisp_typep(MLISP_CONS, value2, machine)){
    return equal_cons(value1, value2, machine);
  }
  else {
    return value1 == value2;
  }
}

static int __mlisp_equal (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    void *first = args->car;
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!equal(first, cons->car, machine)){
        *valuep = MLISP_NIL;
        return 0;
      }
    }
    *valuep = MLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_unequal (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (__mlisp_equal(args, machine, &value) != 0){ return 1; }
  *valuep = value != MLISP_NIL? MLISP_NIL: MLISP_T;
  return 0;
}

static int __mlisp_less (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!mlisp_typep(MLISP_NUMBER, args->car, machine)){ return 1; }
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
      if (!(*(mlisp_number*)(args->car) < *(mlisp_number*)(cons->car))){
        *valuep = MLISP_NIL;
        return 0;
      }
    }
    *valuep = MLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_less_or_equal (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!mlisp_typep(MLISP_NUMBER, args->car, machine)){ return 1; }
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
      if (!(*(mlisp_number*)(args->car) <= *(mlisp_number*)(cons->car))){
        *valuep = MLISP_NIL;
        return 0;
      }
    }
    *valuep = MLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_great (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!mlisp_typep(MLISP_NUMBER, args->car, machine)){ return 1; }
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
      if (!(*(mlisp_number*)(args->car) > *(mlisp_number*)(cons->car))){
        *valuep = MLISP_NIL;
        return 0;
      }
    }
    *valuep = MLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_great_or_equal (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!mlisp_typep(MLISP_NUMBER, args->car, machine)){ return 1; }
    for (mlisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
      if (!(*(mlisp_number*)(args->car) >= *(mlisp_number*)(cons->car))){
        *valuep = MLISP_NIL;
        return 0;
      }
    }
    *valuep = MLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __mlisp_numberp (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = mlisp_typep(MLISP_NUMBER, value, machine)? MLISP_T: MLISP_NIL;
  return 0;
}

static int __mlisp_symbolp (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = mlisp_typep(MLISP_SYMBOL, value, machine)? MLISP_T: MLISP_NIL;
  return 0;
}

static int __mlisp_functionp (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = mlisp_functionp(value, machine)? MLISP_T: MLISP_NIL;
  return 0;
}

static int __mlisp_consp (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = mlisp_typep(MLISP_CONS, value, machine)? MLISP_T: MLISP_NIL;
  return 0;
}

static int __mlisp_listp (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = listp(value, machine)? MLISP_T: MLISP_NIL;
  return 0;
}

static int __mlisp_read (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (mlisp_read(stdin, machine, &value) != 0){ return 1; }
  *valuep = value;
  return 0;
}

static int __mlisp_read_string (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  size_t readsize;
  if (1 <= list_length(args)){
    void *size;
    if (list_nth(0, args, &size) != 0){ return 1; }
    if (!mlisp_typep(MLISP_NUMBER, size, machine)){ return 1; }
    readsize = *(mlisp_number*)size;
  }
  else {
    readsize = SIZE_MAX;
  }
  mlisp_cons *list = NULL;
  for (size_t index = 0; index < readsize; index++){
    int character = getc(stdin);
    if (character != EOF){
      mlisp_number *number = mlisp_allocate_number(machine);
      if (number == NULL){ return 1; }
      *number = character;
      mlisp_cons *cons = mlisp_allocate_cons(number, list, machine);
      if (cons == NULL){ return 1; }
      if (mlisp_decrease(number, machine) != 0){ return 1; } 
      if (mlisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
    }
    else {
      break;
    }
  }
  *valuep = list_nreverse(list);
  return 0;
}

static int __mlisp_read_line (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  size_t readsize;
  if (1 <= list_length(args)){
    void *size;
    if (list_nth(0, args, &size) != 0){ return 1; }
    if (!mlisp_typep(MLISP_NUMBER, size, machine)){ return 1; }
    readsize = *(mlisp_number*)size;
  }
  else {
    readsize = SIZE_MAX;
  }
  mlisp_cons *list = NULL;
  for (size_t index = 0; index < readsize; index++){
    int character = getc(stdin);
    if (character != EOF){
      mlisp_number *number = mlisp_allocate_number(machine);
      if (number == NULL){ return 1; }
      *number = character;
      mlisp_cons *cons = mlisp_allocate_cons(number, list, machine);
      if (cons == NULL){ return 1; }
      if (mlisp_decrease(number, machine) != 0){ return 1; } 
      if (mlisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
      if (character == '\n'){
        break;
      }
    }
    else {
      break;
    }
  }
  *valuep = list_nreverse(list);
  return 0;
}

static int __mlisp_eval (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  void *valueevaluated;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (mlisp_eval(value, machine, &valueevaluated) != 0){ return 1; }
  *valuep = valueevaluated;
  return 0;
}

static int __mlisp_eval_string (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *list;
  if (list_nth(0, args, &list) != 0){ return 1; }
  if (!listp(list, machine)){ return 1; }
  size_t length = list_length(list);
  char buffer[length];
  size_t index;
  mlisp_cons *cons;
  for (index = 0, cons = list; index < length && cons != NULL; index++, cons = cons->cdr){
    if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
    buffer[index] = *(mlisp_number*)(cons->car);
  }
  return mlisp_eval_string(buffer, length, machine, valuep);
}

static int __mlisp_print (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (mlisp_print(value, stdout, machine) != 0){ return 1; }
  if (mlisp_increase(value, machine) != 0){ return 1; }
  *valuep = value;
  return 0;
}

static int __mlisp_println (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (mlisp_println(value, stdout, machine) != 0){ return 1; }
  if (mlisp_increase(value, machine) != 0){ return 1; }
  *valuep = value;
  return 0;
}

static int __mlisp_write (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *list;
  if (list_nth(0, args, &list) != 0){ return 1; }
  if (!mlisp_typep(MLISP_CONS, list, machine)){ return 1; }
  for (mlisp_cons *cons = list; cons != NULL; cons = cons->cdr){
    void *consdereferenced;
    if (mlisp_reference_get(cons, machine, &consdereferenced) != 0){ return 1; }
    if (!mlisp_typep(MLISP_CONS, consdereferenced, machine)){ return 1; }
    if (!mlisp_typep(MLISP_NUMBER, ((mlisp_cons*)consdereferenced)->car, machine)){ return 1; }
    putchar((int)*(mlisp_number*)((mlisp_cons*)consdereferenced)->car);
  }
  return 0;
}

static int __mlisp_writeln (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  if (__mlisp_write(args, machine, valuep) != 0){ return 1; }
  putchar('\n');
  return 0;
}

static int __mlisp_error (mlisp_cons *args, mlisp_machine *machine, void **valuep){
  void *errorcode;
  void *errormessage;
  if (list_nth(0, args, &errorcode) != 0){ return 1; }
  if (list_nth(1, args, &errormessage) != 0){ return 1; }
  if (!mlisp_typep(MLISP_NUMBER, errorcode, machine)){ return 1; }
  if (!listp(errormessage, machine)){ return 1; }
  char buffer[MLISP_ERROR_INFO_MAX_LENGTH];
  size_t index;
  mlisp_cons *cons;
  for (index = 0, cons = errormessage; index < MLISP_ERROR_INFO_MAX_LENGTH && cons != NULL; index++, cons = cons->cdr){
    if (!mlisp_typep(MLISP_NUMBER, cons->car, machine)){ return 1; }
    buffer[index] = *(mlisp_number*)(cons->car);
  }
  mlisp_error_set(*(mlisp_number*)errorcode, buffer, index);
  return 1;
}

// setup syntax 

static int setup_builtin_syntax (mlisp_machine *machine){
  // define function 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("function", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_function, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define syntax 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("syntax", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_syntax, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define macro 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("macro", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_macro, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define progn 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("progn", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_progn, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define if 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("if", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_if, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define while
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("while", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_while, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define set
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("set", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_set, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define var
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("var", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_var, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define quote
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("quote", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_SYNTAX, __mlisp_quote, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  return 0;
}

// setup function 

static int setup_builtin_function (mlisp_machine *machine){
  // define symbol 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("symbol", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_symbol, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define symbol-name 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("symbol-name", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_symbol_name, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define symbol-value
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("symbol-value", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_symbol_value, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define cons
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("cons", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_cons, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define list
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("list", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_list, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define car
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("car", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_car, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define cdr
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("cdr", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_cdr, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define + 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("+", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_add, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define -
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("-", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_sub, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define *
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("*", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_mul, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define / 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("/", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_div, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define % 
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("%", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_mod, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define <<
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("<<", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_lshift, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define >>
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0(">>", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_rshift, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define >>>
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0(">>>", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_log_rshift, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define ~
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("~", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_log_not, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define &
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("&", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_log_and, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define |
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("|", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_log_or, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define ^
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("^", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_log_xor, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define floor
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("floor", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_floor, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define ceil
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("ceil", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_ceil, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define round
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("round", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_round, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define ==
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("==", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_equal, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define !=
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("!=", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_unequal, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define <
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("<", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_less, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define <=
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("<=", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_less_or_equal, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define >
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0(">", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_great, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define >=
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0(">=", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_great_or_equal, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define numberp
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("numberp", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_numberp, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define symbolp
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("symbolp", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_symbolp, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define functionp
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("functionp", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_functionp, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define consp
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("consp", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_consp, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define listp
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("listp", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_listp, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define read
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("read", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_read, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define read-string
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("read-string", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_read_string, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define read-line
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("read-line", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_read_line, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define eval
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("eval", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_eval, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define eval-string
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("eval-string", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_eval_string, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define print
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("print", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_print, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define println
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("println", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_println, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define write
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("write", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_write, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define writeln
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("writeln", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_writeln, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define error
  {
    mlisp_symbol *symbol = mlisp_allocate_symbol0("error", machine);
    if (symbol == NULL){ return 1; }
    mlisp_c_function *function = mlisp_allocate_c_function(MLISP_FUNCTION, __mlisp_error, machine);
    if (function == NULL){ return 1; }
    if (mlisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(symbol, machine) != 0){ return 1; }
    if (mlisp_decrease(function, machine) != 0){ return 1; }
  }
  // define not 
  {
    char source[] = "(function not (any) (if any nil t))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define null 
  {
    char source[] = "(var null not)";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define length
  {
    char source[] = "(function length (lst) (if (null lst) 0 (+ (length (cdr lst)) 1)))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define nth
  {
    char source[] = "(function nth (index lst) (if (null lst) nil (if (<= index 0) (car lst) (nth (- index 1) (cdr lst)))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define map
  {
    char source[] = "(function map (fn lst) (if (null lst) nil (cons (fn (car lst)) (map fn (cdr lst)))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define filter
  {
    char source[] = "(function filter (fn lst) (if (null lst) nil (if (fn (car lst)) (cons (car lst) (filter fn (cdr lst))) (filter fn (cdr lst)))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define --reduce
  {
    char source[] = "(function --reduce (fn acc lst) (if (null lst) acc (--reduce fn (fn acc (car lst)) (cdr lst))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define reduce
  {
    char source[] = "(function reduce (fn lst) (if (null lst) nil (--reduce fn (car lst) (cdr lst))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define find
  {
    char source[] = "(function find (fn lst) (if (null lst) nil (if (fn (car lst)) (car lst) (find fn (cdr lst)))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  return 0;
}

// setup macro 

static int setup_builtin_macro (mlisp_machine *machine){
  // define until 
  {
    char source[] = "(macro until (cond &rest forms) (cons 'while (cons (list 'not cond) forms)))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define when
  {
    char source[] = "(macro when (cond &rest forms) (list 'if cond (cons 'progn forms) nil))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define unless 
  {
    char source[] = "(macro unless (cond &rest forms) (list 'if cond nil (cons 'progn forms)))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define and 
  {
    char source[] = "(macro and (&rest forms) (if (null forms) t (if (null (cdr forms)) (car forms) (list 'if (car forms) (cons 'and (cdr forms)) nil))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define or 
  {
    char source[] = "(macro or (&rest forms) (if (null forms) nil (list 'progn (list 'var '--temp (car forms)) (list 'if '--temp '--temp (cons 'or (cdr forms))))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  // define cond 
  {
    char source[] = "(macro cond (&rest conds) (if (null conds) nil (list 'if (car (car conds)) (cons 'progn (cdr (car conds))) (cons 'cond (cdr conds)))))";
    void *value;
    if (mlisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (mlisp_decrease(value, machine) != 0){ return 1; }
  }
  return 0;
}

// mlisp 

int mlisp_open (mlisp_machine *machine){
  mlisp_init(machine);
  if (mlisp_scope_begin(machine) != 0){ return 1; }
  return 0;
}

int mlisp_load_library (mlisp_machine *machine){
  if (setup_builtin_syntax(machine) != 0){ return 1; }
  if (setup_builtin_function(machine) != 0){ return 1; }
  if (setup_builtin_macro(machine) != 0){ return 1; }
  return 0;
}

int mlisp_close (mlisp_machine *machine){
  mlisp_memory_free(&(machine->memory));
  return 0;
}
