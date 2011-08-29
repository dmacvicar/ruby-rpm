/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: db.c 45 2004-06-04 15:11:20Z kazuhiko $ */

#include "private.h"

VALUE rpm_cDB;
VALUE rpm_cTransaction;
VALUE rpm_cMatchIterator;
VALUE rpm_sCallbackData;
VALUE rpm_sProblem;

static ID id_db;
static ID id_sf;
static ID id_keys;
static ID id_commited;
static ID id_aborted;
static ID id_pl;
static ID id_type;
static ID id_key;
static ID id_pkg;
static ID id_mes;
static ID id_amount;
static ID id_total;
static ID id_file;
static ID id_fdt;

static void
db_ref(rpm_db_t* db){
	db->ref_count++;
}

static void
db_unref(rpm_db_t* db){
	db->ref_count--;
	if (!db->ref_count){
		rpmdbClose(db->db);
		free(db);
	}
}

static void
db_free(rpm_db_t* db)
{
	if (db)
		db_unref(db);
}

/*
 * The package database is opened, but transactional processing
 * (@see RPM::DB#transaction) cannot be done for when +writable+ is false.
 * When +writable+ is +false+ then the generated object gets freezed.
 * @param [Boolean] writable Whether the database is writable. Default is +false+.
 * @param [String] root Root path for the database, default is empty.
 * @return [RPM::DB]
 *
 * @example
 *   db = RPM::DB.open
 *   db.each do |pkg|
 *     puts pkg.name
 *   end
 */
static VALUE
db_s_open(int argc, VALUE* argv, VALUE obj)
{
	VALUE db;
	rpm_db_t* rdb;
	int writable = 0;
	const char* root = "";

	switch (argc) {
	case 0:
		break;

	case 1:
		writable = RTEST(argv[0]);
		break;

	case 2:
		if (!NIL_P(argv[1])) {
			if (TYPE(argv[1]) != T_STRING) {
				rb_raise(rb_eTypeError, "illegal argument type");
			}
			root = RSTRING_PTR(argv[1]);
		}
		writable = RTEST(argv[0]);
		break;

	default:
		rb_raise(rb_eArgError, "too many argument(0..2)");
	}


	rdb = ALLOC_N(rpm_db_t,1);
	if (rpmdbOpen(root, &(rdb->db), writable ? O_RDWR | O_CREAT : O_RDONLY, 0644)) {
		free(rdb);
		rb_raise(rb_eRuntimeError, "can not open database in %s",
				 RSTRING_PTR(rb_str_concat(rb_str_new2(root),
			         rb_str_new2("/var/lib/rpm"))));
	}

	rdb->ref_count = 0;
	db_ref(rdb);
	db = Data_Wrap_Struct(rpm_cDB, NULL, db_free, rdb);
	if (!writable) {
		rb_obj_freeze(db);
	}
	return db;
}

VALUE
rpm_db_open(int writable, const char* root)
{
	VALUE argv[2];
	argv[0] = writable ? Qtrue : Qfalse;
	argv[1] = root ? rb_str_new2(root) : Qnil;
	return db_s_open(2, argv, rpm_cDB);
}

/*
 * Initialize the package database
 * The database {RPM::DB#root} / var / lib /rpm is created.
 *
 * @param [String] root Root of the database
 * @param [Boolean] writable Whether the database is writable. Default +false+.
 */
static VALUE
db_s_init(int argc, VALUE* argv, VALUE obj)
{
	int writable = 0;
	const char* root;

	switch (argc) {
	case 0:
		rb_raise(rb_eArgError, "too few argument(1..2)");

	case 1: case 2:
		if (TYPE(argv[0]) != T_STRING) {
			rb_raise(rb_eTypeError, "illegal argument type");
		}
		root = RSTRING_PTR(argv[0]);
		if (argc == 2) {
			writable = RTEST(argv[1]);
		}
		break;

	default:
		rb_raise(rb_eArgError, "too many argument(1..2)");
	}

	if (rpmdbInit(root, writable ? O_RDWR | O_CREAT : O_RDONLY)) {
		rb_raise(rb_eRuntimeError, "can not initialize database in %s",
				 RSTRING_PTR(rb_str_concat(rb_str_new2(root),
									   rb_str_new2("/var/lib/rpm"))));
	}

	return Qnil;
}

/*
 * See RPM::DB#init
 */
void
rpm_db_init(const char* root, int writable)
{
	VALUE argv[2];
	argv[0] = rb_str_new2(root);
	argv[1] = writable ? Qtrue : Qfalse;
	db_s_init(2, argv, rpm_cDB);
}

/*
 * Rebuild the package database
 * It should reside in +root+ / var / lib /rpm
 * @param [String] root Root path of the database
 */
static VALUE
db_s_rebuild(int argc, VALUE* argv, VALUE obj)
{
	const char* root = "";
	int ret;

	switch (argc) {
	case 0:
		break;

	case 1:
		if (!NIL_P(argv[0])) {
			if (TYPE(argv[0]) != T_STRING) {
				rb_raise(rb_eTypeError, "illegal argument type");
			}
			root = RSTRING_PTR(argv[0]);
		}
		break;

	default:
		rb_raise(rb_eArgError, "too many arguments(0..1)");
		break;
	}

#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	ret = rpmdbRebuild(root);
#elif RPM_VERSION_CODE < RPM_VERSION(5,0,0)
	ret = rpmdbRebuild(root, NULL, NULL);
#else
	ret = rpmdbRebuild(root, NULL);
#endif
	if (ret) {
		rb_raise(rb_eRuntimeError, "can not rebuild database in %s",
				 RSTRING_PTR(rb_str_concat(rb_str_new2(root),
									   rb_str_new2("/var/lib/rpm"))));
	}

	return Qnil;
}

