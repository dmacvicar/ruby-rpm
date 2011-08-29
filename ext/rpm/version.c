/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: version.c 22 2004-03-29 03:42:35Z zaki $ */

#include "private.h"

VALUE rpm_cVersion;

static ID id_v;
static ID id_r;
static ID id_e;

/* from rpm-4.0.4/lib/depends.c */
static void
parseEVR(char* evr, const char** ep, const char** vp, const char** rp)
{
	const char* epoch;
	const char* version;	/* assume only version is present */
	const char* release;
#if 0
        naze?
	char* evr;
#endif
	char* s;
	char* se;

	s = evr;
	while (*s && xisdigit(*s)) s++;	/* s points to epoch terminator */
	se = strrchr(s, '-');			/* se points to version terminator */

	if (*s == ':') {
		epoch = evr;
		*s++ = '\0';
		version = s;
		if (*epoch == '\0') epoch = "0";
	}
	else {
		epoch = NULL;				/* disable epoch compare if missing */
		version = evr;
	}

	if (se) {
		*se++ = '\0';
		release = se;
	}
	else {
		release = NULL;
	}

	if (ep) *ep = epoch;
	if (vp) *vp = version;
	if (rp) *rp = release;
}

static void
version_parse(const char* str, VALUE* v, VALUE* r, VALUE* e)
{
	char* evr;
	const char* epoch;
	const char* version;
	const char* release;

	*v = *r = *e = Qnil;

	if (str == NULL) return;

	evr = ALLOCA_N(char, strlen(str)+1);
	strcpy(evr, str);

	parseEVR(evr, &epoch, &version, &release);

	if (epoch && *epoch && atol(epoch) >= 0) {
		if (e) *e = LONG2NUM(atol(epoch));
	}

	if (v) *v = rb_str_new2(version);
	if (release && *release) {
		if (r) *r = rb_str_new2(release);
	}
}

/*
 * @overload new(vr, e = nil)
 *   Creates a version object from a string representation
 *   @param [String] vr version and release in the form "v-r"
 *   @param [Number] e epoch
 *   @return [Version]
 * @overload new(v, r, e = nil)
 *   Creates a version object from a string representation
 *   @param [String] v version
 *   @param [String] r release
 *   @param [Number] e epoch
 *   @return [Version]
 * @example
 *   RPM:: Version.new "1.0.0-3"
 *   RPM:: Version.new "1.04"
 *   RPM:: Version.new "1.0.0-3k", 1
 *   RPM:: Version.new "2.0.3", "5k"
 */
static VALUE
version_initialize(int argc, VALUE* argv, VALUE ver)
{
	VALUE v = Qnil;
	VALUE r = Qnil;
	VALUE e = Qnil;

	switch (argc) {
	case 0:
		rb_raise(rb_eArgError, "argument too few(1..3)");

	case 1:
		if (TYPE(argv[0]) != T_STRING) {
			rb_raise(rb_eTypeError, "illegal argument type");
		}
		version_parse(RSTRING_PTR(argv[0]), &v, &r, &e);
		break;

	case 2:
		if (TYPE(argv[0]) != T_STRING) {
			rb_raise(rb_eTypeError, "illegal argument type");
		}
		version_parse(RSTRING_PTR(argv[0]), &v, &r, &e);
		if (e != Qnil) {
			rb_raise(rb_eTypeError, "illegal argument value");
		}
		e = rb_Integer(argv[1]);
		break;

	case 3:
		if (TYPE(argv[0]) != T_STRING
			|| TYPE(argv[1]) != T_STRING) {
			rb_raise(rb_eTypeError, "illegal argument type");
		}
		v = rb_str_new2(RSTRING_PTR(argv[0]));
		r = rb_str_new2(RSTRING_PTR(argv[1]));
		e = rb_Integer(argv[2]);
		break;

	default:
		rb_raise(rb_eArgError, "argument too many(1..3)");
	}

	if (NIL_P(v) || (!NIL_P(e) && NUM2INT(e) < 0)) {
		rb_raise(rb_eArgError, "illegal argument value");
	}

	rb_ivar_set(ver, id_v, v);
	rb_ivar_set(ver, id_r, r);
	rb_ivar_set(ver, id_e, e);

	return ver;
}

