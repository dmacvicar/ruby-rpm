/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: source.c 22 2004-03-29 03:42:35Z zaki $ */

#include "private.h"

VALUE rpm_cSource;
VALUE rpm_cPatch;
VALUE rpm_cIcon;

static ID id_full;
static ID id_fn;
static ID id_num;
static ID id_no;

static VALUE
source_initialize(int argc, VALUE* argv, VALUE src)
{
	switch (argc) {
	case 0: case 1:
		rb_raise(rb_eArgError, "argument too few(2..3)");

	case 2: case 3:
		if (TYPE(argv[0]) != T_STRING) {
			rb_raise(rb_eTypeError, "illegal argument type");
		}

		rb_ivar_set(src, id_full, argv[0]);
		rb_ivar_set(src, id_num, rb_Integer(argv[1]));
		if (argc == 3) {
			rb_ivar_set(src, id_no, RTEST(argv[2]) ? Qtrue : Qfalse);
		} else {
			rb_ivar_set(src, id_no, Qfalse);
		}
		break;

	default:
		rb_raise(rb_eArgError, "argument too many(2..3)");
	}

	return src;
}

VALUE
rpm_source_new(const char* fullname, unsigned int num, int no)
{
	VALUE src, argv[3];

	argv[0] = rb_str_new2(fullname);
	argv[1] = UINT2NUM(num);
	argv[2] = no ? Qtrue : Qfalse;

	src = rb_newobj();
	OBJSETUP(src, rpm_cSource, T_OBJECT);
	rb_obj_call_init(src, 3, argv);

	return src;
}

VALUE
rpm_patch_new(const char* fullname, unsigned int num, int no)
{
	VALUE src, argv[3];

	argv[0] = rb_str_new2(fullname);
	argv[1] = UINT2NUM(num);
	argv[2] = no ? Qtrue : Qfalse;

	src = rb_newobj();
	OBJSETUP(src, rpm_cPatch, T_OBJECT);
	rb_obj_call_init(src, 3, argv);

	return src;
}

VALUE
rpm_icon_new(const char* fullname, unsigned int num, int no)
{
	VALUE src, argv[3];

	argv[0] = rb_str_new2(fullname);
	argv[1] = UINT2NUM(num);
	argv[2] = no ? Qtrue : Qfalse;

	src = rb_newobj();
	OBJSETUP(src, rpm_cIcon, T_OBJECT);
	rb_obj_call_init(src, 3, argv);

	return src;
}

VALUE
rpm_source_get_fullname(VALUE src)
{
	return rb_ivar_get(src, id_full);
}

VALUE
rpm_source_get_filename(VALUE src)
{
	VALUE fn = rb_ivar_get(src, id_fn);

	if (NIL_P(fn)) {
		VALUE full = rb_ivar_get(src, id_full);
		const char* p = strrchr(RSTRING_PTR(full), '/');
		if (p == NULL) {
			p = RSTRING_PTR(full);
		} else {
			p++;
		}
		fn = rb_str_new2(p);
		rb_ivar_set(src, id_fn, fn);
	}

	return fn;
}

VALUE
rpm_source_get_num(VALUE src)
{
	return rb_ivar_get(src, id_num);
}

VALUE
rpm_source_is_no(VALUE src)
{
	return rb_ivar_get(src, id_no);
}

void
Init_rpm_source(void)
{
	rpm_cSource = rb_define_class_under(rpm_mRPM, "Source", rb_cObject);
	rb_define_method(rpm_cSource, "initialize", source_initialize, -1);
	rb_define_method(rpm_cSource, "fullname", rpm_source_get_fullname, 0);
	rb_define_alias(rpm_cSource, "to_s", "fullname");
	rb_define_method(rpm_cSource, "filename", rpm_source_get_filename, 0);
	rb_define_method(rpm_cSource, "num", rpm_source_get_num, 0);
	rb_define_method(rpm_cSource, "no?", rpm_source_is_no, 0);

	rpm_cPatch = rb_define_class_under(rpm_mRPM, "Patch", rpm_cSource);
	rpm_cIcon = rb_define_class_under(rpm_mRPM, "Icon", rpm_cSource);

	id_full = rb_intern("fullname");
	id_fn = rb_intern("filename");
	id_num = rb_intern("num");
	id_no = rb_intern("no");
}