void
rpm_db_rebuild(const char* root)
{
	VALUE argv[1];
	argv[0] = root ? rb_str_new2(root) : Qnil;
	db_s_rebuild(1, argv, rpm_cDB);
}

/*
 * Closes the database
 */
VALUE
rpm_db_close(VALUE db)
{
	db_unref((rpm_db_t*)DATA_PTR(db));
	DATA_PTR(db) = NULL;
    rb_gc();
    return Qnil;
}

/*
 * @return [Boolean] +true+ if the database is closed
 */
VALUE
rpm_db_is_closed(VALUE vdb)
{
	return DATA_PTR(vdb) ? Qfalse : Qtrue;
}

static void
check_closed(VALUE db)
{
	if (!DATA_PTR(db)) {
		rb_raise(rb_eRuntimeError, "RPM::DB closed");
	}
}

#ifndef RPMDB_OPAQUE
/*
 * @return [String] The root path of the database
 */
VALUE
rpm_db_get_root(VALUE db)
{
	check_closed(db);
	return rb_str_new2(RPM_DB(db)->db_root);
}

/*
 * @return [String] The home path of the database
 */
VALUE
rpm_db_get_home(VALUE db)
{
	check_closed(db);
	return rb_str_new2(RPM_DB(db)->db_home);
}
#endif

/*
 * @return [Boolean] +true+ if the database is writable
 */
VALUE
rpm_db_is_writable(VALUE db)
{
	check_closed(db);
	return OBJ_FROZEN(db) ? Qfalse : Qtrue;
}

/*
 * @yield [Package] Called for each match
 * @param [Number] key RPM tag key
 * @param [String] val Value to match
 * @example
 *   db.each_match(RPM::TAG_ARCH, "x86_64") do |pkg|
 *     puts pkg.name
 *   end
 */
VALUE
rpm_db_each_match(VALUE db, VALUE key, VALUE val)
{
	VALUE mi;

	check_closed(db);

	mi = rpm_db_init_iterator (db, key, val);

	if (!NIL_P(mi))
		return rpm_mi_each (mi);
        return Qnil;
}

/*
 * @yield [Package] Called for each package in the database
 * @example
 *   db.each do |pkg|
 *     puts pkg.name
 *   end
 */
VALUE
rpm_db_each(VALUE db)
{
	check_closed(db);
	return rpm_db_each_match(db,INT2NUM(RPMDBI_PACKAGES),Qnil);
}

static void
transaction_free(rpm_trans_t* trans)
{
	if (trans->script_fd)
		Fclose(trans->script_fd);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmtransFree(trans->ts);
#else
	rpmtsFree(trans->ts);
#endif
	db_unref(trans->db);
	free(trans);
}

static VALUE
transaction_yield(VALUE tag, VALUE ts)
{
	return rb_yield(ts);
}

static VALUE
transaction_commit(VALUE tag, VALUE ts)
{
	rpm_transaction_commit(0, NULL, ts);

	/* not reached because rpm_transaction_commit() always call rb_throw() */
	return Qnil;
}

VALUE
rpm_db_transaction(int argc, VALUE* argv, VALUE db)
{
	VALUE trans;
	rpm_trans_t* ts;
	const char* root = "/";

#if 0
	if (OBJ_FROZEN(db)) {
		rb_error_frozen("RPM::DB");
	}
#endif
	switch (argc) {
	case 0:
		break;

	case 1:
		if (TYPE(argv[0]) != T_STRING) {
			rb_raise(rb_eTypeError, "illegal argument type");
		}
		root = RSTRING_PTR(argv[0]);
		break;

	default:
		rb_raise(rb_eArgError, "argument too many(0..1)");
	}

	ts = ALLOC(rpm_trans_t);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	ts->ts = rpmtransCreateSet(RPM_DB(db), root);
#else
	ts->ts = rpmtsCreate();
	rpmtsSetRootDir(ts->ts, root);
#endif
	ts->script_fd = 0;
	ts->db = DATA_PTR(db);
	trans = Data_Wrap_Struct(rpm_cTransaction, NULL, transaction_free, ts);
	db_ref(ts->db);
	rb_ivar_set(trans, id_db, db);

	rb_catch("abort", transaction_yield, trans);

	if (rb_ivar_get(trans, id_aborted) == Qtrue) {
		return Qfalse;
	} else if (rb_ivar_get(trans, id_commited) != Qtrue && !OBJ_FROZEN(db)) {
		rb_catch("abort", transaction_commit, trans);
	}

	return rb_ivar_get(trans, id_pl);
}

/*
 * @return [RPM::DB] The database associated with this transaction
 */
VALUE
rpm_transaction_get_db(VALUE trans)
{
	return rb_ivar_get(trans, id_db);
}

/*
 * @return [File] Get transaction script file handle
 *   i.e stdout/stderr on scriptlet execution
 */
