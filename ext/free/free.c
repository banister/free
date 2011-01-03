/* (c) 2010 John Mair (banisterfiend), MIT license */

#include <ruby.h>
#include "compat.h"

#ifdef RUBY_19
# include <ruby/io.h>
# include <ruby/re.h>
# include "vm_core.h"
#else
# include "re.h"
# include "env.h"
# include "node.h"
# include "rubysig.h"
# include "rubyio.h"
#endif

#include "free.h"

typedef struct RVALUE {
  union {
    struct {
      VALUE flags;		/* always 0 for freed obj */
      struct RVALUE *next;
    } free;
    struct RBasic  basic;
    struct RObject object;
    struct RClass  klass;
    struct RFloat  flonum;
    struct RString string;
    struct RArray  array;
    struct RRegexp regexp;
    struct RHash   hash;
    struct RData   data;
    struct RStruct rstruct;
    struct RBignum bignum;
    struct RFile   file;
    struct RNode   node;
    struct RMatch  match;
#ifdef RUBY_19
    struct RTypedData   typeddata;
    struct RRational rational;
    struct RComplex complex;
#else
    struct RVarmap varmap;
    struct SCOPE   scope;
#endif
  } as;
} RVALUE;

#define RANY(o) ((RVALUE*)(o))

#ifdef RUBY_19
static inline void
make_deferred(RVALUE *p)
{
  p->as.basic.flags = (p->as.basic.flags & ~T_MASK) | T_ZOMBIE;
}

static inline void
make_io_deferred(RVALUE *p)
{
  rb_io_t *fptr = p->as.file.fptr;
  make_deferred(p);
  p->as.data.dfree = (void (*)(void*))rb_io_fptr_finalize;
  p->as.data.data = fptr;
}

static int
free_method_entry_i(ID key, rb_method_entry_t *me, st_data_t data)
{
  rb_free_method_entry(me);
  return ST_CONTINUE;
}

static void
rb_free_m_table(st_table *tbl)
{
  st_foreach(tbl, free_method_entry_i, 0);
  st_free_table(tbl);
}
#else

#define T_DEFERRED 0x3a

static inline void
make_deferred(RVALUE *p)
{
  p->as.basic.flags = (p->as.basic.flags & ~T_MASK) | T_DEFERRED;
}

#endif

