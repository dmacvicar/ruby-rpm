/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: file.c 9 2004-03-13 14:19:20Z zaki $ */

#include "private.h"

VALUE rpm_cFile;

static ID id_path;
static ID id_md5sum;
static ID id_link_to;
static ID id_size;
static ID id_mtime;
static ID id_owner;
static ID id_group;
static ID id_rdev;
static ID id_mode;
static ID id_attr;
static ID id_state;

static VALUE
file_initialize(VALUE file, VALUE path, VALUE md5sum, VALUE link_to,
				VALUE size, VALUE mtime, VALUE owner, VALUE group,
				VALUE rdev, VALUE mode, VALUE attr, VALUE state)
{
	if (TYPE(path) != T_STRING
		|| TYPE(md5sum) != T_STRING
		|| (!NIL_P(link_to) && TYPE(link_to) != T_STRING)
		|| (!NIL_P(owner) && TYPE(owner) != T_STRING)
		|| (!NIL_P(group) && TYPE(group) != T_STRING)) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}
	rb_ivar_set(file, id_path, path);
	rb_ivar_set(file, id_md5sum, md5sum);
	rb_ivar_set(file, id_link_to, (!NIL_P(link_to) && RSTRING_LEN(link_to)) ? link_to : Qnil);
	rb_ivar_set(file, id_size, rb_Integer(size));
	if (rb_obj_is_kind_of(mtime, rb_cTime) == Qfalse) {
		mtime = rb_time_new(NUM2INT(rb_Integer(mtime)), (time_t)0);
	}
	rb_ivar_set(file, id_mtime, mtime);
	rb_ivar_set(file, id_owner, owner);
	rb_ivar_set(file, id_group, group);
	rb_ivar_set(file, id_rdev, rb_Integer(rdev));
	rb_ivar_set(file, id_mode, UINT2NUM(NUM2UINT(rb_Integer(mode))&0777));
	rb_ivar_set(file, id_attr, rb_Integer(attr));
	rb_ivar_set(file, id_state, rb_Integer(state));

	return file;
}

VALUE
rpm_file_new(const char* path, const char* md5sum, const char* link_to,
			 size_t size, time_t mtime, const char* owner, const char* group,
			 dev_t rdev, mode_t mode, rpmfileAttrs attr, rpmfileState state)
{
	VALUE file, argv[11];

	argv[0] = rb_str_new2(path);
	argv[1] = rb_str_new2(md5sum);
	argv[2] = link_to ? rb_str_new2(link_to) : Qnil;
	argv[3] = UINT2NUM(size);
	argv[4] = rb_time_new(mtime, (time_t)0);
	argv[5] = owner ? rb_str_new2(owner) : Qnil;
	argv[6] = group ? rb_str_new2(group) : Qnil;
	argv[7] = UINT2NUM(rdev);
	argv[8] = UINT2NUM(mode);
	argv[9] = INT2NUM(attr);
	argv[10] = INT2NUM(state);

	file = rb_newobj();
	OBJSETUP(file, rpm_cFile, T_OBJECT);
	rb_obj_call_init(file, 11, argv);

	return file;
}

/*
 * @return [String] file path
 */
VALUE
rpm_file_get_path(VALUE file)
{
	return rb_ivar_get(file, id_path);
}

/*
 * @return [String] md5sum as string
 */
VALUE
rpm_file_get_md5sum(VALUE file)
{
	return rb_ivar_get(file, id_md5sum);
}

/*
 * @return [String] Path to the destination if the file is a symbolic link
 * @note
 *   This path is sometimes relative. To convert an absolute path from relative path:
 *   File.expand_path (file.link_to, File.dirname (file.path))
 */
VALUE
rpm_file_get_link_to(VALUE file)
{
	return rb_ivar_get(file, id_link_to);
}

/*
 * @return [Number] File size
 */
VALUE
rpm_file_get_size(VALUE file)
{
	return rb_ivar_get(file, id_size);
}

/*
 * @return [Time] File modification time.
 */
VALUE
rpm_file_get_mtime(VALUE file)
{
	return rb_ivar_get(file, id_mtime);
}

/*
 * @return [String] File owner. Nil may be returned.
 */
VALUE
rpm_file_get_owner(VALUE file)
{
	return rb_ivar_get(file, id_owner);
}

/*
 * @return [String] Group that owns the file. Nil may be returned.
 */
VALUE
rpm_file_get_group(VALUE file)
{
	return rb_ivar_get(file, id_group);
}

/*
 * @return [Number] Device type of the file
 */
VALUE
rpm_file_get_rdev(VALUE file)
{
	return rb_ivar_get(file, id_rdev);
}

/*
 * @return [Number] File permissions
 */
VALUE
rpm_file_get_mode(VALUE file)
{
	return rb_ivar_get(file, id_mode);
}

VALUE
rpm_file_get_attr(VALUE file)
{
	return rb_ivar_get(file, id_attr);
}

VALUE
rpm_file_get_state(VALUE file)
{
	return rb_ivar_get(file, id_state);
}

/*
 * @return [Boolean] True if the file is a symbolic link
 */
VALUE
rpm_file_is_symlink(VALUE file)
{
	return NIL_P(rb_ivar_get(file, id_link_to)) ? Qfalse : Qtrue;
}

/*
 * @return [Boolean] True if the file is marked as a configuration file
 */
