/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: dependency.c 42 2004-06-03 14:26:37Z kazuhiko $ */

#include "private.h"

VALUE rpm_cDependency;
VALUE rpm_cProvide;
VALUE rpm_cRequire;
VALUE rpm_cConflict;
VALUE rpm_cObsolete;

static ID id_name;
static ID id_ver;
static ID id_flags;
static ID id_owner;

static ID id_nametag;
static ID id_versiontag;
static ID id_flagstag;

static VALUE
dependency_initialize(VALUE dep, VALUE name, VALUE version,
					  VALUE flags, VALUE owner)
{
	if (TYPE(name) != T_STRING
		|| (!NIL_P(version) && rb_obj_is_kind_of(version, rpm_cVersion) == Qfalse)) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	rb_ivar_set(dep, id_name, name);
	rb_ivar_set(dep, id_ver, version);
	rb_ivar_set(dep, id_flags, rb_Integer(flags));
	rb_ivar_set(dep, id_owner, owner);
	return dep;
}

static VALUE
provide_initialize(VALUE dep, VALUE name, VALUE version,
					  VALUE flags, VALUE owner)
{
	dependency_initialize(dep,name,version,flags,owner);
	rb_ivar_set(dep, id_nametag, INT2NUM(RPMTAG_PROVIDENAME));
	rb_ivar_set(dep, id_versiontag, INT2NUM(RPMTAG_PROVIDEVERSION));
	rb_ivar_set(dep, id_flagstag, INT2NUM(RPMTAG_PROVIDEFLAGS));
	return dep;
}

static VALUE
require_initialize(VALUE dep, VALUE name, VALUE version,
					  VALUE flags, VALUE owner)
{
	dependency_initialize(dep,name,version,flags,owner);
	rb_ivar_set(dep, id_nametag, INT2NUM(RPMTAG_REQUIRENAME));
	rb_ivar_set(dep, id_versiontag, INT2NUM(RPMTAG_REQUIREVERSION));
	rb_ivar_set(dep, id_flagstag, INT2NUM(RPMTAG_REQUIREFLAGS));
	return dep;
}

static VALUE
obsolete_initialize(VALUE dep, VALUE name, VALUE version,
					  VALUE flags, VALUE owner)
{
	dependency_initialize(dep,name,version,flags,owner);
	rb_ivar_set(dep, id_nametag, INT2NUM(RPMTAG_OBSOLETENAME));
	rb_ivar_set(dep, id_versiontag, INT2NUM(RPMTAG_OBSOLETEVERSION));
	rb_ivar_set(dep, id_flagstag, INT2NUM(RPMTAG_OBSOLETEFLAGS));
	return dep;
}

static VALUE
conflict_initialize(VALUE dep, VALUE name, VALUE version,
					  VALUE flags, VALUE owner)
{
	dependency_initialize(dep,name,version,flags,owner);
	rb_ivar_set(dep, id_nametag, INT2NUM(RPMTAG_CONFLICTNAME));
	rb_ivar_set(dep, id_versiontag, INT2NUM(RPMTAG_CONFLICTVERSION));
	rb_ivar_set(dep, id_flagstag, INT2NUM(RPMTAG_CONFLICTFLAGS));
	return dep;
}

VALUE
rpm_provide_new(const char* name, VALUE version, int flags, VALUE target)
{
	VALUE prov, argv[4];

	argv[0] = rb_str_new2(name);
	argv[1] = version;
	argv[2] = INT2NUM(flags);
	argv[3] = target;

	prov = rb_newobj();
	OBJSETUP(prov, rpm_cProvide, T_OBJECT);
	rb_obj_call_init(prov, 4, argv);
	rb_ivar_set(prov, id_nametag, INT2NUM(RPMTAG_PROVIDENAME));
	rb_ivar_set(prov, id_versiontag, INT2NUM(RPMTAG_PROVIDEVERSION));
	rb_ivar_set(prov, id_flagstag, INT2NUM(RPMTAG_PROVIDEFLAGS));
	return prov;
}

VALUE
rpm_require_new(const char* name, VALUE version, int flags, VALUE target)
{
	VALUE req, argv[4];

	argv[0] = rb_str_new2(name);
	argv[1] = version;
	argv[2] = INT2NUM(flags);
	argv[3] = target;

	req = rb_newobj();
	OBJSETUP(req, rpm_cRequire, T_OBJECT);
	rb_obj_call_init(req, 4, argv);
	rb_ivar_set(req, id_nametag, INT2NUM(RPMTAG_REQUIRENAME));
	rb_ivar_set(req, id_versiontag, INT2NUM(RPMTAG_REQUIREVERSION));
	rb_ivar_set(req, id_flagstag, INT2NUM(RPMTAG_REQUIREFLAGS));
	return req;
}