VALUE
rpm_transaction_get_script_file(VALUE trans)
{
	return rb_ivar_get(trans, id_sf);
}

/*
 * Set the transaction script file handle
 *   i.e stdout/stderr on scriptlet execution
 * @param [File] file File handle
 */
VALUE
rpm_transaction_set_script_file(VALUE trans, VALUE file)
{
	if (TYPE(file) != T_FILE) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}
	rb_ivar_set(trans, id_sf, file);
	RPM_SCRIPT_FD(trans) = fdDup(NUM2INT(rb_Integer(file)));
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmtransSetScriptFd(RPM_TRANSACTION(trans), RPM_SCRIPT_FD(trans));
#else
	rpmtsSetScriptFd(RPM_TRANSACTION(trans), RPM_SCRIPT_FD(trans));
#endif
	return Qnil;
}

/*
 * Add a install operation to the transaction
 * @param [Package] pkg Package to install
 */
VALUE
rpm_transaction_install(VALUE trans, VALUE pkg, VALUE key)
{
	VALUE keys;

	if (rb_obj_is_kind_of(pkg, rpm_cPackage) == Qfalse ||
            TYPE(key) != T_STRING) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	keys = rb_ivar_get(trans, id_keys);

	if (NIL_P(keys)) {
		keys = rb_ary_new();
		rb_ivar_set(trans, id_keys, keys);
	}
	if (rb_ary_includes(keys, key) == Qtrue) {
		rb_raise(rb_eArgError, "key must be unique");
	}
	rb_ary_push(keys, key);

#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmtransAddPackage(RPM_TRANSACTION(trans), RPM_HEADER(pkg), NULL,
					   RSTRING_PTR(key), 0, NULL);
#else
	rpmtsAddInstallElement(RPM_TRANSACTION(trans), RPM_HEADER(pkg),
					   RSTRING_PTR(key), 0, NULL);
#endif

	return Qnil;
}

/*
 * Add a upgrade operation to the transaction
 * @param [Package] pkg Package to upgrade
 */
VALUE
rpm_transaction_upgrade(VALUE trans, VALUE pkg, VALUE key)
{
	VALUE keys;
	if (rb_obj_is_kind_of(pkg, rpm_cPackage) == Qfalse ||
            TYPE(key) != T_STRING) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	keys = rb_ivar_get(trans, id_keys);

	if (NIL_P(keys)) {
		keys = rb_ary_new();
		rb_ivar_set(trans, id_keys, keys);
	}
	if (rb_ary_includes(keys, key) == Qtrue) {
		rb_raise(rb_eArgError, "key must be unique");
	}
	rb_ary_push(keys, key);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmtransAddPackage(RPM_TRANSACTION(trans), RPM_HEADER(pkg), NULL,
					   RSTRING_PTR(key), 1, NULL);
#else
	rpmtsAddInstallElement(RPM_TRANSACTION(trans), RPM_HEADER(pkg),
					   RSTRING_PTR(key), 1, NULL);
#endif

	return Qnil;
}

#ifdef RPMTS_AVAILABLE
VALUE
rpm_transaction_available(VALUE trans, VALUE pkg, VALUE key)
{
	VALUE keys;

	if (rb_obj_is_kind_of(pkg, rpm_cPackage) == Qfalse ||
            TYPE(key) != T_STRING) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	keys = rb_ivar_get(trans, id_keys);
	if (NIL_P(keys)) {
		keys = rb_ary_new();
		rb_ivar_set(trans, id_keys, keys);
	}
	if (rb_ary_includes(keys, key) == Qtrue) {
		rb_raise(rb_eArgError, "key must be unique");
	}
	rb_ary_push(keys, key);

#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmtransAvailablePackage(RPM_TRANSACTION(trans), RPM_HEADER(pkg),
					   RSTRING_PTR(key));
#else
    rb_raise(rb_eNotImpError, "need rpmtsAvailablePackage");
    // FIXME: What is the analog for rpmtsAvailablePackage
    // in newer RPM's ?
    //rpmtsAvailablePackage(RPM_TRANSACTION(trans), RPM_HEADER(pkg),
    //                      RSTRING(key)->ptr);
#endif

	return Qnil;
}
#endif /* RPMTS_AVAILABLE */

/*
 * Add a delete operation to the transaction
 * @param [String, Package, Dependency] pkg Package to delete
 */
