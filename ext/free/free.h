#ifndef GUARD_FREE_H
#define GUARD_FREE_H

VALUE rb_object_free(VALUE obj);
VALUE rb_object_free_args(int argc, VALUE *argv, VALUE self);

#endif