VALUE
rpm_conflict_new(const char* name, VALUE version, int flags, VALUE target)
{
	VALUE conf, argv[4];

	argv[0] = rb_str_new2(name);
	argv[1] = version;
	argv[2] = INT2NUM(flags);
	argv[3] = target;

	conf = rb_newobj();
	OBJSETUP(conf, rpm_cConflict, T_OBJECT);
	rb_obj_call_init(conf, 4, argv);
	rb_ivar_set(conf, id_nametag, INT2NUM(RPMTAG_CONFLICTNAME));
	rb_ivar_set(conf, id_versiontag, INT2NUM(RPMTAG_CONFLICTVERSION));
	rb_ivar_set(conf, id_flagstag, INT2NUM(RPMTAG_CONFLICTFLAGS));
	return conf;
}

VALUE
rpm_obsolete_new(const char* name, VALUE version, int flags, VALUE target)
{
	VALUE obso, argv[4];

	argv[0] = rb_str_new2(name);
	argv[1] = version;
	argv[2] = INT2NUM(flags);
	argv[3] = target;

	obso = rb_newobj();
	OBJSETUP(obso, rpm_cObsolete, T_OBJECT);
	rb_obj_call_init(obso, 4, argv);
	rb_ivar_set(obso, id_nametag, INT2NUM(RPMTAG_OBSOLETENAME));
	rb_ivar_set(obso, id_versiontag, INT2NUM(RPMTAG_OBSOLETEVERSION));
	rb_ivar_set(obso, id_flagstag, INT2NUM(RPMTAG_OBSOLETEFLAGS));
	return obso;
}