VALUE
rpm_transaction_delete(VALUE trans, VALUE pkg)
{
	VALUE db;
	VALUE mi;

	db = rb_ivar_get(trans, id_db);

	if (TYPE(pkg) == T_STRING)
		mi = rpm_db_init_iterator(db, INT2NUM(RPMDBI_LABEL), pkg);
	else if (rb_obj_is_kind_of(pkg, rpm_cPackage) != Qfalse) {
		VALUE sigmd5 = rpm_package_aref(pkg,INT2NUM(RPMTAG_SIGMD5));
		if (sigmd5 != Qnil){
			mi = rpm_db_init_iterator(db, INT2NUM(RPMTAG_SIGMD5), sigmd5);
		}else{
			VALUE name = rpm_package_aref(pkg,INT2NUM(RPMDBI_LABEL));
			mi = rpm_db_init_iterator(db, INT2NUM(RPMDBI_LABEL), name);
		}
	} else if ( rb_obj_is_kind_of(pkg, rpm_cDependency) ==Qfalse &&
                    rb_respond_to(pkg,rb_intern("name")) && rb_respond_to(pkg,rb_intern("version"))){
		mi = rpm_db_init_iterator(db, INT2NUM(RPMDBI_LABEL),rb_funcall(pkg,rb_intern("name"),0));
		rpm_mi_set_iterator_version(mi,rb_funcall(pkg,rb_intern("version"),0));
	} else
		rb_raise(rb_eTypeError, "illegal argument type");

	VALUE p;
	while (!NIL_P(p = rpm_mi_next_iterator(mi))) {
		VALUE off = rpm_mi_get_iterator_offset(mi);
		if (!NIL_P(off)){
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
			rpmtransRemovePackage(RPM_TRANSACTION(trans), NUM2INT(off));
#else
			rpmtsAddEraseElement(RPM_TRANSACTION(trans), RPM_HEADER(p), NUM2INT(off));
#endif
		}
	}

	return Qnil;
}

#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE

#if 0 /* XXX this shouldn't be needed at all */
/* from rpm-4.2.1/lib/rpmps.c */
static int
sameProblem(const rpmProblem p1, const rpmProblem p2)
{
    if (p1->type != p2->type)
		return 1;
    if (p1->pkgNEVR)
		if (p2->pkgNEVR && strcmp(p1->pkgNEVR, p2->pkgNEVR))
			return 1;
    if (p1->altNEVR)
		if (p2->altNEVR && strcmp(p1->altNEVR, p2->altNEVR))
			return 1;
    if (p1->str1)
		if (p2->str1 && strcmp(p1->str1, p2->str1))
			return 1;
    if (p1->ulong1 != p2->ulong1)
		return 1;

    return 0;
}
#endif

static VALUE
version_new_from_EVR(const char* evr)
{
	char *e = NULL;
	char *vr = NULL;
	char *end = NULL;
	char *tmp_evr = NULL;
	VALUE version = Qnil;

	tmp_evr = strdup(evr);
	if (tmp_evr==NULL) { return Qnil; }

	e = tmp_evr;
	if ( (end=strchr(e, ':')) ) {
		/* epoch is found */
		*end = '\0';
		vr = end+1;
	} else {
		/* no epoch */
		vr = e;
		e = NULL;
	}

	if ( e ) {
		version = rpm_version_new2(vr, atoi(e));
	} else {
		version = rpm_version_new(vr);
	}

	free(tmp_evr);
	return version;
}

static VALUE
package_new_from_NEVR(const char* nevr)
{
	char *name = NULL;
	char *evr = NULL;
    char *end = NULL;
	char *tmp_nevr = NULL;
	VALUE package = Qnil;
	int i=0;

	tmp_nevr = strdup(nevr);
	if (tmp_nevr==NULL) { return Qnil; }

	name = tmp_nevr;
	for ( end=name; *end != '\0'; end++ ) {
	}
	i=0;
	while ( i<2 ) {
		if ( end <= name ) { break; }
		end--;
		if ( *end == '-' ) { i++; }
	}
	if ( i==2 ) {
		*end = '\0'; evr = end + 1;
	} else {
		evr = NULL;
	}

	package = rpm_package_new_from_N_EVR(rb_str_new2(name),
                                         version_new_from_EVR(evr ? evr : ""));
	free(tmp_nevr);
	return package;
}
#endif

/*
 * Check the dependencies.
 * @return [Array<Dependency>, +nil+] If dependencies are not met returns an
 *    array with dependencies. Otherwise +nil+.
 */
