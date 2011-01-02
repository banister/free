#ifndef GUARD_FREE_H
#define GUARD_FREE_H


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
	struct RTypedData   typeddata;
	struct RStruct rstruct;
	struct RBignum bignum;
	struct RFile   file;
	struct RNode   node;
	struct RMatch  match;
	struct RRational rational;
	struct RComplex complex;
    } as;
#ifdef GC_DEBUG
    const char *file;
    int   line;
#endif
} RVALUE;

#define RANY(o) ((RVALUE*)(o))

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

#endif