VALUE
rpm_file_is_config(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_CONFIG) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is marked as documentation
 */
VALUE
rpm_file_is_doc(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_DOC) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is marked as do not use
 */
VALUE
rpm_file_is_donotuse(VALUE file)
{
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_DONOTUSE) ? Qtrue : Qfalse;
#else
	return Qfalse;
#endif
}

/*
 * @return [Boolean] True if the file is marked that can be missing on disk
 *
 * This modifier is used for files or links that are created during the %post scripts
 *   but will need to be removed if the package is removed
 */
VALUE
rpm_file_is_missingok(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_MISSINGOK) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is marked as configuration not to be replaced
 *
 * This flag is used to protect local modifications.
 * If used, the file will not overwrite an existing file that has been modified.
 * If the file has not been modified on disk, the rpm command will overwrite the file. But,
 *   if the file has been modified on disk, the rpm command will copy the new file with an extra
 *   file-name extension of .rpmnew.
 */
VALUE
rpm_file_is_noreplace(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_NOREPLACE) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is marked as a spec file
 */
VALUE
rpm_file_is_specfile(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_SPECFILE) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is marked as ghost
 *
 * This flag indicates the file should not be included in the package.
 * It can be used to name the needed attributes for a file that the program, when installed,
 *   will create.
 * For example, you may want to ensure that a program’s log file has certain attributes.
 */
VALUE
rpm_file_is_ghost(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_GHOST) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is a license
 */
VALUE
rpm_file_is_license(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_LICENSE) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is a README
 */
VALUE
rpm_file_is_readme(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_README) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is listed in the exlude section
 */
VALUE
rpm_file_is_exclude(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_attr)) & RPMFILE_EXCLUDE) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is replaced during installation
 */
VALUE
rpm_file_is_replaced(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_state))
			== RPMFILE_STATE_REPLACED) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is not installed
 */
VALUE
rpm_file_is_notinstalled(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_state))
			== RPMFILE_STATE_NOTINSTALLED) ? Qtrue : Qfalse;
}

/*
 * @return [Boolean] True if the file is shared over the network
 */
VALUE
rpm_file_is_netshared(VALUE file)
{
	return (NUM2INT(rb_ivar_get(file, id_state))
			== RPMFILE_STATE_NETSHARED) ? Qtrue : Qfalse;
}

void
Init_rpm_file(void)
{
	rpm_cFile = rb_define_class_under(rpm_mRPM, "File", rb_cObject);
	rb_define_method(rpm_cFile, "initialize", file_initialize, 11);
	rb_define_method(rpm_cFile, "path", rpm_file_get_path, 0);
	rb_define_alias(rpm_cFile, "to_s", "path");
	rb_define_method(rpm_cFile, "md5sum", rpm_file_get_md5sum, 0);
	rb_define_method(rpm_cFile, "link_to", rpm_file_get_link_to, 0);
	rb_define_method(rpm_cFile, "size", rpm_file_get_size, 0);
	rb_define_method(rpm_cFile, "mtime", rpm_file_get_mtime, 0);
	rb_define_method(rpm_cFile, "owner", rpm_file_get_owner, 0);
	rb_define_method(rpm_cFile, "group", rpm_file_get_group, 0);
	rb_define_method(rpm_cFile, "rdev", rpm_file_get_rdev, 0);
	rb_define_method(rpm_cFile, "mode", rpm_file_get_mode, 0);
	rb_define_method(rpm_cFile, "attr", rpm_file_get_attr, 0);
	rb_define_method(rpm_cFile, "state", rpm_file_get_state, 0);
	rb_define_method(rpm_cFile, "symlink?", rpm_file_is_symlink, 0);
	rb_define_method(rpm_cFile, "config?", rpm_file_is_config, 0);
	rb_define_method(rpm_cFile, "doc?", rpm_file_is_doc, 0);
	rb_define_method(rpm_cFile, "donotuse?", rpm_file_is_donotuse, 0);
	rb_define_method(rpm_cFile, "missingok?", rpm_file_is_missingok, 0);
	rb_define_method(rpm_cFile, "noreplace?", rpm_file_is_noreplace, 0);
	rb_define_method(rpm_cFile, "specfile?", rpm_file_is_specfile, 0);
	rb_define_method(rpm_cFile, "ghost?", rpm_file_is_ghost, 0);
	rb_define_method(rpm_cFile, "license?", rpm_file_is_license, 0);
	rb_define_method(rpm_cFile, "readme?", rpm_file_is_readme, 0);
	rb_define_method(rpm_cFile, "exclude?", rpm_file_is_exclude, 0);
	rb_define_method(rpm_cFile, "replaced?", rpm_file_is_replaced, 0);
	rb_define_method(rpm_cFile, "notinstalled?", rpm_file_is_notinstalled, 0);
	rb_define_method(rpm_cFile, "netshared?", rpm_file_is_netshared, 0);

	id_path = rb_intern("path");
	id_md5sum = rb_intern("md5sum");
	id_link_to = rb_intern("link_to");
	id_size = rb_intern("size");
	id_mtime = rb_intern("mtime");
	id_owner = rb_intern("owner");
	id_group = rb_intern("group");
	id_rdev = rb_intern("rdev");
	id_mode = rb_intern("mode");
	id_attr = rb_intern("attr");
	id_state = rb_intern("state");
}