VALUE
rpm_transaction_check(VALUE trans)
{
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmDependencyConflict conflicts;
	int num;

	rpmdepCheck(RPM_TRANSACTION(trans), &conflicts, &num);
	if (num) {
		VALUE list = rb_ary_new();
		register int i;

		for (i = 0; i < num; i++) {
			VALUE dep;
			switch (conflicts[i].sense) {
			case RPMDEP_SENSE_REQUIRES:
				dep = rpm_require_new(conflicts[i].needsName,
									  rpm_version_new(conflicts[i].needsVersion),
									  conflicts[i].needsFlags,
									  rpm_package_new_from_header(conflicts[i].byHeader));
				break;

			case RPMDEP_SENSE_CONFLICTS:
				dep = rpm_conflict_new(conflicts[i].needsName,
									   rpm_version_new(conflicts[i].needsVersion),
									   conflicts[i].needsFlags,
									   rpm_package_new_from_header(conflicts[i].byHeader));
				break;
			}
			rb_ary_push(list, dep);
		}

		rpmdepFreeConflicts(conflicts, num);
		return list;
	}

	return Qnil;
#else
	int rc;
	rpmps ps;
	int num;
	VALUE list = Qnil;

	rc = rpmtsCheck(RPM_TRANSACTION(trans));
	ps = rpmtsProblems(RPM_TRANSACTION(trans));
	/* get rid of duplicate problems */
	rpmpsTrim(ps, RPMPROB_FILTER_NONE);
	num = rpmpsNumProblems(ps);

#ifdef RPMPS_OPAQUE
	rpmpsi psi = rpmpsInitIterator(ps);
	if (num > 0) {
		list = rb_ary_new();
	}
	while (rpmpsNextIterator(psi) >= 0) {
		rpmProblem p = rpmpsGetProblem(psi);
		VALUE dep;
		switch (rpmProblemGetType(p)) {
		case RPMPROB_REQUIRES: {
			char *buf = strdup (rpmProblemGetAltNEVR(p));
			/* TODO: zaki: NULL check*/
			char *end;

			char *name = buf+2;
			char *relation = NULL;
			char *evr = NULL;
			rpmsenseFlags sense_flags = 0;

			end = strchr ( name, ' ');
			if ( end ) {
				*end = '\0';
				relation = end + 1;
				end = strchr ( relation, ' ');
				if ( end ) {
					*end = '\0';
					evr = end + 1;
				}
				for ( ; (*relation) != '\0'; relation++ ) {
					if ( (*relation) == '=' ) {
						sense_flags |= RPMSENSE_EQUAL;
					} else if ( (*relation) == '>' ) {
						sense_flags |= RPMSENSE_GREATER;
					} else if ( (*relation) == '<' ) {
						sense_flags |= RPMSENSE_LESS;
					}
				}
			}

			dep = rpm_require_new(name,
                                  rpm_version_new(evr ? evr : ""),
					  sense_flags,
					  package_new_from_NEVR(
						rpmProblemGetPkgNEVR(p)
					  ));
			free ( buf );
			rb_ary_push(list, dep);
			break;
		}
		default:
			break;
		}
	}
#else
	if (ps != NULL && 0 < num) {
		rpmProblem p;
		int i;
		list = rb_ary_new();

		for (i = 0; i < num; i++) {
			const char *altNEVR;
			VALUE dep;

			p = ps->probs + i;
			altNEVR = (p->altNEVR ? p->altNEVR : "? ?altNEVR?");

			if (p->ignoreProblem)
				continue;

#if 0 /* XXX shouldn't be needed at all due to rpmpsTrim() */
			/* Filter already appended problems. */
			for (j = 0; j < i; j++) {
				if (!sameProblem(p, ps->probs + j))
					break;
			}
			if (j < i)
				continue;
#endif

			if ( p->type == RPMPROB_REQUIRES ) {
				char *buf = strdup ( altNEVR );
				/* TODO: zaki: NULL check*/
				char *end;

				char *name = buf+2;
				char *relation = NULL;
				char *evr = "";
				rpmsenseFlags sense_flags = 0;

				end = strchr ( name, ' ');
				if ( end ) {
					*end = '\0';
					relation = end + 1;
					end = strchr ( relation, ' ');
					if ( end ) {
						*end = '\0';
						evr = end + 1;
					}
					for ( ; (*relation) != '\0'; relation++ ) {
						if ( (*relation) == '=' ) {
							sense_flags |= RPMSENSE_EQUAL;
						} else if ( (*relation) == '>' ) {
							sense_flags |= RPMSENSE_GREATER;
						} else if ( (*relation) == '<' ) {
							sense_flags |= RPMSENSE_LESS;
						}
					}
				}

				dep = rpm_require_new(name,
									  rpm_version_new(evr),
									  sense_flags,
									  package_new_from_NEVR(p->pkgNEVR)
									  );
				free ( buf );
				rb_ary_push(list, dep);
			} else {
#if 0
			RPMPROB_CONFLICT:
			RPMPROB_BADARCH:
			RPMPROB_BADOS:
			RPMPROB_PKG_INSTALLED:
			RPMPROB_BADRELOCATE:
			RPMPROB_NEW_FILE_CONFLICT:
			RPMPROB_FILE_CONFLICT:
			RPMPROB_OLDPACKAGE:
			RPMPROB_DISKSPACE:
			RPMPROB_DISKNODES:
			RPMPROB_BADPRETRANS:
#endif
				break;
			}

#if 0
			printf ("%d, type=%d, ignoreProblem=%d, str1=%s pkgNEVR=%s, %s\n",
					i, p->type, p->ignoreProblem, p->str1, p->pkgNEVR, altNEVR);
#endif
        }
	}
#endif /* RPMPS_OPAQUE */
	ps = rpmpsFree(ps);

	return list;
#endif
}

/*
 * To determine the processing order.
 */
VALUE
rpm_transaction_order(VALUE trans)
{
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmdepOrder(RPM_TRANSACTION(trans));
#else
	rpmtsOrder(RPM_TRANSACTION(trans));
#endif
	return Qnil;
}

/*
 * @return [Array<String>] an array of keys corresponding to all transactions
 *   that have been added.
 */
VALUE
rpm_transaction_keys(VALUE trans)
{
	return rb_ivar_get(trans, id_keys);
}

