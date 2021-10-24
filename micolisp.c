
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "memnode.h"
#include "hashset.h"
#include "hashtable.h"
#include "cgcmemnode.h"
#include "micolisp.h"

#ifndef MAX 
#define MAX(a, b) ((a)<(b)?(b):(a))
#endif 

// hashset

static bool micolisp_symbol_equal (micolisp_symbol*, micolisp_symbol*);

static size_t __hashset_hash (void *symbol, void *arg){
  return ((micolisp_symbol*)symbol)->hash;
}

static bool __hashset_compare (void *symbol1, void *symbol2, void *arg){
  return micolisp_symbol_equal(symbol1, symbol2);
}

static const hashset_class __MICOLISP_HASHSET_CLASS = { __hashset_hash, __hashset_compare, NULL, NULL };
static const hashset_class *MICOLISP_HASHSET_CLASS = &__MICOLISP_HASHSET_CLASS;

// hashtable

static size_t __hashtable_hash (void *symbol, void *arg){
  return ((micolisp_symbol*)symbol)->hash;
}

static bool __hashtable_compare (void *symbol1, void *symbol2, void *arg){
  return symbol1 == symbol2;
}

static const hashtable_class __MICOLISP_HASHTABLE_CLASS = { __hashtable_hash, __hashtable_compare, NULL, NULL };
static const hashtable_class *MICOLISP_HASHTABLE_CLASS = &__MICOLISP_HASHTABLE_CLASS;

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

static int listp (micolisp_cons *list, micolisp_machine *machine){
  for (micolisp_cons *cons = list; cons != NULL; cons = cons->cdr){
    if (!micolisp_typep(MICOLISP_CONS, cons, machine)){ return false; }
  }
  return true;
}

static int list_nth (size_t index, micolisp_cons *list, void **valuep){
  size_t i;
  micolisp_cons *cons;
  for (i = 0, cons = list; i < index && cons != NULL; i++, cons = cons->cdr);
  if (i == index && cons != NULL){
    *valuep = cons->car;
    return 0;
  }
  else {
    return 1;
  }
}

static size_t list_length (micolisp_cons *cons){
  size_t length = 0;
  for (micolisp_cons *cn = cons; cn != NULL; cn = cn->cdr){
    length++;
  }
  return length;
}

static micolisp_cons *list_nreverse (micolisp_cons *list){
  micolisp_cons *consprevious = NULL;
  while (list != NULL){
    micolisp_cons *consnext = list->cdr;
    list->cdr = consprevious;
    consprevious = list;
    list = consnext;
  }
  return consprevious;
}

// error info 

_Thread_local micolisp_error_info micolisp_error = { MICOLISP_NOERROR, {} };

void micolisp_error_set (int code, char *message, size_t messagelen){
  micolisp_error.code = code;
  if (MICOLISP_ERROR_INFO_MAX_LENGTH <= messagelen){
    copy(message, MICOLISP_ERROR_INFO_MAX_LENGTH, micolisp_error.message);
    micolisp_error.message[MICOLISP_ERROR_INFO_MAX_LENGTH -1] = '\0';
    micolisp_error.message[MICOLISP_ERROR_INFO_MAX_LENGTH -2] = '.';
    micolisp_error.message[MICOLISP_ERROR_INFO_MAX_LENGTH -3] = '.';
    micolisp_error.message[MICOLISP_ERROR_INFO_MAX_LENGTH -4] = '.';
  }
  else {
    copy(message, messagelen, micolisp_error.message);
    micolisp_error.message[messagelen] = '\0';
  }
}

void micolisp_error_set0 (int code, char *message){
  size_t length = string_length(message);
  micolisp_error_set(code, message, length);
}

void micolisp_error_get (int *codep, char *messagep){
  if (micolisp_error.code == MICOLISP_NOERROR){
    *codep = MICOLISP_NOERROR;
    char noerrormessage[] = "no error recorded.";
    copy(noerrormessage, sizeof(noerrormessage), messagep);
  }
  else {
    *codep = micolisp_error.code;
    copy(micolisp_error.message, MICOLISP_ERROR_INFO_MAX_LENGTH, messagep);
  }
}

// number 