VALUE
rpm_version_new(const char* vr)
{
	VALUE ver;
	VALUE argv[1];
	argv[0] = rb_str_new2(vr);

	ver = rb_newobj();
	OBJSETUP(ver, rpm_cVersion, T_OBJECT);
	rb_obj_call_init(ver, 1, argv);

	return ver;
}

VALUE
rpm_version_new2(const char* vr, int e)
{
	VALUE ver;
	VALUE argv[2];

	argv[0] = rb_str_new2(vr);
	argv[1] = INT2NUM(e);

	ver = rb_newobj();
	OBJSETUP(ver, rpm_cVersion, T_OBJECT);
	rb_obj_call_init(ver, 2, argv);

	return ver;
}

VALUE
rpm_version_new3(const char* v, const char* r, int e)
{
	VALUE ver;
	VALUE argv[3];

	argv[0] = rb_str_new2(v);
	argv[1] = rb_str_new2(r);
	argv[2] = INT2NUM(e);

	ver = rb_newobj();
	OBJSETUP(ver, rpm_cVersion, T_OBJECT);
	rb_obj_call_init(ver, 3, argv);

	return ver;
}

/*
 * Comparison between versions
 * @param [Version] other
 * @return [Number] -1 if +other+ is greater than, 0 if +other+ is equal to,
 *   and +1 if other is less than version.
 *
 * @example
 *   v1 = RPM::Version.new("3.0-0",1)
 *   v2 = RPM::Version.new("3.1-0",1)
 *   v1 <=> v2
 *   => -1
 */
VALUE
rpm_version_cmp(VALUE ver, VALUE other)
{
	VALUE ve,oe;
	VALUE vr,or;
	VALUE vv,ov;
	int sense = 0;

	if (rb_obj_is_kind_of(other, rpm_cVersion) != Qtrue) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	ve = rb_ivar_get(ver,id_e);
	oe = rb_ivar_get(other,id_e);
	if (!NIL_P(ve) && NUM2INT(ve) > 0 && NIL_P(oe))
		return INT2FIX(1);
	else if (NIL_P(ve) && !NIL_P(oe) && NUM2INT(oe) > 0)
		return INT2FIX(-1);
	else if (!NIL_P(ve) && !NIL_P(oe)) {
		if (NUM2INT(ve) < NUM2INT(oe))
			return INT2FIX(-1);
		else if (NUM2INT(ve) > NUM2INT(oe))
			return INT2FIX(1);
	}

	vv = rb_ivar_get(ver,id_v);
	ov = rb_ivar_get(other,id_v);
	if (!NIL_P(vv) && NIL_P(ov))	/* XXX */
		return INT2FIX(1);
	else if (NIL_P(vv) && !NIL_P(ov))	/* XXX */
		return INT2FIX(-1);
	else if (!NIL_P(vv) && !NIL_P(ov)) {
		sense = rpmvercmp(RSTRING_PTR(vv), RSTRING_PTR(ov));
		if (sense)
			return INT2FIX(sense);
	}

	vr = rb_ivar_get(ver,id_r);
	or = rb_ivar_get(other,id_r);
	if (!NIL_P(vr) && NIL_P(or))
		return INT2FIX(1);
	else if (NIL_P(vr) && !NIL_P(or))
		return INT2FIX(-1);
	else if (!NIL_P(vr) && !NIL_P(or)) {
		sense = rpmvercmp(RSTRING_PTR(vr), RSTRING_PTR(or));
	}
	return INT2FIX(sense);
}

/*
 * @param [Version] other Version to compare against
 * @return [Boolean] true if the version is newer than +other+
 */
VALUE
rpm_version_is_newer(VALUE ver, VALUE other)
{
	return (NUM2INT(rpm_version_cmp(ver, other)) > 0) ? Qtrue : Qfalse;
}

/*
 * @param [Version] other Version to compare against
 * @return [Boolean] true if the version is older than +other+
 */
VALUE
rpm_version_is_older(VALUE ver, VALUE other)
{
	return rpm_version_is_newer(ver, other) ? Qfalse : Qtrue;
}

/*
 * Access the version component
 * @return [String] version component
 */
VALUE
rpm_version_get_v(VALUE ver)
{
	return rb_ivar_get(ver, id_v);
}

/*
 * Access the release component
 * @return [String] release component or nil if the
 *   version does not have a release component
 */
VALUE
rpm_version_get_r(VALUE ver)
{
	return rb_ivar_get(ver, id_r);
}

