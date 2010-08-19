/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: ruby-rpm.h 27 2004-05-23 04:54:24Z zaki $ */

#ifndef ruby_rpm_h_Included
#define ruby_rpm_h_Included 1

#include <ruby.h>

extern VALUE rpm_mRPM;
extern VALUE rpm_cConflict;
extern VALUE rpm_cDB;
extern VALUE rpm_cDependency;
extern VALUE rpm_cFile;
extern VALUE rpm_cIcon;
extern VALUE rpm_cMatchIterator;
extern VALUE rpm_cObsolete;
extern VALUE rpm_cPackage;
extern VALUE rpm_cPatch;
extern VALUE rpm_cProvide;
extern VALUE rpm_cRequire;
extern VALUE rpm_cSource;
extern VALUE rpm_cSpec;
extern VALUE rpm_cTransaction;
extern VALUE rpm_cVersion;
extern VALUE rpm_sChangeLog;
extern VALUE rpm_sCallbackData;
extern VALUE rpm_sProblem;

/* db.c */
VALUE rpm_db_open(int writable, const char* root);
void rpm_db_init(const char* root, int writable);
void rpm_db_rebuild(const char* root);
VALUE rpm_db_get_root(VALUE db);
VALUE rpm_db_get_home(VALUE db);
VALUE rpm_db_is_writable(VALUE db);
VALUE rpm_db_each_match(VALUE db, VALUE key, VALUE val);
VALUE rpm_db_each(VALUE db);
VALUE rpm_db_transaction(int argc, VALUE* argv, VALUE db);
VALUE rpm_transaction_get_db(VALUE trans);
VALUE rpm_transaction_get_script_file(VALUE trans);
VALUE rpm_transaction_set_script_file(VALUE trans, VALUE file);
VALUE rpm_transaction_install(VALUE trans, VALUE pkg, VALUE key);
VALUE rpm_transaction_upgrade(VALUE trans, VALUE pkg, VALUE key);
VALUE rpm_transaction_available(VALUE trans, VALUE pkg, VALUE key);
VALUE rpm_transaction_delete(VALUE trans, VALUE name);
VALUE rpm_transaction_check(VALUE trans);
VALUE rpm_transaction_order(VALUE trans);
VALUE rpm_transaction_keys(VALUE trans);
VALUE rpm_transaction_commit(int argc, VALUE* argv, VALUE trans);
VALUE rpm_transaction_abort(VALUE trans);
VALUE rpm_db_init_iterator(VALUE db, VALUE key, VALUE val);
VALUE rpm_mi_next_iterator(VALUE mi);
VALUE rpm_mi_each(VALUE mi);
VALUE rpm_mi_get_iterator_count(VALUE mi);
VALUE rpm_mi_get_iterator_offset(VALUE mi);
VALUE rpm_mi_set_iterator_re(VALUE mi,VALUE tag, VALUE mode, VALUE re);
VALUE rpm_mi_set_iterator_version(VALUE mi, VALUE version);


/* dependency.c */
VALUE rpm_provide_new(const char* name, VALUE version, int flags, VALUE target);
VALUE rpm_require_new(const char* name, VALUE version, int flags, VALUE target);
VALUE rpm_conflict_new(const char* name, VALUE version, int flags, VALUE target);
VALUE rpm_obsolete_new(const char* name, VALUE version, int flags, VALUE target);
VALUE rpm_dependency_get_name(VALUE dep);
VALUE rpm_dependency_get_version(VALUE dep);
VALUE rpm_dependency_get_flags(VALUE dep);
VALUE rpm_dependency_get_target(VALUE dep);
VALUE rpm_dependency_is_lt(VALUE dep);
VALUE rpm_dependency_is_gt(VALUE dep);
VALUE rpm_dependency_is_eq(VALUE dep);
VALUE rpm_dependency_is_le(VALUE dep);
VALUE rpm_dependency_is_ge(VALUE dep);
VALUE rpm_require_is_pre(VALUE req);
VALUE rpm_dependency_get_nametag(VALUE dep);
VALUE rpm_dependency_get_versiontag(VALUE dep);
VALUE rpm_dependency_get_flagstag(VALUE dep);


/* file.c */
VALUE rpm_file_new(const char* path, const char* md5sum, const char* link_to,
				   size_t size, time_t mtime, const char* owner, const char* group,
				   dev_t rdev, mode_t mode, rpmfileAttrs attr, rpmfileState state);