micolisp_number *micolisp_allocate_number (micolisp_machine *machine){
  return micolisp_allocate(MICOLISP_NUMBER, sizeof(micolisp_number), machine);
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

static int micolisp_symbol_init (char *characters, size_t length, micolisp_symbol *symbol){
  if (length <= MICOLISP_SYMBOL_MAX_LENGTH){
    copy(characters, length, symbol->characters);
    symbol->length = length;
    symbol->hash = calculate_hash(characters, length);
    return 0;
  }
  else {
    micolisp_error_set0(MICOLISP_VALUE_ERROR, "symbol name's length must be less than MICOLISP_ERROR_INFO_MAX_LENGTH."); 
    return 1; 
  }
}

micolisp_symbol *micolisp_allocate_symbol (char *characters, size_t length, micolisp_machine *machine){
  micolisp_symbol symbol;
  if (micolisp_symbol_init(characters, length, &symbol) != 0){ return NULL; }
  void *foundsymbol;
  if (hashset_get(&symbol, &(machine->symbol), &foundsymbol) != 0){
    micolisp_symbol *sym = micolisp_allocate(MICOLISP_SYMBOL, sizeof(micolisp_symbol), machine);
    if (sym == NULL){ return NULL; }
    *sym = symbol;
    if (hashset_add(sym, &(machine->symbol)) != 0){
      size_t newlen = MAX(8, machine->symbol.length * 2);
      hashset_entry *newentries = micolisp_allocate(MICOLISP_HASHSET_ENTRY, newlen * sizeof(hashset_entry), machine);
      if (newentries == NULL){ return NULL; }
      if (hashset_stretch(newentries, newlen, &(machine->symbol)) != 0){ 
        micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function hashset_stretch() was failed."); 
        return NULL; 
      }
      if (hashset_add(sym, &(machine->symbol)) != 0){ 
        micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function hashset_add() was failed."); 
        return NULL; 
      }
      return sym;
    }
    else {
      return sym;
    }
  }
  else {
    if (micolisp_increase(foundsymbol, machine) != 0){ return NULL; }
    return foundsymbol;
  }
}

micolisp_symbol *micolisp_allocate_symbol0 (char *characters, micolisp_machine *machine){
  size_t length = string_length(characters);
  return micolisp_allocate_symbol(characters, length, machine);
}

static bool micolisp_symbol_equal (micolisp_symbol *symbol1, micolisp_symbol *symbol2){
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

static void micolisp_function_init (micolisp_function_type, micolisp_function*);

micolisp_c_function *micolisp_allocate_c_function (micolisp_function_type type, micolisp_c_function_main main, micolisp_machine *machine){
  micolisp_c_function *function = micolisp_allocate(MICOLISP_C_FUNCTION, sizeof(micolisp_c_function), machine);
  if (function == NULL){ return NULL; }
  micolisp_function_init(type, &(function->function));
  function->main = main;
  return function;
}

static int micolisp_c_function_call (micolisp_cons *args, micolisp_c_function *function, micolisp_machine *machine, void **valuep){
  return function->main(args, machine, valuep);
}

// user-function

static void micolisp_function_init (micolisp_function_type, micolisp_function*);
static int micolisp_scope_begin (micolisp_machine*);
static int micolisp_scope_end (micolisp_machine*);

micolisp_user_function *micolisp_allocate_user_function (micolisp_function_type type, micolisp_cons *args, micolisp_cons *form, micolisp_machine *machine){
  void *argsdereferenced;
  void *formdereferenced;
  if (micolisp_reference_get(args, machine, &argsdereferenced) != 0){ return NULL; }
  if (micolisp_reference_get(form, machine, &formdereferenced) != 0){ return NULL; }
  if (!listp(argsdereferenced, machine)){
    micolisp_error_set0(MICOLISP_TYPE_ERROR, "args must be a list.");
    return NULL; 
  }
  if (micolisp_increase(argsdereferenced, machine) != 0){ return NULL; }
  if (micolisp_increase(formdereferenced, machine) != 0){ return NULL; }
  micolisp_user_function *function = micolisp_allocate(MICOLISP_USER_FUNCTION, sizeof(micolisp_user_function), machine);
  if (function == NULL){ return NULL; }
  micolisp_function_init(type, &(function->function));
  function->args = argsdereferenced;
  function->form = formdereferenced;
  return function;
}

static int micolisp_user_function_call (micolisp_cons *args, micolisp_user_function *function, micolisp_machine *machine, void **valuep){
  if (micolisp_scope_begin(machine) != 0){ return 1; }
  micolisp_symbol *andrest = micolisp_allocate_symbol0("&rest", machine);
  if (andrest == NULL){ return 1; }
  micolisp_cons *afrom;
  micolisp_cons *ato;
  for (afrom = args, ato = function->args; ato != NULL; afrom = afrom->cdr, ato = ato->cdr){
    if (afrom != NULL){ 
      if (ato->car == andrest){
        if (ato->cdr != NULL){
          if (micolisp_scope_set(afrom, ((micolisp_cons*)(ato->cdr))->car, machine) != 0){ return 1; }
          break;
        }
        else {
          micolisp_error_set0(MICOLISP_VALUE_ERROR, "need a symbol after &rest keyword."); 
          return 1;
        }
      }
      else {
        if (micolisp_scope_set(afrom->car, ato->car, machine) != 0){ return 1; }
      }
    }
    else {
      micolisp_error_set0(MICOLISP_ERROR, "given not enough argument."); 
      return 1; 
    }
  }
  if (micolisp_decrease(andrest, machine) != 0){ return 1; }
  int status = micolisp_eval(function->form, machine, valuep);
  if (micolisp_scope_end(machine) != 0){ return 1; }
  return status;
}

// function 

static void micolisp_function_init (micolisp_function_type type, micolisp_function *function){
  function->type = type;
}

bool micolisp_functionp (void *function, micolisp_machine *machine){
  return micolisp_typep(MICOLISP_C_FUNCTION, function, machine) || micolisp_typep(MICOLISP_USER_FUNCTION, function, machine);
}

static int eval_args (micolisp_cons *args, micolisp_machine *machine, micolisp_cons **argsevaluatedp){
  micolisp_cons *argsevaluated = NULL;
  for (micolisp_cons *cons = args; cons != NULL; cons = cons->cdr){
    void *evaluated;
    if (micolisp_eval(cons->car, machine, &evaluated) != 0){ return 1; }
    micolisp_cons *cn = micolisp_allocate_cons(evaluated, argsevaluated, machine);
    if (cn == NULL){ return 1; }
    if (micolisp_decrease(evaluated, machine) != 0){ return 1; }
    if (micolisp_decrease(argsevaluated, machine) != 0){ return 1; } 
    argsevaluated = cn;
  }
  *argsevaluatedp = list_nreverse(argsevaluated);
  return 0;
}

static int micolisp_function_call (micolisp_cons *args, void *function, micolisp_machine *machine, void **valuep){
  if (micolisp_functionp(function, machine)){
    micolisp_cons *newargs;
    if (((micolisp_function*)function)->type == MICOLISP_FUNCTION){
      if (eval_args(args, machine, &newargs) != 0){ return 1; }
    }
    else {
      if (micolisp_increase(args, machine) != 0){ return 1; }
      newargs = args;
    }
    int status;
    void *value;
    if (micolisp_typep(MICOLISP_C_FUNCTION, function, machine)){
      status = micolisp_c_function_call(newargs, function, machine, &value);
    }
    else 
    if (micolisp_typep(MICOLISP_USER_FUNCTION, function, machine)){
      status = micolisp_user_function_call(newargs, function, machine, &value);
    }
    else {
      micolisp_error_set0(MICOLISP_TYPE_ERROR, "tried calling a non function."); 
      return 1; 
    }
    void *newvalue;
    if (((micolisp_function*)function)->type == MICOLISP_MACRO){
      if (micolisp_eval(value, machine, &newvalue) != 0){ return 1; }
    }
    else {
      if (micolisp_increase(value, machine) != 0){ return 1; }
      newvalue = value;
    }
    if (micolisp_decrease(newargs, machine) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
    *valuep = newvalue;
    return status;
  }
  else {
    micolisp_error_set0(MICOLISP_TYPE_ERROR, "tried calling a non function."); 
    return 1;
  }
}

// quote 

micolisp_cons *micolisp_quote (void *value, micolisp_machine *machine){
  micolisp_cons *cons2 = micolisp_allocate_cons(value, NULL, machine);
  if (cons2 == NULL){ return NULL; }
  micolisp_symbol *quote = micolisp_allocate_symbol0("quote", machine);
  if (quote == NULL){ return NULL; }
  micolisp_cons *cons1 = micolisp_allocate_cons(quote, cons2, machine);
  if (cons1 == NULL){ return NULL; }
  if (micolisp_decrease(cons2, machine) != 0){ return NULL; }
  if (micolisp_decrease(quote, machine) != 0){ return NULL; }
  return cons1;
}

// cons 

micolisp_cons *micolisp_allocate_cons (void *car, void *cdr, micolisp_machine *machine){
  void *cardereferenced;
  void *cdrdereferenced;
  if (micolisp_reference_get(car, machine, &cardereferenced) != 0){ return NULL; }
  if (micolisp_reference_get(cdr, machine, &cdrdereferenced) != 0){ return NULL; }
  if (micolisp_increase(cardereferenced, machine) != 0){ return NULL; }
  if (micolisp_increase(cdrdereferenced, machine) != 0){ return NULL; }
  micolisp_cons *cons = micolisp_allocate(MICOLISP_CONS, sizeof(micolisp_cons), machine);
  if (cons == NULL){ return NULL; }
  cons->car = cardereferenced;
  cons->cdr = cdrdereferenced;
  return cons;
}

int micolisp_cons_set (void *value, micolisp_cons_whence whence, micolisp_cons *cons, micolisp_machine *machine){
  void *valuedereferenced;
  if (micolisp_reference_get(value, machine, &valuedereferenced) != 0){ return 1; }
  switch (whence){
    case MICOLISP_CONS_CAR:
      if (micolisp_increase(valuedereferenced, machine) != 0){ return 1; }
      if (micolisp_decrease(cons->car, machine) != 0){ return 1; }
      cons->car = value;
      return 0;
    case MICOLISP_CONS_CDR:
      if (micolisp_increase(valuedereferenced, machine) != 0){ return 1; }
      if (micolisp_decrease(cons->cdr, machine) != 0){ return 1; }
      cons->cdr = value;
      return 0;
    default:
      micolisp_error_set0(MICOLISP_VALUE_ERROR, "given an unknown whence."); 
      return 1;
  }
}

int micolisp_cons_get (micolisp_cons_whence whence, micolisp_cons *cons, void **valuep){
  switch (whence){
    case MICOLISP_CONS_CAR: 
      *valuep = cons->car; 
      return 0;
    case MICOLISP_CONS_CDR: 
      *valuep = cons->cdr; 
      return 0;
    default: 
      micolisp_error_set0(MICOLISP_VALUE_ERROR, "given an unknown whence."); 
      return 1;
  }
}

micolisp_cons_reference *micolisp_cons_get_reference (micolisp_cons_whence whence, micolisp_cons *cons, micolisp_machine *machine){
  if (micolisp_increase(cons, machine) != 0){ return NULL; }
  micolisp_cons_reference *reference = micolisp_allocate(MICOLISP_CONS_REFERENCE, sizeof(micolisp_cons_reference), machine);
  if (reference == NULL){ return NULL; }
  reference->whence = whence;
  reference->cons = cons;
  return reference;
}

int micolisp_cons_reference_set (void *value, micolisp_cons_reference *reference, micolisp_machine *machine){
  return micolisp_cons_set(value, reference->whence, reference->cons, machine);
}

int micolisp_cons_reference_get (micolisp_cons_reference *reference, void **valuep){
  return micolisp_cons_get(reference->whence, reference->cons, valuep);
}

// scope 

int micolisp_scope_set (void *value, micolisp_symbol *name, micolisp_machine *machine){
  void *valuedereferenced;
  void *namedereferenced;
  if (micolisp_reference_get(value, machine, &valuedereferenced) != 0){ return 1; }
  if (micolisp_reference_get(name, machine, &namedereferenced) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_SYMBOL, namedereferenced, machine)){ 
    micolisp_error_set0(MICOLISP_TYPE_ERROR, "name must be a symbol.");
    return 1; 
  }
  if (machine->scope != NULL){
    if (micolisp_increase(valuedereferenced, machine) != 0){ return 1; }
    void *foundvalue;
    if (hashtable_get(namedereferenced, &(machine->scope->hashtable), &foundvalue) == 0){
      if (micolisp_decrease(foundvalue, machine) != 0){ return 1; }
      if (hashtable_set(valuedereferenced, namedereferenced, &(machine->scope->hashtable)) != 0){ 
        micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function hashtable_set() was failed.");
        return 1; 
      }
      return 0;
    }
    else {
      if (micolisp_increase(namedereferenced, machine) != 0){ return 1; }
      if (hashtable_set(valuedereferenced, namedereferenced, &(machine->scope->hashtable)) != 0){
        size_t newlen = MAX(8, machine->scope->hashtable.length * 2);
        hashtable_entry *newentries = micolisp_allocate(MICOLISP_HASHTABLE_ENTRY, newlen * sizeof(hashtable_entry), machine);
        if (newentries == NULL){ return 1; }
        if (hashtable_stretch(newentries, newlen, &(machine->scope->hashtable)) != 0){ 
          micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function hashtable_stretch() was failed.");
          return 1; 
        }
        if (hashtable_set(valuedereferenced, namedereferenced, &(machine->scope->hashtable)) != 0){
          micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function hashtable_set() was failed.");
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
    micolisp_error_set0(MICOLISP_ERROR, "no assignable scope, because scope is nil.");
    return 1;
  }
}

int micolisp_scope_get (micolisp_symbol *name, micolisp_machine *machine, void **valuep){
  void *namedereferenced;
  if (micolisp_reference_get(name, machine, &namedereferenced) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_SYMBOL, namedereferenced, machine)){ 
    micolisp_error_set0(MICOLISP_TYPE_ERROR, "name must be a symbol.");
    return 1; 
  }
  for (micolisp_scope *scope = machine->scope; scope != NULL; scope = scope->parent){
    if (hashtable_get(namedereferenced, &(scope->hashtable), valuep) == 0){ return 0; }
  }
  micolisp_error_set0(MICOLISP_ERROR, "could not find name in the scope.");
  return 1;
}

micolisp_scope_reference *micolisp_scope_get_reference (micolisp_symbol *name, micolisp_machine *machine){
  void *namedereferenced;
  if (micolisp_scope_get(name, machine, &namedereferenced) != 0){ return NULL; }
  if (!micolisp_typep(MICOLISP_SYMBOL, namedereferenced, machine)){ 
    micolisp_error_set0(MICOLISP_TYPE_ERROR, "name must be a symbol.");
    return NULL; 
  }
  if (micolisp_increase(namedereferenced, machine) != 0){ return NULL; }
  if (micolisp_increase(machine->scope, machine) != 0){ return NULL; }
  micolisp_scope_reference *reference = micolisp_allocate(MICOLISP_SCOPE_REFERENCE, sizeof(micolisp_scope_reference), machine);
  if (reference == NULL){ return NULL; }
  reference->name = namedereferenced;
  reference->scope = machine->scope;
  return reference;
}

int micolisp_scope_reference_set (void *value, micolisp_scope_reference *reference, micolisp_machine *machine){
  void *valuedereferenced;
  if (micolisp_reference_get(value, machine, &valuedereferenced) != 0){ return 1; }
  if (micolisp_increase(valuedereferenced, machine) != 0){ return 1; }
  for (micolisp_scope *scope = reference->scope; scope != NULL; scope = scope->parent){
    void *foundvalue;
    if (hashtable_get(reference->name, &(scope->hashtable), &foundvalue) == 0){
      if (micolisp_decrease(foundvalue, machine) != 0){ return 1; }
      if (hashtable_set(valuedereferenced, reference->name, &(scope->hashtable)) != 0){ 
        micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function hashtable_set() was failed.");
        return 1; 
      }
      return 0;
    }
  }
  if (micolisp_increase(reference->name, machine) != 0){ return 1; }
  if (hashtable_set(valuedereferenced, reference->name, &(reference->scope->hashtable)) != 0){
    size_t newlen = machine->scope->hashtable.length * 2;
    hashtable_entry *newentries = micolisp_allocate(MICOLISP_HASHTABLE_ENTRY, newlen * sizeof(MICOLISP_HASHTABLE_ENTRY), machine);
    if (newentries == NULL){ return 1; }
    if (hashtable_stretch(newentries, newlen, &(reference->scope->hashtable)) != 0){ 
      micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function hashtable_stretch() was failed.");
      return 1;
    }
    if (hashtable_set(valuedereferenced, reference->name, &(reference->scope->hashtable)) != 0){ 
      micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function hashtable_set() was failed.");
      return 1;
    }
    return 0;
  }
  else {
    return 0;
  }
}

int micolisp_scope_reference_get (micolisp_scope_reference *reference, void **valuep){
  for (micolisp_scope *scope = reference->scope; scope != NULL; scope = scope->parent){
    if (hashtable_get(reference->name, &(scope->hashtable), valuep) == 0){ return 0; }
  }
  micolisp_error_set0(MICOLISP_ERROR, "could not find name in the scope.");
  return 1;
}

static int micolisp_scope_begin (micolisp_machine *machine){
  if (micolisp_increase(machine->scope, machine) != 0){ return 1; }
  micolisp_scope *scope = micolisp_allocate(MICOLISP_SCOPE, sizeof(micolisp_scope), machine);
  if (scope == NULL){ return 1; }
  hashtable_entry *scopeentries = micolisp_allocate(MICOLISP_HASHTABLE_ENTRY, 8 * sizeof(hashtable_entry), machine);
  if (scopeentries == NULL){ return 1; }
  hashtable_init(scopeentries, 8, MICOLISP_HASHTABLE_CLASS, &(scope->hashtable));
  scope->parent = machine->scope;
  machine->scope = scope;
  return 0;
}

static int micolisp_scope_end (micolisp_machine *machine){
  if (machine->scope != NULL){
    micolisp_scope *parent = machine->scope->parent;
    if (micolisp_decrease(machine->scope, machine) != 0){ return 1; }
    machine->scope = parent;
    return 0;
  }
  else {
    micolisp_error_set0(MICOLISP_ERROR, "could not back to previous scope, because scope is nil.");
    return 1;
  }
}

// reference 

bool micolisp_referencep (void *address, micolisp_machine *machine){
  return micolisp_typep(MICOLISP_CONS_REFERENCE, address, machine) || micolisp_typep(MICOLISP_SCOPE_REFERENCE, address, machine);
}

int micolisp_reference_set (void *value, void *reference, micolisp_machine *machine){
  if (micolisp_typep(MICOLISP_CONS_REFERENCE, reference, machine)){
    return micolisp_cons_reference_set(value, reference, machine);
  }
  else 
  if (micolisp_typep(MICOLISP_SCOPE_REFERENCE, reference, machine)){
    return micolisp_scope_reference_set(value, reference, machine);
  }
  else {
    micolisp_error_set0(MICOLISP_VALUE_ERROR, "tried assigning value to a non reference.");
    return 1;
  }
}

int micolisp_reference_get (void *mayreference, micolisp_machine *machine, void **valuep){
  if (micolisp_typep(MICOLISP_CONS_REFERENCE, mayreference, machine)){
    return micolisp_cons_reference_get(mayreference, valuep);
  }
  else 
  if (micolisp_typep(MICOLISP_SCOPE_REFERENCE, mayreference, machine)){
    return micolisp_scope_reference_get(mayreference, valuep);
  }
  else {
    *valuep = mayreference;
    return 0;
  }
}

// memory

static void micolisp_memory_init (micolisp_memory *memory){
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

static void micolisp_memory_free (micolisp_memory *memory){
  free_cgcmemnode_all(memory->number);
  free_cgcmemnode_all(memory->symbol);
  free_cgcmemnode_all(memory->cons);
  free_cgcmemnode_all(memory->consreference);
  free_cgcmemnode_all(memory->cfunction);
  free_cgcmemnode_all(memory->userfunction);
  free_cgcmemnode_all(memory->scope);
  free_cgcmemnode_all(memory->scopereference);
  free_cgcmemnode_all(memory->hashtableentry);
  free_cgcmemnode_all(memory->hashsetentry);
}

static micolisp_memory_type micolisp_memory_info (micolisp_memory_type type, micolisp_memory *memory, size_t *sizep, cgcmemnode **cmemnodep, cgcmemnode ***cmemnodepp){
  switch (type){
    case MICOLISP_NUMBER:
      *cmemnodep = memory->number;
      *cmemnodepp = &(memory->number);
      *sizep = sizeof(micolisp_number);
      return 0;
    case MICOLISP_SYMBOL:
      *cmemnodep = memory->symbol;
      *cmemnodepp = &(memory->symbol);
      *sizep = sizeof(micolisp_symbol);
      return 0;
    case MICOLISP_CONS:
      *cmemnodep = memory->cons;
      *cmemnodepp = &(memory->cons);
      *sizep = sizeof(micolisp_cons);
      return 0;
    case MICOLISP_CONS_REFERENCE:
      *cmemnodep = memory->consreference;
      *cmemnodepp = &(memory->consreference);
      *sizep = sizeof(micolisp_cons_reference);
      return 0;
    case MICOLISP_C_FUNCTION:
      *cmemnodep = memory->cfunction;
      *cmemnodepp = &(memory->cfunction);
      *sizep = sizeof(micolisp_c_function);
      return 0;
    case MICOLISP_USER_FUNCTION:
      *cmemnodep = memory->userfunction;
      *cmemnodepp = &(memory->userfunction);
      *sizep = sizeof(micolisp_user_function);
      return 0;
    case MICOLISP_SCOPE:
      *cmemnodep = memory->scope;
      *cmemnodepp = &(memory->scope);
      *sizep = sizeof(micolisp_scope);
      return 0;
    case MICOLISP_SCOPE_REFERENCE:
      *cmemnodep = memory->scopereference;
      *cmemnodepp = &(memory->scopereference);
      *sizep = sizeof(micolisp_scope_reference);
      return 0;
    case MICOLISP_HASHTABLE_ENTRY:
      *cmemnodep = memory->hashtableentry;
      *cmemnodepp = &(memory->hashtableentry);
      *sizep = sizeof(hashtable_entry);
      return 0;
    case MICOLISP_HASHSET_ENTRY:
      *cmemnodep = memory->hashsetentry;
      *cmemnodepp = &(memory->hashsetentry);
      *sizep = sizeof(hashset_entry);
      return 0;
    default:
      micolisp_error_set0(MICOLISP_VALUE_ERROR, "given an unknown type.");
      return 1;
  }
}

static bool micolisp_memory_typep (micolisp_memory_type type, void *address, micolisp_memory *memory){
  size_t basesize;
  cgcmemnode *cmemnode;
  cgcmemnode **cmemnodep;
  if (micolisp_memory_info(type, memory, &basesize, &cmemnode, &cmemnodep) != 0){ return false; }
  return memnode_find(address, cgcmemnode_memnode(cmemnode)) != NULL;
}

static void *micolisp_memory_allocate (micolisp_memory_type type, size_t size, micolisp_memory *memory){
  size_t basesize;
  cgcmemnode *cmemnode;
  cgcmemnode **cmemnodep;
  if (micolisp_memory_info(type, memory, &basesize, &cmemnode, &cmemnodep) != 0){ return NULL; }
  void *address = cgcmemnode_allocate(size, cmemnode);
  if (address == NULL){
    micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function cgcmemnode_allocate() was failed.");
    return NULL;
  }
  return address;
}

static int micolisp_memory_increase (micolisp_memory_type type, void *address, size_t size, micolisp_memory *memory){
  size_t basesize;
  cgcmemnode *cmemnode;
  cgcmemnode **cmemnodep;
  if (micolisp_memory_info(type, memory, &basesize, &cmemnode, &cmemnodep) != 0){ return 1; }
  if (cgcmemnode_increase(address, size, cmemnode) != 0){
    micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function cgcmemnode_increase() was failed.");
    return 1;
  }
  return 0;
}

static int micolisp_memory_decrease (micolisp_memory_type type, void *address, size_t size, micolisp_memory *memory){
  size_t basesize;
  cgcmemnode *cmemnode;
  cgcmemnode **cmemnodep;
  if (micolisp_memory_info(type, memory, &basesize, &cmemnode, &cmemnodep) != 0){ return 1; }
  if (cgcmemnode_decrease(address, size, cmemnode) != 0){
    micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function cgcmemnode_decrease() was failed.");
    return 1;
  }
  return 0;
}

// machine

void micolisp_init (micolisp_machine *machine){
  micolisp_memory_init(&(machine->memory));
  machine->scope = NULL;
  hashset_init(NULL, 0, MICOLISP_HASHSET_CLASS, &(machine->symbol));
} 

bool micolisp_typep (micolisp_memory_type type, void *address, micolisp_machine *machine){
  return micolisp_memory_typep(type, address, &(machine->memory));
}

static size_t align_size (size_t size, size_t alignment){
  return (size / alignment * alignment) + (0 < size % alignment? alignment: 0);
}

void *micolisp_allocate (micolisp_memory_type type, size_t size, micolisp_machine *machine){
  void *address = micolisp_memory_allocate(type, size, &(machine->memory));
  if (address == NULL){
    size_t basesize;
    cgcmemnode *cmemnode;
    cgcmemnode **cmemnodep;
    if (micolisp_memory_info(type, &(machine->memory), &basesize, &cmemnode, &cmemnodep) != 0){ return NULL; }
    size_t newsize = align_size(size, 4096);
    cgcmemnode *newcmemnode = make_cgcmemnode(newsize, basesize, cmemnode);
    if (newcmemnode == NULL){ 
      micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function make_cgcmemnode() was failed.");
      return NULL; 
    }
    *cmemnodep = newcmemnode;
    void *address = micolisp_memory_allocate(type, size, &(machine->memory)); 
    if (address == NULL){ return NULL; }
    return address;
  }
  return address;
}

static int __micolisp_increase (void *address, micolisp_machine *machine, cgcmemnode_history *history){
  if (cgcmemnode_history_recordp(address, history)){
    return 0; 
  }
  if (address == MICOLISP_NIL){
    return 0;
  }
  else 
  if (address == MICOLISP_T){
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_NUMBER, address, machine)){
    if (micolisp_memory_increase(MICOLISP_NUMBER, address, sizeof(micolisp_number), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_SYMBOL, address, machine)){
    if (micolisp_memory_increase(MICOLISP_SYMBOL, address, sizeof(micolisp_symbol), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_CONS, address, machine)){
    if (micolisp_memory_increase(MICOLISP_CONS, address, sizeof(micolisp_cons), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__micolisp_increase(((micolisp_cons*)address)->car, machine, history) != 0){ return 1; }
    if (__micolisp_increase(((micolisp_cons*)address)->cdr, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_CONS_REFERENCE, address, machine)){
    if (micolisp_memory_increase(MICOLISP_CONS_REFERENCE, address, sizeof(micolisp_cons_reference), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__micolisp_increase(((micolisp_cons_reference*)address)->cons, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_C_FUNCTION, address, machine)){
    if (micolisp_memory_increase(MICOLISP_C_FUNCTION, address, sizeof(micolisp_c_function), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_USER_FUNCTION, address, machine)){
    if (micolisp_memory_increase(MICOLISP_USER_FUNCTION, address, sizeof(micolisp_user_function), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__micolisp_increase(((micolisp_user_function*)address)->args, machine, history) != 0){ return 1; }
    if (__micolisp_increase(((micolisp_user_function*)address)->form, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_SCOPE, address, machine)){
    if (micolisp_memory_increase(MICOLISP_SCOPE, address, sizeof(micolisp_scope), &(machine->memory)) != 0){ return 1; }
    if (micolisp_memory_increase(MICOLISP_HASHTABLE_ENTRY, ((micolisp_scope*)address)->hashtable.entries, ((micolisp_scope*)address)->hashtable.length * sizeof(hashtable_entry), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    hashtable_iterator iterator = hashtable_iterate(&(((micolisp_scope*)address)->hashtable));
    hashtable_entry entry;
    while (hashtable_iterator_next(&iterator, &(((micolisp_scope*)address)->hashtable), &entry) == 0){
      if (__micolisp_increase(entry.key, machine, history) != 0){ return 1; }
      if (__micolisp_increase(entry.value, machine, history) != 0){ return 1; }
    }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_SCOPE_REFERENCE, address, machine)){
    if (micolisp_memory_increase(MICOLISP_SCOPE_REFERENCE, address, sizeof(micolisp_scope_reference), &(machine->memory))){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__micolisp_increase(((micolisp_scope_reference*)address)->name, machine, history) != 0){ return 1; }
    if (__micolisp_increase(((micolisp_scope_reference*)address)->scope, machine, history) != 0){ return 1; }
    return 0;
  }
  else {
    micolisp_error_set0(MICOLISP_TYPE_ERROR, "given unmanaged address.");
    return 1; 
  }
}

static int __micolisp_decrease (void *address, micolisp_machine *machine, cgcmemnode_history *history){
  if (cgcmemnode_history_recordp(address, history)){
    return 0; 
  }
  if (address == MICOLISP_NIL){
    return 0;
  }
  else 
  if (address == MICOLISP_T){
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_NUMBER, address, machine)){
    if (micolisp_memory_decrease(MICOLISP_NUMBER, address, sizeof(micolisp_number), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_SYMBOL, address, machine)){
    if (micolisp_memory_decrease(MICOLISP_SYMBOL, address, sizeof(micolisp_symbol), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_CONS, address, machine)){
    if (micolisp_memory_decrease(MICOLISP_CONS, address, sizeof(micolisp_cons), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__micolisp_decrease(((micolisp_cons*)address)->car, machine, history) != 0){ return 1; }
    if (__micolisp_decrease(((micolisp_cons*)address)->cdr, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_CONS_REFERENCE, address, machine)){
    if (micolisp_memory_decrease(MICOLISP_CONS_REFERENCE, address, sizeof(micolisp_cons_reference), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__micolisp_decrease(((micolisp_cons_reference*)address)->cons, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_C_FUNCTION, address, machine)){
    if (micolisp_memory_decrease(MICOLISP_C_FUNCTION, address, sizeof(micolisp_c_function), &(machine->memory)) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_USER_FUNCTION, address, machine)){
    if (micolisp_memory_decrease(MICOLISP_USER_FUNCTION, address, sizeof(micolisp_user_function), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__micolisp_decrease(((micolisp_user_function*)address)->args, machine, history) != 0){ return 1; }
    if (__micolisp_decrease(((micolisp_user_function*)address)->form, machine, history) != 0){ return 1; }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_SCOPE, address, machine)){
    if (micolisp_memory_decrease(MICOLISP_SCOPE, address, sizeof(micolisp_scope), &(machine->memory)) != 0){ return 1; }
    if (micolisp_memory_decrease(MICOLISP_HASHTABLE_ENTRY, ((micolisp_scope*)address)->hashtable.entries, ((micolisp_scope*)address)->hashtable.length * sizeof(hashtable_entry), &(machine->memory)) != 0){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    hashtable_iterator iterator = hashtable_iterate(&(((micolisp_scope*)address)->hashtable));
    hashtable_entry entry;
    while (hashtable_iterator_next(&iterator, &(((micolisp_scope*)address)->hashtable), &entry) == 0){
      if (__micolisp_decrease(entry.key, machine, history) != 0){ return 1; }
      if (__micolisp_decrease(entry.value, machine, history) != 0){ return 1; }
    }
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_SCOPE_REFERENCE, address, machine)){
    if (micolisp_memory_decrease(MICOLISP_SCOPE_REFERENCE, address, sizeof(micolisp_scope_reference), &(machine->memory))){ return 1; }
    ADD_CGCMEMNODE_HISTORY(address, history);
    if (__micolisp_decrease(((micolisp_scope_reference*)address)->name, machine, history) != 0){ return 1; }
    if (__micolisp_decrease(((micolisp_scope_reference*)address)->scope, machine, history) != 0){ return 1; }
    return 0;
  }
  else {
    micolisp_error_set0(MICOLISP_TYPE_ERROR, "given unmanaged address.");
    return 1; 
  }
}

int micolisp_increase (void *address, micolisp_machine *machine){
  MAKE_CGCMEMNODE_HISTORY(history);
  return __micolisp_increase(address, machine, history);
}

int micolisp_decrease (void *address, micolisp_machine *machine){
  MAKE_CGCMEMNODE_HISTORY(history);
  return __micolisp_decrease(address, machine, history);
}

// lisp 

static int micolisp_print_number (micolisp_number *number, FILE *file, micolisp_machine *machine){
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

static int micolisp_print_symbol (micolisp_symbol *symbol, FILE *file, micolisp_machine *machine){
  for (size_t index = 0; index < symbol->length; index++){
    putc(symbol->characters[index], file);
  }
  return 0;
}

static int micolisp_print_list (micolisp_cons *cons, FILE *file, micolisp_machine *machine){
  fputs("(", file);
  for (micolisp_cons *cn = cons; cn != NULL; cn = cn->cdr){
    if (micolisp_typep(MICOLISP_CONS, cn, machine)){
      if (cn != cons){ fputs(" ", file); }
      if (micolisp_print(cn->car, file, machine) != 0){ return 1; }
    }
    else {
      if (cn != cons){ fputs(" . ", file); }
      if (micolisp_print(cn, file, machine) != 0){ return 1; }
      break;
    }
  }
  fputs(")", file);
  return 0;
}

int micolisp_print (void *value, FILE *file, micolisp_machine *machine){
  void *valuedereferenced;
  if (micolisp_reference_get(value, machine, &valuedereferenced) != 0){ 
    return 1; 
  }
  if (valuedereferenced == MICOLISP_T){ fprintf(file, "t"); 
    return 0;
  }
  else 
  if (valuedereferenced == MICOLISP_NIL){ fprintf(file, "nil"); 
    return 0;
  }
  else 
  if (micolisp_typep(MICOLISP_NUMBER, valuedereferenced, machine)){ 
    return micolisp_print_number(valuedereferenced, file, machine);
  }
  else 
  if (micolisp_typep(MICOLISP_SYMBOL, valuedereferenced, machine)){ 
    return micolisp_print_symbol(valuedereferenced, file, machine);
  }
  else 
  if (micolisp_typep(MICOLISP_CONS, valuedereferenced, machine)){ 
    return micolisp_print_list(valuedereferenced, file, machine);
  }
  else 
  if (micolisp_typep(MICOLISP_C_FUNCTION, valuedereferenced, machine)){ 
    fprintf(file, "<function #%p>", valuedereferenced); 
    return 0; 
  }
  else 
  if (micolisp_typep(MICOLISP_USER_FUNCTION, valuedereferenced, machine)){ 
    fprintf(file, "<function #%p>", valuedereferenced); 
    return 0; 
  }
  else {
    micolisp_error_set0(MICOLISP_TYPE_ERROR, "given an unknown type.");
    return 1;
  }
}

int micolisp_println (void *value, FILE *file, micolisp_machine *machine){
  if (micolisp_print(value, file, machine) != 0){ return 1; }
  putc('\n', file);
  return 0;
}

// read 

#define MICOLISP_READ_SUCCESS 0 
#define MICOLISP_READ_ERROR 1 
#define MICOLISP_READ_EOF 2 
#define MICOLISP_READ_CLOSE_PAREN 3 
#define MICOLISP_READ_DOT 4
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

static int parse_as_number (char *buffer, size_t size, micolisp_machine *machine, void **valuep){
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
      micolisp_error_set0(MICOLISP_SYNTAX_ERROR, "given a non digit character.");
      return 1;
    }
  }
  double decimalpart = 0.0;
  for (double decimalbase = 0.1; index < size; index++, decimalbase /= 10.0){
    if ('0' <= buffer[index] && buffer[index] <= '9'){
      decimalpart += decimalbase * (buffer[index] - '0');
    }
    else {
      micolisp_error_set0(MICOLISP_SYNTAX_ERROR, "given a non digit character.");
      return 1;
    }
  }
  micolisp_number *number = micolisp_allocate_number(machine);
  if (number == NULL){ return 1; }
  *number = sign * (integerpart + decimalpart);
  *valuep = number;
  return 0;
}

static int parse_as_symbol (char *buffer, size_t size, micolisp_machine *machine, void **valuep){
  micolisp_symbol *symbol = micolisp_allocate_symbol(buffer, size, machine);
  if (symbol == NULL){ return 1; }
  *valuep = symbol;
  return 0;
}

static int micolisp_read_token (FILE *file, micolisp_machine *machine, void **valuep){
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
      micolisp_error_set0(MICOLISP_SYNTAX_ERROR, "read token is too much long, so buffer was overflow.");
      return 1; 
    }
  }
  if (parse_as_tp(buffer, index)){
    *valuep = MICOLISP_T;
    return 0;
  }
  else 
  if (parse_as_nilp(buffer, index)){
    *valuep = MICOLISP_NIL;
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

static int micolisp_read_quote (FILE *file, micolisp_machine *machine, void **valuep){
  if (getc(file) != '\''){
    micolisp_error_set0(MICOLISP_SYNTAX_ERROR, "first character must be '\''.");
    return 1; 
  }
  void *value;
  if (micolisp_read(file, machine, &value) != 0){ return 1; }
  micolisp_cons *quoted = micolisp_quote(value, machine);
  if (quoted == NULL){ return 1; }
  if (micolisp_decrease(value, machine) != 0){ return 1; } 
  *valuep = quoted;
  return 0;
}

static int micolisp_read_list (FILE *file, micolisp_machine *machine, void **valuep){
  if (getc(file) != '('){ 
    micolisp_error_set0(MICOLISP_SYNTAX_ERROR, "first character must be '('.");
    return 1; 
  }
  micolisp_cons *list = NULL;
  void *value;
  while (true){
    int status = micolisp_read(file, machine, &value);
    if (status == MICOLISP_READ_SUCCESS){
      micolisp_cons *cons = micolisp_allocate_cons(value, list, machine);
      if (cons == NULL){ return 1; }
      if (micolisp_decrease(value, machine) != 0){ return 1; } 
      if (micolisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
    }
    else 
    if (status == MICOLISP_READ_CLOSE_PAREN){
      *valuep = list_nreverse(list);
      return 0;
    }
    else 
    if (status == MICOLISP_READ_DOT){
      void *value1;
      void *value2;
      if (micolisp_read(file, machine, &value1) != MICOLISP_READ_SUCCESS){ return 1; }
      if (micolisp_read(file, machine, &value2) != MICOLISP_READ_CLOSE_PAREN){ 
        micolisp_error_set0(MICOLISP_SYNTAX_ERROR, "must exist close paren after value after dot.");
        return 1; 
      }
      micolisp_cons *lastcons = list;
      micolisp_cons *cons = list_nreverse(list);
      if (micolisp_cons_set(value1, MICOLISP_CONS_CDR, lastcons, machine) != 0){ return 1; }
      if (micolisp_decrease(value1, machine) != 0){ return 1; }
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
      micolisp_error_set0(MICOLISP_SYNTAX_ERROR, "read eof.");
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

static int micolisp_read_string (FILE *file, micolisp_machine *machine, void **valuep){
  if (getc(file) != '"'){ 
    micolisp_error_set0(MICOLISP_SYNTAX_ERROR, "first character must be '\"'.");
    return 1; 
  }
  micolisp_cons *list = NULL;
  int character;
  while ((character = getc(file)) != EOF){
    if (character == '"'){
      break;
    }
    else 
    if (character == '\\'){
      char unescaped;
      if (unescape(file, &unescaped) != 0){ return 1; }
      micolisp_number *number = micolisp_allocate_number(machine);
      if (number == NULL){ return 1; }
      *number = unescaped;
      micolisp_cons *cons = micolisp_allocate_cons(number, list, machine);
      if (cons == NULL){ return 1; }
      if (micolisp_decrease(number, machine) != 0){ return 1; } 
      if (micolisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
    }
    else {
      micolisp_number *number = micolisp_allocate_number(machine);
      if (number == NULL){ return 1; }
      *number = character;
      micolisp_cons *cons = micolisp_allocate_cons(number, list, machine);
      if (cons == NULL){ return 1; }
      if (micolisp_decrease(number, machine) != 0){ return 1; } 
      if (micolisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
    }
  }
  *valuep = micolisp_quote(list_nreverse(list), machine);
  return 0;
}

static void micolisp_read_comment (FILE *file){
  int character;
  while ((character = getc(file)) != EOF){
    if (character == '\n'){
      break;
    }
  }
}

int micolisp_read (FILE *file, micolisp_machine *machine, void **valuep){
  int character;
  while ((character = getc(file)) != EOF){
    if (character == ';'){
      micolisp_read_comment(file);
      continue;
    }
    else 
    if (character == '"'){
      ungetc(character, file);
      return micolisp_read_string(file, machine, valuep);
    }
    else 
    if (character == '('){
      ungetc(character, file);
      return micolisp_read_list(file, machine, valuep);
    }
    else 
    if (character == ')'){
      return MICOLISP_READ_CLOSE_PAREN;
    }
    else 
    if (character == '.'){
      return MICOLISP_READ_DOT;
    }
    else 
    if (character == '\''){
      ungetc(character, file);
      return micolisp_read_quote(file, machine, valuep);
    }
    else 
    if (string_find0(character, WHITESPACE_CHARACTERS)){
      continue;
    }
    else 
    if (string_find0(character, TOKEN_CHARACTERS)){
      ungetc(character, file);
      return micolisp_read_token(file, machine, valuep);
    }
    else {
      return MICOLISP_READ_ERROR; 
    }
  }
  return MICOLISP_READ_EOF;
}

int micolisp_eval (void *form, micolisp_machine *machine, void **valuep){
  void *formdereferenced;
  if (micolisp_reference_get(form, machine, &formdereferenced) != 0){ return 1; }
  if (micolisp_typep(MICOLISP_CONS, formdereferenced, machine)){
    if (!listp(formdereferenced, machine)){
      micolisp_error_set0(MICOLISP_VALUE_ERROR, "formula is an incomplete list.");
      return 1;
    }
    void *function;
    if (micolisp_eval(((micolisp_cons*)formdereferenced)->car, machine, &function) != 0){ return 1; }
    if (micolisp_functionp(function, machine)){
      if (micolisp_function_call(((micolisp_cons*)formdereferenced)->cdr, function, machine, valuep) != 0){ return 1; }
      if (micolisp_decrease(function, machine) != 0){ return 1; }
      return 0;
    }
    else {
      micolisp_error_set0(MICOLISP_VALUE_ERROR, "operator in formula is non function.");
      return 1;
    }
  }
  else 
  if (micolisp_typep(MICOLISP_SYMBOL, formdereferenced, machine)){
    void *value;
    if (micolisp_scope_get(formdereferenced, machine, &value) != 0){ return 1; }
    if (micolisp_increase(value, machine) != 0){ return 1; }
    *valuep = value;
    return 0;
  }
  else {
    if (micolisp_increase(formdereferenced, machine) != 0){ return 1; }
    *valuep = formdereferenced;
    return 0;
  }
}

int micolisp_eval_string (char *sequence, size_t size, micolisp_machine *machine, void **valuep){
  FILE *file = tmpfile();
  if (file == NULL){ 
    micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function tmpfile() was failed.");
    return 1; 
  }
  size_t wrotesize = fwrite(sequence, sizeof(char), size, file);
  if (wrotesize < size){ 
    micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function fwrite() was failed.");
    return 1; 
  }
  if (fseek(file, 0, SEEK_SET) != 0){ 
    micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function fseek() was failed.");
    return 1; 
  }
  void *value;
  if (micolisp_read(file, machine, &value) != 0){ return 1; }
  if (micolisp_eval(value, machine, valuep) != 0){ return 1; }
  if (micolisp_decrease(value, machine) != 0){ return 1; }
  if (fclose(file) != 0){ 
    micolisp_error_set0(MICOLISP_INTERNAL_ERROR, "internal function fclose() was failed.");
    return 1; 
  }
  return 0;
}

int micolisp_eval_string0 (char *sequence, micolisp_machine *machine, void **valuep){
  size_t length = string_length(sequence);
  return micolisp_eval_string(sequence, length, machine, valuep);
}

static void print_error (FILE *file){
  int errorcode;
  char errormessage[MICOLISP_ERROR_INFO_MAX_LENGTH];
  micolisp_error_get(&errorcode, errormessage);
  fprintf(file, ">> REPL is aborted, because error was detected. <<\n");
  fprintf(file, ">> error code = %d: %s <<\n", errorcode, errormessage); 
}

int micolisp_repl (FILE *input, FILE *output, FILE *error, micolisp_machine *machine){
  while (true){
    void *value;
    int status = micolisp_read(input, machine, &value);
    if (status == MICOLISP_READ_SUCCESS){
      void *valueevaluated;
      if (micolisp_eval(value, machine, &valueevaluated) != 0){ print_error(error); return 1; }
      if (micolisp_println(valueevaluated, output, machine) != 0){ print_error(error); return 1; }
      if (micolisp_decrease(value, machine) != 0){ print_error(error); return 1; }
      if (micolisp_decrease(valueevaluated, machine) != 0){ print_error(error); return 1; }
    }
    else 
    if (status == MICOLISP_READ_EOF){
      return 0;
    }
    else 
    if (status == MICOLISP_READ_DOT){
      micolisp_error_set0(MICOLISP_ERROR, "read cons dot before open paren.");
      print_error(error);
      return 1;
    }
    else 
    if (status == MICOLISP_READ_CLOSE_PAREN){
      micolisp_error_set0(MICOLISP_ERROR, "read close paren before open paren.");
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

static int make_function (micolisp_function_type type, micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args == NULL){ return 1; }
  void *a = args->car;
  void *forms = args->cdr;
  if (!listp(a, machine)){ return 1; }
  micolisp_symbol *progn = micolisp_allocate_symbol0("progn", machine);
  if (progn == NULL){ return 1; }
  micolisp_cons *prognforms = micolisp_allocate_cons(progn, forms, machine);
  if (prognforms == NULL){ return 1; }
  micolisp_user_function *function = micolisp_allocate_user_function(type, a, prognforms, machine);
  if (function == NULL){ return 1; }
  if (micolisp_decrease(progn, machine) != 0){ return 1; }
  if (micolisp_decrease(prognforms, machine) != 0){ return 1; }
  *valuep = function;
  return 0;
}

static int try_define_function (micolisp_function_type type, micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args == NULL){ return 1; }
  void *name = args->car;
  void *definations = args->cdr;
  if (micolisp_typep(MICOLISP_SYMBOL, name, machine)){
    void *function;
    if (make_function(type, definations, machine, &function) != 0){ return 1; }
    if (micolisp_scope_set(function, name, machine) != 0){ return 1; }
    *valuep = function;
    return 0;
  }
  else {
    return make_function(type, args, machine, valuep);
  }
}

static int __micolisp_function (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  return try_define_function(MICOLISP_FUNCTION, args, machine, valuep);
}

static int __micolisp_syntax (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  return try_define_function(MICOLISP_SYNTAX, args, machine, valuep);
}

static int __micolisp_macro (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  return try_define_function(MICOLISP_MACRO, args, machine, valuep);
}

static int __micolisp_progn (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *lastvalue = NULL;
  for (micolisp_cons *cons = args; cons != NULL; cons = cons->cdr){
    if (micolisp_decrease(lastvalue, machine) != 0){ return 1; }
    if (micolisp_eval(((micolisp_cons*)cons)->car, machine, &lastvalue) != 0){ return 1; }
  }
  *valuep = lastvalue;
  return 0;
}

static int __micolisp_if (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *cond;
  void *then;
  void *els;
  if (list_nth(0, args, &cond) != 0){ return 1; }
  if (list_nth(1, args, &then) != 0){ return 1; }
  if (list_nth(2, args, &els) != 0){ return 1; }
  void *condevaluated;
  void *conddereferenced;
  if (micolisp_eval(cond, machine, &condevaluated) != 0){ return 1; }
  if (micolisp_reference_get(condevaluated, machine, &conddereferenced) != 0){ return 1; }
  if (micolisp_decrease(conddereferenced, machine) != 0){ return 1; }
  return micolisp_eval(conddereferenced != NULL? then: els, machine, valuep);
}

static int __micolisp_while (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args == NULL){ return 1; }
  void *cond = args->car;
  void *forms = args->cdr;
  while (true){
    void *condevaluated;
    if (micolisp_eval(cond, machine, &condevaluated) != 0){ return 1; }
    if (micolisp_decrease(condevaluated, machine) != 0){ return 1; }
    if (condevaluated != NULL){
      void *formsevaluated;
      if (__micolisp_progn(forms, machine, &formsevaluated) != 0){ return 1; }
      if (micolisp_decrease(formsevaluated, machine) != 0){ return 1; }
    }
    else {
      break;
    }
  }
  *valuep = NULL;
  return 0;
}

static int __micolisp_var (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *name;
  void *form;
  void *formevaluated;
  if (list_nth(0, args, &name) != 0){ return 1; }
  if (list_nth(1, args, &form) != 0){ return 1; }
  if (micolisp_eval(form, machine, &formevaluated) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_SYMBOL, name, machine)){ return 1; }
  if (micolisp_scope_set(formevaluated, name, machine) != 0){ return 1; }
  if (micolisp_increase(formevaluated, machine) != 0){ return 1; }
  *valuep = formevaluated;
  return 0;
}

static int __micolisp_set (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *reference;
  void *form;
  void *referenceevaluated;
  void *formevaluated;
  if (list_nth(0, args, &reference) != 0){ return 1; }
  if (list_nth(1, args, &form) != 0){ return 1; }
  if (micolisp_eval(reference, machine, &referenceevaluated) != 0){ return 1; }
  if (micolisp_eval(form, machine, &formevaluated) != 0){ return 1; }
  if (!micolisp_referencep(referenceevaluated, machine)){ return 1; }
  if (micolisp_reference_set(formevaluated, referenceevaluated, machine) != 0){ return 1; }
  if (micolisp_decrease(referenceevaluated, machine) != 0){ return 1; }
  *valuep = formevaluated;
  return 0;
}

static int __micolisp_quote (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (micolisp_increase(value, machine) != 0){ return 1; }
  *valuep = value;
  return 0;
}

// builtin function

static int __micolisp_symbol (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (!listp(value, machine)){ return 1; }
  char buffer[MICOLISP_SYMBOL_MAX_LENGTH];
  size_t index;
  micolisp_cons *cons;
  for (index = 0, cons = value; index < sizeof(buffer) && cons != NULL; index++, cons = cons->cdr){
    if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
    buffer[index] = (int)*(micolisp_number*)(cons->car);
  }
  micolisp_symbol *symbol = micolisp_allocate_symbol(buffer, index, machine);
  if (symbol == NULL){ return 1; }
  *valuep = symbol;
  return 0;
}

static int __micolisp_symbol_name (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *symbol;
  if (list_nth(0, args, &symbol) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_SYMBOL, symbol, machine)){ return 1; }
  micolisp_cons *list = NULL;
  for (size_t index = 0; index < ((micolisp_symbol*)symbol)->length; index++){
    micolisp_number *number = micolisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = ((micolisp_symbol*)symbol)->characters[index];
    micolisp_cons *cons = micolisp_allocate_cons(number, list, machine);
    if (cons == NULL){ return 1; }
    if (micolisp_decrease(number, machine) != 0){ return 1; } 
    if (micolisp_decrease(list, machine) != 0){ return 1; } 
    list = cons;
  }
  *valuep = list_nreverse(list);
  return 0;
}

static int __micolisp_symbol_value (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *symbol;
  if (list_nth(0, args, &symbol) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_SYMBOL, symbol, machine)){ return 1; }
  micolisp_scope_reference *reference = micolisp_scope_get_reference(symbol, machine);
  if (reference == NULL){ return 1; }
  *valuep = reference;
  return 0;
}

static int __micolisp_cons (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *car;
  void *cdr;
  if (list_nth(0, args, &car) != 0){ return 1; }
  if (list_nth(1, args, &cdr) != 0){ return 1; }
  micolisp_cons *cons = micolisp_allocate_cons(car, cdr, machine);
  if (cons == NULL){ return 1; }
  *valuep = cons;
  return 0;
}

static int __micolisp_list (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  micolisp_cons *list = NULL;
  for (micolisp_cons *cons = args; cons != NULL; cons = cons->cdr){
    micolisp_cons *cn = micolisp_allocate_cons(cons->car, list, machine);
    if (cn == NULL){ return 1; }
    if (micolisp_decrease(list, machine) != 0){ return 1; } 
    list = cn;
  }
  *valuep = list_nreverse(list);
  return 0;
}

static int __micolisp_car (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *cons;
  if (list_nth(0, args, &cons) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_CONS, cons, machine)){ return 1; }
  micolisp_cons_reference *reference = micolisp_cons_get_reference(MICOLISP_CONS_CAR, cons, machine);
  if (reference == NULL){ return 1; }
  *valuep = reference;
  return 0;
}

static int __micolisp_cdr (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *cons;
  if (list_nth(0, args, &cons) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_CONS, cons, machine)){ return 1; }
  micolisp_cons_reference *reference = micolisp_cons_get_reference(MICOLISP_CONS_CDR, cons, machine);
  if (reference == NULL){ return 1; }
  *valuep = reference;
  return 0;
}

static int __micolisp_add (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  micolisp_number result = 0;
  for (micolisp_cons *cons = args; cons != NULL; cons = cons->cdr){
    if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
    result += *(micolisp_number*)(cons->car);
  }
  micolisp_number *resultp = micolisp_allocate_number(machine);
  if (resultp == NULL){ return 1; }
  *resultp = result;
  *valuep = resultp;
  return 0;
}

static int __micolisp_sub (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!micolisp_typep(MICOLISP_NUMBER, args->car, machine)){ return 1; }
    micolisp_number result = *(micolisp_number*)(args->car);
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
      result -= *(micolisp_number*)(cons->car);
    }
    micolisp_number *resultp = micolisp_allocate_number(machine);
    if (resultp == NULL){ return 1; }
    *resultp = result;
    *valuep = resultp;
    return 0;
  }
  else {
    return 1;
  }
}

static int __micolisp_mul (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!micolisp_typep(MICOLISP_NUMBER, args->car, machine)){ return 1; }
    micolisp_number result = *(micolisp_number*)(args->car);
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
      result *= *(micolisp_number*)(cons->car);
    }
    micolisp_number *resultp = micolisp_allocate_number(machine);
    if (resultp == NULL){ return 1; }
    *resultp = result;
    *valuep = resultp;
    return 0;
  }
  else {
    return 1;
  }
}

static int __micolisp_div (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!micolisp_typep(MICOLISP_NUMBER, args->car, machine)){ return 1; }
    micolisp_number result = *(micolisp_number*)(args->car);
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
      result /= *(micolisp_number*)(cons->car);
    }
    micolisp_number *resultp = micolisp_allocate_number(machine);
    if (resultp == NULL){ return 1; }
    *resultp = result;
    *valuep = resultp;
    return 0;
  }
  else {
    return 1;
  }
}

static int __micolisp_mod (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!micolisp_typep(MICOLISP_NUMBER, args->car, machine)){ return 1; }
    micolisp_number result = *(micolisp_number*)(args->car);
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
      result = result - floor(result / *(micolisp_number*)(cons->car));
    }
    micolisp_number *resultp = micolisp_allocate_number(machine);
    if (resultp == NULL){ return 1; }
    *resultp = result;
    *valuep = resultp;
    return 0;
  }
  else {
    return 1;
  }
}

static int __micolisp_lshift (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value1, machine)){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(micolisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(micolisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    micolisp_number *number = micolisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 << (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_rshift (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value1, machine)){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(micolisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(micolisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    micolisp_number *number = micolisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 >> (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_log_rshift (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value1, machine)){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(micolisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(micolisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    micolisp_number *number = micolisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (uintmax_t)integerpart1 >> (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_log_not (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value, machine)){ return 1; }
  double integerpart;
  double decimalpart = modf(*(micolisp_number*)value, &integerpart);
  if (abs(decimalpart) <= 0.0){
    micolisp_number *number = micolisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = ~(intmax_t)integerpart;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_log_and (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value1, machine)){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(micolisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(micolisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    micolisp_number *number = micolisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 & (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_log_or (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value1, machine)){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(micolisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(micolisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    micolisp_number *number = micolisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 | (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_log_xor (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value1;
  void *value2;
  if (list_nth(0, args, &value1) != 0){ return 1; }
  if (list_nth(1, args, &value2) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value1, machine)){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, value2, machine)){ return 1; }
  double integerpart1;
  double decimalpart1 = modf(*(micolisp_number*)value1, &integerpart1);
  double integerpart2;
  double decimalpart2 = modf(*(micolisp_number*)value2, &integerpart2);
  if (abs(decimalpart1) <= 0.0 && abs(decimalpart2) <= 0.0){
    micolisp_number *number = micolisp_allocate_number(machine);
    if (number == NULL){ return 1; }
    *number = (intmax_t)integerpart1 ^ (intmax_t)integerpart2;
    *valuep = number;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_floor (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *number;
  if (list_nth(0, args, &number) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, number, machine)){ return 1; }
  micolisp_number *newnumber = micolisp_allocate_number(machine);
  if (newnumber == NULL){ return 1; }
  *newnumber = floor(*(micolisp_number*)number);
  *valuep = newnumber;
  return 0;
}

static int __micolisp_ceil (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *number;
  if (list_nth(0, args, &number) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, number, machine)){ return 1; }
  micolisp_number *newnumber = micolisp_allocate_number(machine);
  if (newnumber == NULL){ return 1; }
  *newnumber = ceil(*(micolisp_number*)number);
  *valuep = newnumber;
  return 0;
}

static int __micolisp_round (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *number;
  if (list_nth(0, args, &number) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, number, machine)){ return 1; }
  micolisp_number *newnumber = micolisp_allocate_number(machine);
  if (newnumber == NULL){ return 1; }
  *newnumber = round(*(micolisp_number*)number);
  *valuep = newnumber;
  return 0;
}

static bool equal (void*, void*, micolisp_machine*);

static bool equal_cons (micolisp_cons *cons1, micolisp_cons *cons2, micolisp_machine *machine){
  return equal(cons1->car, cons2->car, machine) && equal(cons1->cdr, cons2->cdr, machine);
}

static bool equal (void *value1, void *value2, micolisp_machine *machine){
  if (micolisp_typep(MICOLISP_NUMBER, value1, machine) && micolisp_typep(MICOLISP_NUMBER, value2, machine)){
    return *(micolisp_number*)value1 == *(micolisp_number*)value2;
  }
  else 
  if (micolisp_typep(MICOLISP_CONS, value1, machine) && micolisp_typep(MICOLISP_CONS, value2, machine)){
    return equal_cons(value1, value2, machine);
  }
  else {
    return value1 == value2;
  }
}

static int __micolisp_equal (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    void *first = args->car;
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!equal(first, cons->car, machine)){
        *valuep = MICOLISP_NIL;
        return 0;
      }
    }
    *valuep = MICOLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_unequal (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (__micolisp_equal(args, machine, &value) != 0){ return 1; }
  *valuep = value != MICOLISP_NIL? MICOLISP_NIL: MICOLISP_T;
  return 0;
}

static int __micolisp_less (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!micolisp_typep(MICOLISP_NUMBER, args->car, machine)){ return 1; }
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
      if (!(*(micolisp_number*)(args->car) < *(micolisp_number*)(cons->car))){
        *valuep = MICOLISP_NIL;
        return 0;
      }
    }
    *valuep = MICOLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_less_or_equal (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!micolisp_typep(MICOLISP_NUMBER, args->car, machine)){ return 1; }
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
      if (!(*(micolisp_number*)(args->car) <= *(micolisp_number*)(cons->car))){
        *valuep = MICOLISP_NIL;
        return 0;
      }
    }
    *valuep = MICOLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_great (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!micolisp_typep(MICOLISP_NUMBER, args->car, machine)){ return 1; }
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
      if (!(*(micolisp_number*)(args->car) > *(micolisp_number*)(cons->car))){
        *valuep = MICOLISP_NIL;
        return 0;
      }
    }
    *valuep = MICOLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_great_or_equal (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (args != NULL){
    if (!micolisp_typep(MICOLISP_NUMBER, args->car, machine)){ return 1; }
    for (micolisp_cons *cons = args->cdr; cons != NULL; cons = cons->cdr){
      if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
      if (!(*(micolisp_number*)(args->car) >= *(micolisp_number*)(cons->car))){
        *valuep = MICOLISP_NIL;
        return 0;
      }
    }
    *valuep = MICOLISP_T;
    return 0;
  }
  else {
    return 1; 
  }
}

static int __micolisp_numberp (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = micolisp_typep(MICOLISP_NUMBER, value, machine)? MICOLISP_T: MICOLISP_NIL;
  return 0;
}

static int __micolisp_symbolp (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = micolisp_typep(MICOLISP_SYMBOL, value, machine)? MICOLISP_T: MICOLISP_NIL;
  return 0;
}

static int __micolisp_functionp (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = micolisp_functionp(value, machine)? MICOLISP_T: MICOLISP_NIL;
  return 0;
}

static int __micolisp_consp (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = micolisp_typep(MICOLISP_CONS, value, machine)? MICOLISP_T: MICOLISP_NIL;
  return 0;
}

static int __micolisp_listp (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  *valuep = listp(value, machine)? MICOLISP_T: MICOLISP_NIL;
  return 0;
}

static int __micolisp_read (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (micolisp_read(stdin, machine, &value) != 0){ return 1; }
  *valuep = value;
  return 0;
}

static int __micolisp_read_string (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  size_t readsize;
  if (1 <= list_length(args)){
    void *size;
    if (list_nth(0, args, &size) != 0){ return 1; }
    if (!micolisp_typep(MICOLISP_NUMBER, size, machine)){ return 1; }
    readsize = *(micolisp_number*)size;
  }
  else {
    readsize = SIZE_MAX;
  }
  micolisp_cons *list = NULL;
  for (size_t index = 0; index < readsize; index++){
    int character = getc(stdin);
    if (character != EOF){
      micolisp_number *number = micolisp_allocate_number(machine);
      if (number == NULL){ return 1; }
      *number = character;
      micolisp_cons *cons = micolisp_allocate_cons(number, list, machine);
      if (cons == NULL){ return 1; }
      if (micolisp_decrease(number, machine) != 0){ return 1; } 
      if (micolisp_decrease(list, machine) != 0){ return 1; } 
      list = cons;
    }
    else {
      break;
    }
  }
  *valuep = list_nreverse(list);
  return 0;
}

static int __micolisp_read_line (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  size_t readsize;
  if (1 <= list_length(args)){
    void *size;
    if (list_nth(0, args, &size) != 0){ return 1; }
    if (!micolisp_typep(MICOLISP_NUMBER, size, machine)){ return 1; }
    readsize = *(micolisp_number*)size;
  }
  else {
    readsize = SIZE_MAX;
  }
  micolisp_cons *list = NULL;
  for (size_t index = 0; index < readsize; index++){
    int character = getc(stdin);
    if (character != EOF){
      micolisp_number *number = micolisp_allocate_number(machine);
      if (number == NULL){ return 1; }
      *number = character;
      micolisp_cons *cons = micolisp_allocate_cons(number, list, machine);
      if (cons == NULL){ return 1; }
      if (micolisp_decrease(number, machine) != 0){ return 1; } 
      if (micolisp_decrease(list, machine) != 0){ return 1; } 
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

static int __micolisp_eval (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  void *valueevaluated;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (micolisp_eval(value, machine, &valueevaluated) != 0){ return 1; }
  *valuep = valueevaluated;
  return 0;
}

static int __micolisp_eval_string (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *list;
  if (list_nth(0, args, &list) != 0){ return 1; }
  if (!listp(list, machine)){ return 1; }
  size_t length = list_length(list);
  char buffer[length];
  size_t index;
  micolisp_cons *cons;
  for (index = 0, cons = list; index < length && cons != NULL; index++, cons = cons->cdr){
    if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
    buffer[index] = *(micolisp_number*)(cons->car);
  }
  return micolisp_eval_string(buffer, length, machine, valuep);
}

static int __micolisp_print (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (micolisp_print(value, stdout, machine) != 0){ return 1; }
  if (micolisp_increase(value, machine) != 0){ return 1; }
  *valuep = value;
  return 0;
}

static int __micolisp_println (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *value;
  if (list_nth(0, args, &value) != 0){ return 1; }
  if (micolisp_println(value, stdout, machine) != 0){ return 1; }
  if (micolisp_increase(value, machine) != 0){ return 1; }
  *valuep = value;
  return 0;
}

static int __micolisp_write (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *list;
  if (list_nth(0, args, &list) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_CONS, list, machine)){ return 1; }
  for (micolisp_cons *cons = list; cons != NULL; cons = cons->cdr){
    void *consdereferenced;
    if (micolisp_reference_get(cons, machine, &consdereferenced) != 0){ return 1; }
    if (!micolisp_typep(MICOLISP_CONS, consdereferenced, machine)){ return 1; }
    if (!micolisp_typep(MICOLISP_NUMBER, ((micolisp_cons*)consdereferenced)->car, machine)){ return 1; }
    putchar((int)*(micolisp_number*)((micolisp_cons*)consdereferenced)->car);
  }
  return 0;
}

static int __micolisp_writeln (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  if (__micolisp_write(args, machine, valuep) != 0){ return 1; }
  putchar('\n');
  return 0;
}

static int __micolisp_error (micolisp_cons *args, micolisp_machine *machine, void **valuep){
  void *errorcode;
  void *errormessage;
  if (list_nth(0, args, &errorcode) != 0){ return 1; }
  if (list_nth(1, args, &errormessage) != 0){ return 1; }
  if (!micolisp_typep(MICOLISP_NUMBER, errorcode, machine)){ return 1; }
  if (!listp(errormessage, machine)){ return 1; }
  char buffer[MICOLISP_ERROR_INFO_MAX_LENGTH];
  size_t index;
  micolisp_cons *cons;
  for (index = 0, cons = errormessage; index < MICOLISP_ERROR_INFO_MAX_LENGTH && cons != NULL; index++, cons = cons->cdr){
    if (!micolisp_typep(MICOLISP_NUMBER, cons->car, machine)){ return 1; }
    buffer[index] = *(micolisp_number*)(cons->car);
  }
  micolisp_error_set(*(micolisp_number*)errorcode, buffer, index);
  return 1;
}

// setup syntax 

static int setup_builtin_syntax (micolisp_machine *machine){
  // define function 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("function", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_function, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define syntax 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("syntax", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_syntax, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define macro 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("macro", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_macro, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define progn 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("progn", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_progn, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define if 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("if", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_if, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define while
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("while", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_while, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define set
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("set", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_set, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define var
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("var", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_var, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define quote
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("quote", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_SYNTAX, __micolisp_quote, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  return 0;
}

// setup function 

static int setup_builtin_function (micolisp_machine *machine){
  // define symbol 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("symbol", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_symbol, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define symbol-name 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("symbol-name", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_symbol_name, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define symbol-value
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("symbol-value", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_symbol_value, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define cons
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("cons", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_cons, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define list
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("list", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_list, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define car
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("car", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_car, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define cdr
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("cdr", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_cdr, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define + 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("+", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_add, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define -
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("-", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_sub, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define *
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("*", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_mul, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define / 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("/", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_div, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define % 
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("%", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_mod, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define <<
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("<<", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_lshift, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define >>
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0(">>", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_rshift, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define >>>
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0(">>>", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_log_rshift, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define ~
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("~", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_log_not, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define &
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("&", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_log_and, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define |
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("|", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_log_or, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define ^
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("^", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_log_xor, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define floor
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("floor", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_floor, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define ceil
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("ceil", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_ceil, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define round
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("round", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_round, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define ==
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("==", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_equal, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define !=
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("!=", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_unequal, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define <
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("<", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_less, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define <=
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("<=", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_less_or_equal, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define >
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0(">", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_great, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define >=
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0(">=", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_great_or_equal, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define numberp
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("numberp", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_numberp, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define symbolp
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("symbolp", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_symbolp, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define functionp
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("functionp", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_functionp, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define consp
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("consp", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_consp, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define listp
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("listp", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_listp, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define read
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("read", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_read, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define read-string
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("read-string", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_read_string, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define read-line
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("read-line", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_read_line, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define eval
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("eval", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_eval, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define eval-string
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("eval-string", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_eval_string, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define print
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("print", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_print, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define println
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("println", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_println, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define write
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("write", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_write, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define writeln
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("writeln", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_writeln, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define error
  {
    micolisp_symbol *symbol = micolisp_allocate_symbol0("error", machine);
    if (symbol == NULL){ return 1; }
    micolisp_c_function *function = micolisp_allocate_c_function(MICOLISP_FUNCTION, __micolisp_error, machine);
    if (function == NULL){ return 1; }
    if (micolisp_scope_set(function, symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(symbol, machine) != 0){ return 1; }
    if (micolisp_decrease(function, machine) != 0){ return 1; }
  }
  // define not 
  {
    char source[] = "(function not (any) (if any nil t))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define null 
  {
    char source[] = "(var null not)";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define length
  {
    char source[] = "(function length (lst) (if (null lst) 0 (+ (length (cdr lst)) 1)))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define nth
  {
    char source[] = "(function nth (index lst) (if (null lst) nil (if (<= index 0) (car lst) (nth (- index 1) (cdr lst)))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define map
  {
    char source[] = "(function map (fn lst) (if (null lst) nil (cons (fn (car lst)) (map fn (cdr lst)))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define filter
  {
    char source[] = "(function filter (fn lst) (if (null lst) nil (if (fn (car lst)) (cons (car lst) (filter fn (cdr lst))) (filter fn (cdr lst)))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define --reduce
  {
    char source[] = "(function --reduce (fn acc lst) (if (null lst) acc (--reduce fn (fn acc (car lst)) (cdr lst))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define reduce
  {
    char source[] = "(function reduce (fn lst) (if (null lst) nil (--reduce fn (car lst) (cdr lst))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define find
  {
    char source[] = "(function find (fn lst) (if (null lst) nil (if (fn (car lst)) (car lst) (find fn (cdr lst)))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  return 0;
}

// setup macro 

static int setup_builtin_macro (micolisp_machine *machine){
  // define until 
  {
    char source[] = "(macro until (cond &rest forms) (cons 'while (cons (list 'not cond) forms)))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define when
  {
    char source[] = "(macro when (cond &rest forms) (list 'if cond (cons 'progn forms) nil))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define unless 
  {
    char source[] = "(macro unless (cond &rest forms) (list 'if cond nil (cons 'progn forms)))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define and 
  {
    char source[] = "(macro and (&rest forms) (if (null forms) t (if (null (cdr forms)) (car forms) (list 'if (car forms) (cons 'and (cdr forms)) nil))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define or 
  {
    char source[] = "(macro or (&rest forms) (if (null forms) nil (list 'progn (list 'var '--temp (car forms)) (list 'if '--temp '--temp (cons 'or (cdr forms))))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  // define cond 
  {
    char source[] = "(macro cond (&rest conds) (if (null conds) nil (list 'if (car (car conds)) (cons 'progn (cdr (car conds))) (cons 'cond (cdr conds)))))";
    void *value;
    if (micolisp_eval_string0(source, machine, &value) != 0){ return 1; }
    if (micolisp_decrease(value, machine) != 0){ return 1; }
  }
  return 0;
}

// micolisp 

int micolisp_open (micolisp_machine *machine){
  micolisp_init(machine);
  if (micolisp_scope_begin(machine) != 0){ return 1; }
  return 0;
}

int micolisp_load_library (micolisp_machine *machine){
  if (setup_builtin_syntax(machine) != 0){ return 1; }
  if (setup_builtin_function(machine) != 0){ return 1; }
  if (setup_builtin_macro(machine) != 0){ return 1; }
  return 0;
}

int micolisp_close (micolisp_machine *machine){
  micolisp_memory_free(&(machine->memory));
  return 0;
}