/*
 * Access the epoch component
 * @return [Number] epoch component or nil if the
 *   version does not have an epoch component
 */
VALUE
rpm_version_get_e(VALUE ver)
{
	return rb_ivar_get(ver, id_e);
}

/*
 * String representation in the form "v-r"
 * @return [String]
 * @note The epoch is not included
 */
VALUE
rpm_version_to_s(VALUE ver)
{
	char buf[BUFSIZ];
	char *p = buf;
	VALUE v, r;
	v = rb_ivar_get(ver, id_v);
	r = rb_ivar_get(ver, id_r);
	strcpy(p, RSTRING_PTR(v));
	if (!NIL_P(r)) {
		strcat(p, "-");
		strcat(p, RSTRING_PTR(r));
	}
	return rb_str_new2(buf);
}

/*
 * String representation in the form "e:v-r"
 * @return [String]
 * @note The epoch is included
 */
VALUE
rpm_version_to_vre(VALUE ver)
{
	char buf[BUFSIZ];
	char *p = buf;
	VALUE v, r, e;
	v = rb_ivar_get(ver, id_v);
	r = rb_ivar_get(ver, id_r);
	e = rb_ivar_get(ver, id_e);
	if (!NIL_P(e)) {
		snprintf(buf,BUFSIZ,"%ld:", (long) NUM2INT(e));
		p += strlen(buf);
        }
	strcpy(p, RSTRING_PTR(v));
	if (!NIL_P(r)) {
		strcat(p, "-");
		strcat(p, RSTRING_PTR(r));
	}
	return rb_str_new2(buf);
}

/*
 * Inspect the version object
 * @return [String] object in the form "#<RPM::Version v=V, r=R, e=E>"
 */
VALUE
rpm_version_inspect(VALUE ver)
{
	char buf[BUFSIZ];
	VALUE v, r, e;
	v = rb_ivar_get(ver, id_v);
	r = rb_ivar_get(ver, id_r);
	e = rb_ivar_get(ver, id_e);

	if (!NIL_P(e)) {
		snprintf(buf, BUFSIZ, "#<RPM::Version v=%s, r=%s, e=%ld>", RSTRING_PTR(rb_inspect(v)), RSTRING_PTR(rb_inspect(r)), (long) NUM2INT(e));
        } else {
		snprintf(buf, BUFSIZ, "#<RPM::Version v=%s, r=%s>", RSTRING_PTR(rb_inspect(v)), RSTRING_PTR(rb_inspect(r)));
	}

	return rb_str_new2(buf);
}

/*
 * Hash based on the version content
 * @return [String]
 */

VALUE
rpm_version_hash(VALUE ver)
{
    long h;
    VALUE v, r, e;
	v = rb_ivar_get(ver, id_v);
	r = rb_ivar_get(ver, id_r);
	e = rb_ivar_get(ver, id_e);

    h = NIL_P(e) ? 0 : NUM2INT(e);
    h = (h << 1) ^ NUM2LONG(rb_hash(r));
    h = (h << 1) ^ NUM2LONG(rb_hash(v));
    return LONG2FIX(h);
}

void
Init_rpm_version(void)
{
	rpm_cVersion = rb_define_class_under(rpm_mRPM, "Version", rb_cObject);
	rb_include_module(rpm_cVersion, rb_mComparable);
	rb_define_method(rpm_cVersion, "initialize", version_initialize, -1);
	rb_define_method(rpm_cVersion, "<=>", rpm_version_cmp, 1);
	rb_define_method(rpm_cVersion, "newer?", rpm_version_is_newer, 1);
	rb_define_method(rpm_cVersion, "older?", rpm_version_is_older, 1);
	rb_define_method(rpm_cVersion, "v", rpm_version_get_v, 0);
	rb_define_method(rpm_cVersion, "r", rpm_version_get_r, 0);
	rb_define_method(rpm_cVersion, "e", rpm_version_get_e, 0);
	rb_define_method(rpm_cVersion, "to_s", rpm_version_to_s, 0);
	rb_define_method(rpm_cVersion, "to_vre", rpm_version_to_vre, 0);
	rb_define_method(rpm_cVersion, "inspect", rpm_version_inspect, 0);
    rb_define_method(rpm_cVersion, "hash", rpm_version_hash, 0);

	id_v = rb_intern("version");
	id_r = rb_intern("release");
	id_e = rb_intern("epoch");
}