VALUE object_free(VALUE obj)
{
  ID id_destructor = rb_intern("__destruct__");

  /* value returned by destructor */
  VALUE destruct_value = Qnil;
  
  /* prevent freeing of immediates */
  switch (TYPE(obj)) {
  case T_NIL:
  case T_FIXNUM:
  case T_TRUE:
  case T_FALSE:
  case T_SYMBOL:
    rb_raise(rb_eTypeError, "obj_free() called for immediate value");
    break;
  }

  /* prevent freeing of *some* critical objects */
  if ((obj == rb_cObject) ||
      (obj == rb_cClass) ||
      (obj == rb_cModule) ||
      (obj == rb_cSymbol) ||
      (obj == rb_cFixnum) ||
      (obj == rb_cFloat) ||
      (obj == rb_cString) ||
      (obj == rb_cRegexp) ||
      (obj == rb_cInteger) ||
      (obj == rb_cArray) ||
      (obj == rb_cNilClass) ||
      (obj == rb_cFalseClass) ||
      (obj == rb_cTrueClass) ||
      (obj == rb_cNumeric) ||
      (obj == rb_cBignum) ||
      (obj == rb_cStruct)) 
    rb_raise(rb_eTypeError, "obj_free() called for critical object");
   
  /* run destructor (if one is defined) */
  if (rb_respond_to(obj, id_destructor))
    destruct_value = rb_funcall(obj, id_destructor, 0);

#ifdef RUBY_19
      switch (BUILTIN_TYPE(obj)) {
      case T_NIL:
      case T_FIXNUM:
      case T_TRUE:
      case T_FALSE:
        rb_bug("obj_free() called for broken object");
        break;
      }

  if (FL_TEST(obj, FL_EXIVAR)) {
    rb_free_generic_ivar((VALUE)obj);
    FL_UNSET(obj, FL_EXIVAR);
  }

  switch (BUILTIN_TYPE(obj)) {
  case T_OBJECT:
    if (!(RANY(obj)->as.basic.flags & ROBJECT_EMBED) &&
        RANY(obj)->as.object.as.heap.ivptr) {
      xfree(RANY(obj)->as.object.as.heap.ivptr);
    }
    break;
  case T_MODULE:
  case T_CLASS:
    rb_clear_cache_by_class((VALUE)obj);
    rb_free_m_table(RCLASS_M_TBL(obj));
    if (RCLASS_IV_TBL(obj)) {
      st_free_table(RCLASS_IV_TBL(obj));
    }
    if (RCLASS_IV_INDEX_TBL(obj)) {
      st_free_table(RCLASS_IV_INDEX_TBL(obj));
    }
    xfree(RANY(obj)->as.klass.ptr);
    break;
  case T_STRING:
    rb_str_free(obj);
    break;
  case T_ARRAY:
    rb_ary_free(obj);
    break;
  case T_HASH:
    if (RANY(obj)->as.hash.ntbl) {
      st_free_table(RANY(obj)->as.hash.ntbl);
    }
    break;
  case T_REGEXP:
    if (RANY(obj)->as.regexp.ptr) {
      onig_free(RANY(obj)->as.regexp.ptr);
    }
    break;
  case T_DATA:
    if (DATA_PTR(obj)) {
      if (RTYPEDDATA_P(obj)) {
        RDATA(obj)->dfree = RANY(obj)->as.typeddata.type->dfree;
      }
      if ((long)RANY(obj)->as.data.dfree == -1) {
        xfree(DATA_PTR(obj));
      }
      else if (RANY(obj)->as.data.dfree) {
        make_deferred(RANY(obj));
        return 1;
      }
    }
    break;
  case T_MATCH:
    if (RANY(obj)->as.match.rmatch) {
      struct rmatch *rm = RANY(obj)->as.match.rmatch;
      onig_region_free(&rm->regs, 0);
      if (rm->char_offset)
        xfree(rm->char_offset);
      xfree(rm);
    }
    break;
  case T_FILE:
    if (RANY(obj)->as.file.fptr) {
      make_io_deferred(RANY(obj));
      return 1;
    }
    break;
  case T_RATIONAL:
  case T_COMPLEX:
    break;
  case T_ICLASS:
    /* iClass shares table with the module */
    xfree(RANY(obj)->as.klass.ptr);
    break;

  case T_FLOAT:
    break;

  case T_BIGNUM:
    if (!(RBASIC(obj)->flags & RBIGNUM_EMBED_FLAG) && RBIGNUM_DIGITS(obj)) {
      xfree(RBIGNUM_DIGITS(obj));
    }
    break;
  case T_NODE:
    switch (nd_type(obj)) {
    case NODE_SCOPE:
      if (RANY(obj)->as.node.u1.tbl) {
        xfree(RANY(obj)->as.node.u1.tbl);
      }
      break;
    case NODE_ALLOCA:
      xfree(RANY(obj)->as.node.u1.node);
      break;
    }
    break;			/* no need to free iv_tbl */

  case T_STRUCT:
    if ((RBASIC(obj)->flags & RSTRUCT_EMBED_LEN_MASK) == 0 &&
        RANY(obj)->as.rstruct.as.heap.ptr) {
      xfree(RANY(obj)->as.rstruct.as.heap.ptr);
    }
    break;

  default:
    rb_bug("gc_sweep(): unknown data type 0x%x(%p)",
           BUILTIN_TYPE(obj), (void*)obj);
  }

#else
  switch (BUILTIN_TYPE(obj)) {
  case T_NIL:
  case T_FIXNUM:
  case T_TRUE:
  case T_FALSE:
    rb_bug("obj_free() called for broken object");
    break;
  }

  if (FL_TEST(obj, FL_EXIVAR)) {
    rb_free_generic_ivar((VALUE)obj);
  }

  switch (BUILTIN_TYPE(obj)) {
  case T_OBJECT:
    if (RANY(obj)->as.object.iv_tbl) {
      st_free_table(RANY(obj)->as.object.iv_tbl);
    }
    break;
  case T_MODULE:
  case T_CLASS:
    rb_clear_cache_by_class((VALUE)obj);
    st_free_table(RANY(obj)->as.klass.m_tbl);
    if (RANY(obj)->as.object.iv_tbl) {
      st_free_table(RANY(obj)->as.object.iv_tbl);
    }
    break;
  case T_STRING:
    if (RANY(obj)->as.string.ptr && !FL_TEST(obj, ELTS_SHARED)) {
      RUBY_CRITICAL(free(RANY(obj)->as.string.ptr));
    }
    break;
  case T_ARRAY:
    if (RANY(obj)->as.array.ptr && !FL_TEST(obj, ELTS_SHARED)) {
      RUBY_CRITICAL(free(RANY(obj)->as.array.ptr));
    }
    break;
  case T_HASH:
    if (RANY(obj)->as.hash.tbl) {
      st_free_table(RANY(obj)->as.hash.tbl);
    }
    break;
  case T_REGEXP:
    if (RANY(obj)->as.regexp.ptr) {
      re_free_pattern(RANY(obj)->as.regexp.ptr);
    }
    if (RANY(obj)->as.regexp.str) {
      RUBY_CRITICAL(free(RANY(obj)->as.regexp.str));
    }
    break;
  case T_DATA:
    if (DATA_PTR(obj)) {
      if ((long)RANY(obj)->as.data.dfree == -1) {
        RUBY_CRITICAL(free(DATA_PTR(obj)));
      }
      else if (RANY(obj)->as.data.dfree) {
        make_deferred(RANY(obj));
        return 1;
      }
    }
    break;
  case T_MATCH:
    if (RANY(obj)->as.match.regs) {
      re_free_registers(RANY(obj)->as.match.regs);
      RUBY_CRITICAL(free(RANY(obj)->as.match.regs));
    }
    break;
  case T_FILE:
    if (RANY(obj)->as.file.fptr) {
      struct rb_io_t *fptr = RANY(obj)->as.file.fptr;
      make_deferred(RANY(obj));
      RDATA(obj)->dfree = (void (*)(void*))rb_io_fptr_finalize;
      RDATA(obj)->data = fptr;
      return 1;
    }
    break;
  case T_ICLASS:
    /* iClass shares table with the module */
    break;

  case T_FLOAT:
  case T_VARMAP:
  case T_BLKTAG:
    break;

  case T_BIGNUM:
    if (RANY(obj)->as.bignum.digits) {
      RUBY_CRITICAL(free(RANY(obj)->as.bignum.digits));
    }
    break;
  case T_NODE:
    switch (nd_type(obj)) {
    case NODE_SCOPE:
      if (RANY(obj)->as.node.u1.tbl) {
        RUBY_CRITICAL(free(RANY(obj)->as.node.u1.tbl));
      }
      break;
    case NODE_ALLOCA:
      RUBY_CRITICAL(free(RANY(obj)->as.node.u1.node));
      break;
    }
    break;			/* no need to free iv_tbl */

  case T_SCOPE:
    if (RANY(obj)->as.scope.local_vars &&
        RANY(obj)->as.scope.flags != SCOPE_ALLOCA) {
      VALUE *vars = RANY(obj)->as.scope.local_vars-1;
      if (!(RANY(obj)->as.scope.flags & SCOPE_CLONE) && vars[0] == 0)
        RUBY_CRITICAL(free(RANY(obj)->as.scope.local_tbl));
      if ((RANY(obj)->as.scope.flags & (SCOPE_MALLOC|SCOPE_CLONE)) == SCOPE_MALLOC)
        RUBY_CRITICAL(free(vars));
    }
    break;

  case T_STRUCT:
    if (RANY(obj)->as.rstruct.ptr) {
      RUBY_CRITICAL(free(RANY(obj)->as.rstruct.ptr));
    }
    break;

  default:
    rb_bug("gc_sweep(): unknown data type 0x%lx(0x%lx)",
           RANY(obj)->as.basic.flags & T_MASK, obj);
  }
#endif  

  rb_gc_force_recycle(obj);

  return destruct_value;
}

VALUE object_free_args(int argc, VALUE *argv, VALUE self)
{
  int i;
  for (i = 0; i < argc; i++)  object_free(argv[i]);
  return Qnil;  
}

void
Init_free()
{
  VALUE cFree = rb_define_module("Free");
  
  rb_define_method(cFree, "free", object_free, 0);
  rb_define_singleton_method(cFree, "free", object_free_args, -1);
}