#if RPM_VERSION_CODE < RPM_VERSION(4,4,5)
typedef unsigned long rpmCallbackSize_t;
#elif RPM_VERSION(5,0,0) <= RPM_VERSION_CODE
typedef uint64_t rpmCallbackSize_t;
#else
typedef unsigned long long rpmCallbackSize_t;
#endif
static void*
transaction_callback(const void* hd, const rpmCallbackType type,
					 const rpmCallbackSize_t amount, const rpmCallbackSize_t total,
					 fnpyKey key, rpmCallbackData data)
{
	VALUE trans = (VALUE)data;
	FD_t fdt;
	const Header hdr = (Header)hd;
	VALUE sig;
	VALUE rv;

	sig = rb_struct_new(rpm_sCallbackData, INT2NUM(type), key ? (VALUE)key:Qnil,
						rpm_package_new_from_header(hdr),
						UINT2NUM(amount), UINT2NUM(total));

	rv = rb_yield(sig);

	switch (type) {
	case RPMCALLBACK_INST_OPEN_FILE:
		if (TYPE(rv) != T_FILE) {
			rb_raise(rb_eTypeError, "illegal return value type");
		}
		rb_ivar_set(trans, id_file, rv);
		fdt = fdDup(NUM2INT(rb_Integer(rv)));
		rb_ivar_set(trans, id_fdt, INT2NUM((long)fdt));
		return fdt;

	case RPMCALLBACK_INST_CLOSE_FILE:
		Fclose((FD_t)NUM2LONG(rb_ivar_get(trans, id_fdt)));
		rb_ivar_set(trans, id_file, Qnil);
		rb_ivar_set(trans, id_fdt, Qnil);
	default:
		break;
	}

	return NULL;
}

/*
 * Performs the transaction.
 * @param [Number] flag Transaction flags, default +RPM::TRANS_FLAG_NONE+
 * @param [Number] filter Transaction filter, default +RPM::PROB_FILTER_NONE+
 * @example
 *   transaction.commit do |sig|
 *   end
 * @yield [CallbackData] sig Transaction progress
 */
VALUE
rpm_transaction_commit(int argc, VALUE* argv, VALUE trans)
{
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmProblemSet probset;
	int flags = RPMTRANS_FLAG_NONE;
	int ignores = RPMPROB_FILTER_NONE;
	int rc;
	VALUE db;

	db = rb_ivar_get(trans, id_db);

	if (OBJ_FROZEN(db)) {
		rb_error_frozen("RPM::DB");
	}

	switch (argc) {
	case 0:
		break;

	case 1: case 2:
		flags = NUM2INT(rb_Integer(argv[0]));
		if (argc == 2) {
			ignores = NUM2INT(rb_Integer(argv[1]));
		}
		break;

	default:
		rb_raise(rb_eArgError, "too many arguments(0..2)");
	}
	if (rb_block_given_p() == Qtrue)
		rc = rpmRunTransactions(RPM_TRANSACTION(trans), transaction_callback,
								(void*)trans, NULL, &probset, flags, ignores);
	else{
		VALUE keys;

		/* rpmcli.h:extern int packagesTotal; */
		packagesTotal = 0;

		keys = rpm_transaction_keys(trans);

		if (!NIL_P(keys))
			packagesTotal = NUM2INT(rb_funcall(keys,rb_intern("length"),0));
		rc = rpmRunTransactions(RPM_TRANSACTION(trans), rpmShowProgress,
							(void*)((long)(INSTALL_HASH|INSTALL_LABEL)),
							NULL, &probset, flags, ignores);
	}

	if (probset != NULL) {
		VALUE list = rb_ary_new();
		register int i;

		for (i = 0; i < probset->numProblems; i++) {
			rpmProblem prob = probset->probs + i;
			VALUE prb = rb_struct_new(rpm_sProblem,
									  INT2NUM(prob->type),
									  (VALUE)prob->key,
									  rpm_package_new_from_header(prob->h),
									  rb_str_new2(rpmProblemString(prob)));
			rb_ary_push(list, prb);
		}

		rb_ivar_set(trans, id_pl, list);
	}

#else
	rpmps ps;
	int flags = RPMTRANS_FLAG_NONE;
	int ignores = RPMPROB_FILTER_NONE;
	int rc;
	VALUE db;

	db = rb_ivar_get(trans, id_db);

	if (OBJ_FROZEN(db)) {
		rb_error_frozen("RPM::DB");
	}

	switch (argc) {
	case 0:
		break;

	case 1: case 2:
		flags = NUM2INT(rb_Integer(argv[0]));
		if (argc == 2) {
			ignores = NUM2INT(rb_Integer(argv[1]));
		}
		break;

	default:
		rb_raise(rb_eArgError, "too many arguments(0..2)");
	}


	/* Drop added/available package indices and dependency sets. */
	//rpmtsClean(RPM_TRANSACTION(trans)); // zaki: required?

	if (rb_block_given_p() == Qtrue) {
		rpmtsSetNotifyCallback(RPM_TRANSACTION(trans),
							   (rpmCallbackFunction)transaction_callback,(void *)trans);
	}else{
		VALUE keys;

		/* rpmcli.h:extern int rpmcliPackagesTotal; */
		rpmcliPackagesTotal = 0;

		keys = rpm_transaction_keys(trans);

		if (!NIL_P(keys))
			rpmcliPackagesTotal = NUM2INT(rb_funcall(keys,rb_intern("length"),0));

		rpmtsSetNotifyCallback(RPM_TRANSACTION(trans), rpmShowProgress,
							   (void*)((long)(INSTALL_HASH|INSTALL_LABEL)));
	}
	rc = rpmtsRun(RPM_TRANSACTION(trans), NULL, ignores);
	ps = rpmtsProblems(RPM_TRANSACTION(trans));

	{
	VALUE list = rb_ary_new();
#ifdef RPMPS_OPAQUE
	rpmpsi psi = rpmpsInitIterator(ps);
	while (rpmpsNextIterator(psi) >= 0) {
		rpmProblem p = rpmpsGetProblem(psi);
		VALUE prb = rb_struct_new(rpm_sProblem,
					INT2NUM(rpmProblemGetType(p)),
					(VALUE)rpmProblemGetKey(p),
					package_new_from_NEVR(
						rpmProblemGetAltNEVR(p)+2
					),
					rb_str_new2(rpmProblemString(p)));
		rb_ary_push(list, prb);
	}
#else
	if (ps != NULL && rpmpsNumProblems(ps) > 0) {
		register int i;

		for (i = 0; i < rpmpsNumProblems(ps); i++) {
			rpmProblem p = ps->probs + i;
			const char *altNEVR = (p->altNEVR ? p->altNEVR : "? ?altNEVR?");

			VALUE prb = rb_struct_new(rpm_sProblem,
									  INT2NUM(p->type),
									  (VALUE)p->key,
									  package_new_from_NEVR(altNEVR+2),
									  rb_str_new2(rpmProblemString(p)));
			rb_ary_push(list, prb);
		}
	}
#endif
	rb_ivar_set(trans, id_pl, list);
	}
	if (ps) ps = rpmpsFree(ps);

#endif

	rb_ivar_set(trans, id_commited, Qtrue);
	rb_throw("abort", Qnil);

	return Qnil; /* NOT REACHED */
}

