/* (c) 2010 John Mair (banisterfiend), MIT license */

#include <ruby.h>
#include <ruby/io.h>
#include <ruby/re.h>
#include <vm_core.h>
#include "free.h"
#include "compat.h"

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

static
VALUE obj_free(VALUE obj)
{

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

  rb_gc_force_recycle(obj);

  return Qnil;
}

void
Init_free()
{
  VALUE cFree = rb_define_module("Free");
  
  rb_define_method(cFree, "free", obj_free, 0);
  
}