VALUE rpm_file_get_path(VALUE file);
VALUE rpm_file_get_md5sum(VALUE file);
VALUE rpm_file_get_link_to(VALUE file);
VALUE rpm_file_get_size(VALUE file);
VALUE rpm_file_get_mtime(VALUE file);
VALUE rpm_file_get_owner(VALUE file);
VALUE rpm_file_get_group(VALUE file);
VALUE rpm_file_get_rdev(VALUE file);
VALUE rpm_file_get_mode(VALUE file);
VALUE rpm_file_get_attr(VALUE file);
VALUE rpm_file_get_state(VALUE file);
VALUE rpm_file_is_symlink(VALUE file);
VALUE rpm_file_is_config(VALUE file);
VALUE rpm_file_is_doc(VALUE file);
VALUE rpm_file_is_donotuse(VALUE file);
VALUE rpm_file_is_missingok(VALUE file);
VALUE rpm_file_is_noreplace(VALUE file);
VALUE rpm_file_is_specfile(VALUE file);
VALUE rpm_file_is_ghost(VALUE file);
VALUE rpm_file_is_license(VALUE file);
VALUE rpm_file_is_readme(VALUE file);
VALUE rpm_file_is_exclude(VALUE file);
VALUE rpm_file_is_replaced(VALUE file);
VALUE rpm_file_is_notinstalled(VALUE file);
VALUE rpm_file_is_netshared(VALUE file);

/* package.c */
VALUE rpm_package_new_from_header(Header hdr);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
VALUE rpm_package_new_from_N_EVR(VALUE name, VALUE version);
#endif
VALUE rpm_package_aref(VALUE pkg, VALUE tag);
VALUE rpm_package_get_name(VALUE pkg);
VALUE rpm_package_get_version(VALUE pkg);
VALUE rpm_package_get_signature(VALUE pkg);
VALUE rpm_package_get_files(VALUE pkg);
VALUE rpm_package_get_provides(VALUE pkg);
VALUE rpm_package_get_requires(VALUE pkg);
VALUE rpm_package_get_conflicts(VALUE pkg);
VALUE rpm_package_get_obsoletes(VALUE pkg);
VALUE rpm_package_get_changelog(VALUE pkg);
VALUE rpm_package_dump(VALUE pkg);
VALUE rpm_package_add_dependency(VALUE pkg,VALUE dep);
VALUE rpm_package_add_string(VALUE pkg,VALUE tag,VALUE val);
VALUE rpm_package_add_string_array(VALUE pkg,VALUE tag,VALUE val);
VALUE rpm_package_add_int32(VALUE pkg,VALUE tag,VALUE val);
VALUE rpm_package_add_binary(VALUE pkg,VALUE tag,VALUE val);


/* rpm.c */
VALUE rpm_macro_aref(VALUE name);
VALUE rpm_macro_aset(VALUE name, VALUE val);
void rpm_readrc(const char* rcpath);
void rpm_init_marcros(const char* path);
VALUE rpm_get_verbosity(void);
void rpm_set_verbosity(VALUE verbosity);

/* source.c */
VALUE rpm_source_new(const char* fullname, unsigned int num, int no);
VALUE rpm_patch_new(const char* fullname, unsigned int num, int no);
VALUE rpm_icon_new(const char* fullname, unsigned int num, int no);
VALUE rpm_source_get_fullname(VALUE src);
VALUE rpm_source_get_filename(VALUE src);
VALUE rpm_source_get_num(VALUE src);
VALUE rpm_source_is_no(VALUE src);

/* spec.c */
VALUE rpm_spec_open(const char* filename);
VALUE rpm_spec_get_buildroot(VALUE spec);
VALUE rpm_spec_get_buildsubdir(VALUE spec);
VALUE rpm_spec_get_buildarchs(VALUE spec);
VALUE rpm_spec_get_buildrequires(VALUE spec);
VALUE rpm_spec_get_buildconflicts(VALUE spec);
VALUE rpm_spec_get_build_restrictions(VALUE spec);
VALUE rpm_spec_get_sources(VALUE spec);
VALUE rpm_spec_get_packages(VALUE spec);

/* version.c */
VALUE rpm_version_new(const char* vr);
VALUE rpm_version_new2(const char* vr, int e);
VALUE rpm_version_new3(const char* v, const char* r, int e);
VALUE rpm_version_cmp(VALUE ver, VALUE other);
VALUE rpm_version_is_newer(VALUE ver, VALUE other);
VALUE rpm_version_is_older(VALUE ver, VALUE other);
VALUE rpm_version_get_v(VALUE ver);
VALUE rpm_version_get_r(VALUE ver);
VALUE rpm_version_get_e(VALUE ver);
VALUE rpm_version_to_s(VALUE ver);
VALUE rpm_version_to_vre(VALUE ver);

#endif /* !ruby_rpm_h_Included */