/*
 * To abort the transaction. Database is not changed.
 */
VALUE
rpm_transaction_abort(VALUE trans)
{
	rb_ivar_set(trans, id_aborted, Qtrue);
	rb_throw("abort", Qnil);
	return Qnil; /* NOT REACHED */
}

static void
mi_free(rpm_mi_t* mi)
{
	rpmdbFreeIterator(mi->mi);
	db_unref(mi->db);
	free(mi);
}

VALUE
rpm_db_init_iterator(VALUE db, VALUE key, VALUE val)
{
	rpm_mi_t* mi;

	check_closed(db);

	if (!NIL_P(val) && TYPE(val) != T_STRING) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	mi = ALLOC_N(rpm_mi_t,1);
	if ((mi->mi = rpmdbInitIterator(RPM_DB(db), NUM2INT(rb_Integer(key)),
						   NIL_P(val) ? NULL : RSTRING_PTR(val),
                           NIL_P(val) ? 0 : RSTRING_LEN(val)))){
		mi->db = (rpm_db_t*)DATA_PTR(db);
		db_ref(mi->db);
		return Data_Wrap_Struct(rpm_cMatchIterator, NULL, mi_free, mi);
	}
	free(mi);
    /* FIXME: returning nil here is a pain; for ruby, it would be nicer
       to return an empty array */
	return Qnil;
}

VALUE
rpm_mi_next_iterator(VALUE mi)
{
	Header hdr;
	hdr = rpmdbNextIterator(RPM_MI(mi));
	if (hdr)
		return rpm_package_new_from_header(hdr);
	return Qnil;
}

VALUE
rpm_mi_get_iterator_count(VALUE mi)
{
	return INT2NUM(rpmdbGetIteratorCount(RPM_MI(mi)));
}

VALUE
rpm_mi_get_iterator_offset(VALUE mi)
{
	int off = rpmdbGetIteratorOffset(RPM_MI(mi));
	if (off)
		return INT2NUM(off);
	return Qnil;
}

VALUE
rpm_mi_set_iterator_re(VALUE mi,VALUE tag, VALUE mode, VALUE re)
{
	if (TYPE(re) != T_STRING)
		rb_raise(rb_eTypeError, "illegal argument type");

	rpmdbSetIteratorRE(RPM_MI(mi),NUM2INT(tag),NUM2INT(mode),RSTRING_PTR(re));
	return mi;
}

VALUE
rpm_mi_set_iterator_version(VALUE mi, VALUE version)
{
/* Epoch!! */
	VALUE r;
	if (rb_obj_is_kind_of(version, rpm_cVersion) == Qfalse)
		rb_raise(rb_eTypeError, "illegal argument type");
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmdbSetIteratorVersion(RPM_MI(mi),RSTRING_PTR(rpm_version_get_v(version)));
#else
	rpmdbSetIteratorRE(RPM_MI(mi),RPMTAG_VERSION,RPMMIRE_DEFAULT,RSTRING_PTR(rpm_version_get_v(version)));
#endif
	r = rpm_version_get_r(version);
	if(!NIL_P(r)){
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
		rpmdbSetIteratorRelease(RPM_MI(mi),RSTRING_PTR(r));
#else
		rpmdbSetIteratorRE(RPM_MI(mi),RPMTAG_RELEASE,RPMMIRE_DEFAULT,RSTRING_PTR(r));
#endif
	}
	return mi;
}

VALUE
rpm_mi_each(VALUE mi)
{
	VALUE p;
	while(!NIL_P( p = rpm_mi_next_iterator(mi)))
		rb_yield (p);
        return Qnil;
}