VALUE
rpm_dependency_is_satisfy(VALUE dep,VALUE other)
{
	int oflag;
    int sflag;
	char *svre;
	char *ovre;
	char *name;
    char *oname;
	if (rb_obj_is_kind_of(other,rpm_cPackage) == Qtrue){
		VALUE provide;
		VALUE provides = rpm_package_get_provides(other);
		while (!NIL_P(provide = rb_ary_pop(provides))){
            if (rpm_dependency_is_satisfy(dep,provide) == Qtrue) {
                return Qtrue;
			}
		}
		return Qfalse;
	}

	name = RSTRING_PTR(rb_ivar_get(dep,id_name));
	svre = RSTRING_PTR(rpm_version_to_vre(rb_ivar_get(dep,id_ver)));
    sflag = NUM2INT(rb_ivar_get(dep, id_flags));

	if (rb_obj_is_kind_of(other,rpm_cDependency) == Qtrue){
		oflag = NUM2INT(rb_ivar_get(other, id_flags));
        oname = RSTRING_PTR(rb_ivar_get(other, id_name));
		ovre = RSTRING_PTR(rpm_version_to_vre(rb_ivar_get(other,id_ver)));
		other = rb_ivar_get(other,id_ver);
	} else if (rb_obj_is_kind_of(other,rpm_cVersion) == Qtrue){
		ovre = RSTRING_PTR(rpm_version_to_vre(other));
        oname = name;
		if (!*ovre)
			oflag = 0;
		else
			oflag = RPMSENSE_EQUAL;
	} else {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	if (rpmRangesOverlap(name,ovre,oflag, sname,svre,sflag))
#else
	if (rpmdsCompare(rpmdsSingle(RPMTAG_PROVIDENAME, oname, ovre, oflag),
			 rpmdsSingle(RPMTAG_PROVIDENAME, name, svre, sflag)))
#endif
		return Qtrue;
	return Qfalse;
}

VALUE
rpm_dependency_get_name(VALUE dep)
{
	return rb_ivar_get(dep, id_name);
}

VALUE
rpm_dependency_get_version(VALUE dep)
{
	return rb_ivar_get(dep, id_ver);
}

VALUE
rpm_dependency_get_flags(VALUE dep)
{
	return rb_ivar_get(dep, id_flags);
}

VALUE
rpm_dependency_get_owner(VALUE dep)
{
	return rb_ivar_get(dep, id_owner);
}

VALUE
rpm_dependency_get_nametag(VALUE dep)
{
	return rb_ivar_get(dep, id_nametag);
}

VALUE
rpm_dependency_get_versiontag(VALUE dep)
{
	return rb_ivar_get(dep, id_versiontag);
}

VALUE
rpm_dependency_get_flagstag(VALUE dep)
{
	return rb_ivar_get(dep, id_flagstag);
}

VALUE
rpm_dependency_is_lt(VALUE dep)
{
	return (NUM2INT(rb_ivar_get(dep, id_flags)) & RPMSENSE_LESS) ? Qtrue : Qfalse;
}

VALUE
rpm_dependency_is_gt(VALUE dep)
{
	return (NUM2INT(rb_ivar_get(dep, id_flags)) & RPMSENSE_GREATER) ? Qtrue : Qfalse;
}

VALUE
rpm_dependency_is_eq(VALUE dep)
{
	return (NUM2INT(rb_ivar_get(dep, id_flags)) & RPMSENSE_EQUAL) ? Qtrue : Qfalse;
}

VALUE
rpm_dependency_is_le(VALUE dep)
{
	int flags = NUM2INT(rb_ivar_get(dep, id_flags));
	return ((flags & RPMSENSE_LESS) && (flags & RPMSENSE_EQUAL)) ? Qtrue : Qfalse;
}

VALUE
rpm_dependency_is_ge(VALUE dep)
{
	int flags = NUM2INT(rb_ivar_get(dep, id_flags));
	return ((flags & RPMSENSE_GREATER) && (flags & RPMSENSE_EQUAL)) ? Qtrue : Qfalse;
}

VALUE
rpm_require_is_pre(VALUE req)
{
	return (NUM2INT(rb_ivar_get(req, id_flags)) & RPMSENSE_PREREQ) ? Qtrue : Qfalse;
}

void
Init_rpm_dependency(void)
{
	rpm_cDependency = rb_define_class_under(rpm_mRPM, "Dependency", rb_cObject);
	rb_define_method(rpm_cDependency, "initialize", dependency_initialize, 4);
	rb_define_method(rpm_cDependency, "name", rpm_dependency_get_name, 0);
	rb_define_method(rpm_cDependency, "version", rpm_dependency_get_version, 0);
	rb_define_method(rpm_cDependency, "flags", rpm_dependency_get_flags, 0);
	rb_define_method(rpm_cDependency, "owner", rpm_dependency_get_owner, 0);
	rb_define_method(rpm_cDependency, "lt?", rpm_dependency_is_lt, 0);
	rb_define_method(rpm_cDependency, "gt?", rpm_dependency_is_gt, 0);
	rb_define_method(rpm_cDependency, "eq?", rpm_dependency_is_eq, 0);
	rb_define_method(rpm_cDependency, "le?", rpm_dependency_is_le, 0);
	rb_define_method(rpm_cDependency, "ge?", rpm_dependency_is_ge, 0);
	rb_define_method(rpm_cDependency, "satisfy?", rpm_dependency_is_satisfy, 1);
	rb_define_method(rpm_cDependency, "nametag", rpm_dependency_get_nametag, 0);
	rb_define_method(rpm_cDependency, "versiontag", rpm_dependency_get_versiontag, 0);
	rb_define_method(rpm_cDependency, "flagstag", rpm_dependency_get_flagstag, 0);

	rpm_cProvide = rb_define_class_under(rpm_mRPM, "Provide", rpm_cDependency);
	rb_define_method(rpm_cProvide, "initialize", provide_initialize, 4);
	rpm_cRequire = rb_define_class_under(rpm_mRPM, "Require", rpm_cDependency);
	rb_define_method(rpm_cRequire, "initialize", require_initialize, 4);
	rpm_cConflict = rb_define_class_under(rpm_mRPM, "Conflict", rpm_cDependency);
	rb_define_method(rpm_cConflict, "initialize", conflict_initialize, 4);
	rpm_cObsolete = rb_define_class_under(rpm_mRPM, "Obsolete", rpm_cDependency);
	rb_define_method(rpm_cObsolete, "initialize", obsolete_initialize, 4);

	rb_define_method(rpm_cRequire, "pre?", rpm_require_is_pre, 0);

	id_name = rb_intern("name");
	id_ver = rb_intern("version");
	id_flags = rb_intern("flags");
	id_owner = rb_intern("owner");

	id_nametag = rb_intern("nametag");
	id_versiontag = rb_intern("versiontag");
	id_flagstag = rb_intern("flagstag");
}