void
Init_rpm_DB(void)
{
	rpm_cDB = rb_define_class_under(rpm_mRPM, "DB", rb_cData);
	rb_include_module(rpm_cDB, rb_mEnumerable);
	rb_define_singleton_method(rpm_cDB, "new", db_s_open, -1);
	rb_define_singleton_method(rpm_cDB, "open", db_s_open, -1);
	rb_define_singleton_method(rpm_cDB, "init", db_s_init, -1);
	rb_define_singleton_method(rpm_cDB, "rebuild", db_s_rebuild, -1);
	rb_define_method(rpm_cDB, "close", rpm_db_close, 0);
	rb_define_method(rpm_cDB, "closed?", rpm_db_is_closed, 0);
#ifndef RPMDB_OPAQUE
	rb_define_method(rpm_cDB, "root", rpm_db_get_root, 0);
	rb_define_method(rpm_cDB, "home", rpm_db_get_home, 0);
#endif
	rb_define_method(rpm_cDB, "writable?", rpm_db_is_writable, 0);
	rb_define_method(rpm_cDB, "each_match", rpm_db_each_match, 2);
	rb_define_method(rpm_cDB, "each", rpm_db_each, 0);
	rb_define_method(rpm_cDB, "transaction", rpm_db_transaction, -1);
	rb_define_method(rpm_cDB, "init_iterator", rpm_db_init_iterator, 2);
	rb_undef_method(rpm_cDB, "dup");
	rb_undef_method(rpm_cDB, "clone");
}

void
Init_rpm_MatchIterator(void)
{
	rpm_cMatchIterator = rb_define_class_under(rpm_mRPM, "MatchIterator", rb_cData);
	rb_include_module(rpm_cMatchIterator, rb_mEnumerable);
	rb_define_method(rpm_cMatchIterator, "each", rpm_mi_each, 0);
	rb_define_method(rpm_cMatchIterator, "next_iterator", rpm_mi_next_iterator, 0);
	rb_define_method(rpm_cMatchIterator, "offset", rpm_mi_get_iterator_offset, 0);
	rb_define_method(rpm_cMatchIterator, "set_iterator_re", rpm_mi_set_iterator_re, 3);
	rb_define_method(rpm_cMatchIterator, "regexp", rpm_mi_set_iterator_re, 3);
	rb_define_method(rpm_cMatchIterator, "set_iterator_version", rpm_mi_set_iterator_version, 1);
	rb_define_method(rpm_cMatchIterator, "version", rpm_mi_set_iterator_version, 1);
	rb_define_method(rpm_cMatchIterator, "get_iterator_count",rpm_mi_get_iterator_count, 0);
	rb_define_method(rpm_cMatchIterator, "count",rpm_mi_get_iterator_count, 0);
	rb_define_method(rpm_cMatchIterator, "length",rpm_mi_get_iterator_count, 0);
}

void
Init_rpm_transaction(void)
{
	rpm_cTransaction = rb_define_class_under(rpm_mRPM, "Transaction", rb_cData);
	rb_define_method(rpm_cTransaction, "db", rpm_transaction_get_db, 0);
	rb_define_method(rpm_cTransaction, "script_file", rpm_transaction_get_script_file, 0);
	rb_define_method(rpm_cTransaction, "script_file=", rpm_transaction_set_script_file, 1);
	rb_define_method(rpm_cTransaction, "install", rpm_transaction_install, 2);
	rb_define_method(rpm_cTransaction, "upgrade", rpm_transaction_upgrade, 2);
#ifdef RPMTS_AVAILABLE
	rb_define_method(rpm_cTransaction, "available", rpm_transaction_available, 2);
#endif
	rb_define_method(rpm_cTransaction, "delete", rpm_transaction_delete, 1);
	rb_define_method(rpm_cTransaction, "check", rpm_transaction_check, 0);
	rb_define_method(rpm_cTransaction, "order", rpm_transaction_order, 0);
	rb_define_method(rpm_cTransaction, "keys", rpm_transaction_keys, 0);
	rb_define_method(rpm_cTransaction, "commit", rpm_transaction_commit, -1);
	rb_define_method(rpm_cTransaction, "abort", rpm_transaction_abort, 0);
	rb_undef_method(rpm_cTransaction, "dup");
	rb_undef_method(rpm_cTransaction, "clone");

    /*
     * @attr [Number] type Type of event
     * @attr [Number] key Key transactions
     * @attr [Package] package Package being processed
     * @attr [Number] amount Progress
     * @attr [Number] total Total size
     */
	rpm_sCallbackData = rb_struct_define(NULL, "type", "key", "package",
										 "amount", "total", NULL);

	rb_define_const(rpm_mRPM, "CallbackData", rpm_sCallbackData);

	rpm_sProblem = rb_struct_define(NULL, "type", "key", "package",
									"description", NULL);
	rb_define_const(rpm_mRPM, "Problem", rpm_sProblem);

	id_db = rb_intern("db");
	id_sf = rb_intern("script_file");
	id_keys = rb_intern("keys");
	id_commited = rb_intern("commited");
	id_aborted = rb_intern("aborted");
	id_pl = rb_intern("problist");
	id_type = rb_intern("type");
	id_key = rb_intern("key");
	id_pkg = rb_intern("package");
	id_mes = rb_intern("message");
	id_amount = rb_intern("amount");
	id_total = rb_intern("total");
	//id_pkg_cache = rb_intern("package_cache");
	id_file = rb_intern("file");
	id_fdt = rb_intern("fdt");
}
