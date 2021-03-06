/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: rpm.c 9 2004-03-13 14:19:20Z zaki $ */

#include "private.h"

VALUE rpm_mRPM;

static VALUE
m_expand(VALUE m, VALUE name)
{
	return rb_str_new2(rpmExpand(StringValueCStr(name), NULL));
}

/*
 * @param [String] name Name of the macro
 * @return [String] value of macro +name+
 */
static VALUE
m_aref(VALUE m, VALUE name)
{
	char  buf[BUFSIZ];
	char* tmp;
	VALUE val;

	if (TYPE(name) != T_STRING) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	sprintf(buf, "%%{%s}", RSTRING_PTR(name));
	tmp = strdup(buf);
	expandMacros(NULL, NULL, buf, BUFSIZ);
	if (strcmp(tmp, buf) == 0) {
		val = Qnil;
	} else {
		val = rb_str_new2(buf);
	}
	free(tmp);

	return val;
}

VALUE
rpm_macro_aref(VALUE name)
{
	return m_aref(Qnil, name);
}

/*
 * Setup a macro
 * @param [String] name Name of the macro
 * @param [String] value Value of the macro or +nil+ to delete it
 */
static VALUE
m_aset(VALUE m, VALUE name, VALUE val)
{
	if (TYPE(name) != T_STRING
		|| (val != Qnil && TYPE(val) != T_STRING)) {
		rb_raise(rb_eTypeError, "illegal argument type(s)");
	}
	if (val == Qnil) {
		delMacro(NULL, RSTRING_PTR(name));
	} else {
		addMacro(NULL, RSTRING_PTR(name), NULL, RSTRING_PTR(val), RMIL_DEFAULT);
	}
	return Qnil;
}

VALUE
rpm_macro_aset(VALUE name, VALUE val)
{
	return m_aset(Qnil, name, val);
}

/*
 * Read configuration files
 */
static VALUE
m_readrc(int argc, VALUE* argv, VALUE m)
{
	register int i;
	char buf[BUFSIZ];

	if (argc == 0) {
		rb_raise(rb_eArgError, "too few argument(>= 1)");
	}

	buf[0] = '\0';
	for (i = 0; i < argc; i++) {
		if (TYPE(argv[i]) != T_STRING) {
			rb_raise(rb_eTypeError, "illegal argument type(s)");
		}
		strcat(buf, RSTRING_PTR(argv[i]));
		strcat(buf, ":");
	}
	rpmFreeMacros(NULL);
	if (rpmReadConfigFiles(buf, NULL)) {
		rb_raise(rb_eRuntimeError, "can not read rc file %s", buf);
	}

	return Qnil;
}

void
rpm_readrc(const char* rcpath)
{
	VALUE argv[1];
	argv[0] = rb_str_new2(rcpath);
	m_readrc(1, argv, Qnil);
}

static VALUE
m_init_macros(int argc, VALUE* argv, VALUE m)
{
	register int i;
	char buf[BUFSIZ];

	if (argc == 0) {
		rb_raise(rb_eArgError, "too few argument(>= 1)");
	}

	buf[0] = '\0';
	for (i = 0; i < argc; i++) {
		if (TYPE(argv[i]) != T_STRING) {
			rb_raise(rb_eTypeError, "illegal argument type(s)");
		}
		strcat(buf, RSTRING_PTR(argv[i]));
		strcat(buf, ":");
	}
	rpmInitMacros(NULL, buf);

	return Qnil;
}

void
rpm_init_marcros(const char* path)
{
	VALUE argv[1];
	argv[0] = rb_str_new2(path);
	m_init_macros(1, argv, Qnil);
}

static int rpm_verbosity;

/*
 * @return [Number] Verbosity level
 */
static VALUE
m_get_verbosity(VALUE m)
{
	return INT2NUM(rpm_verbosity);
}

VALUE
rpm_get_verbosity(void)
{
	return INT2NUM(rpm_verbosity);
}

/*
 * Sets the verbosity level
 * @param [Number] verbosity Verbosity level
 */
static VALUE
m_set_verbosity(VALUE m, VALUE verbosity)
{
	int v = NUM2INT(rb_Integer(verbosity));
	switch (v) {
	case RPMLOG_EMERG: case RPMLOG_ALERT:
	case RPMLOG_CRIT: case RPMLOG_ERR:
	case RPMLOG_WARNING: case RPMLOG_NOTICE:
	case RPMLOG_INFO: case RPMLOG_DEBUG:
		break;

	default:
		rb_raise(rb_eArgError, "invalid verbosity");
	}
	rpmSetVerbosity(v);
	rpm_verbosity = v;
	return Qnil;
}

void
rpm_set_verbosity(VALUE verbosity)
{
	m_set_verbosity(Qnil, verbosity);
}

static VALUE ruby_rpm_temp_format = Qnil;

VALUE
ruby_rpm_make_temp_name(void)
{
	static long num = 0;
	VALUE argv[2] = { ruby_rpm_temp_format, Qnil };

	argv[1] = INT2NUM(num++);
	return rb_f_sprintf(2, argv);
}

void
Init_rpm(void)
{
	char *temp;
    VALUE rbtmpdir;

    rb_require("tmpdir");

    rbtmpdir = rb_funcall(rb_const_get(rb_cObject,
                                       rb_intern("Dir")),
                          rb_intern("tmpdir"), 0);
	rpm_mRPM = rb_define_module("RPM");

#define DEF_LOG(name) \
	rb_define_const(rpm_mRPM, "LOG_"#name, INT2NUM(RPMLOG_##name))
	DEF_LOG(EMERG);
	DEF_LOG(ALERT);
	DEF_LOG(CRIT);
	DEF_LOG(ERR);
	DEF_LOG(WARNING);
	DEF_LOG(NOTICE);
	DEF_LOG(INFO);
	DEF_LOG(DEBUG);
#undef DEF_LOG

#if RPM_VERSION_CODE < RPM_VERSION(4,5,90)
#define DEF_MESS(name) \
	rb_define_const(rpm_mRPM, "MESS_"#name, INT2NUM(RPMMESS_##name))
	DEF_MESS(DEBUG);
	DEF_MESS(VERBOSE);
	DEF_MESS(NORMAL);
	DEF_MESS(WARNING);
	DEF_MESS(ERROR);
	DEF_MESS(FATALERROR);
	DEF_MESS(QUIET);
#undef DEF_MESS
#endif

#define DEFINE_DBI(name) \
	rb_define_const(rpm_mRPM, "DBI_"#name, INT2NUM(RPMDBI_##name))
	DEFINE_DBI(PACKAGES);
	DEFINE_DBI(DEPENDS);
	DEFINE_DBI(LABEL);
	DEFINE_DBI(ADDED);
	DEFINE_DBI(REMOVED);
	DEFINE_DBI(AVAILABLE);
#undef DEFINE_DBI

#define DEFINE_TAG(name) \
	rb_define_const(rpm_mRPM, "TAG_"#name, INT2NUM(RPMTAG_##name))
	DEFINE_TAG(HEADERIMAGE);
	DEFINE_TAG(HEADERSIGNATURES);
	DEFINE_TAG(HEADERIMMUTABLE);
	DEFINE_TAG(HEADERREGIONS);
	DEFINE_TAG(HEADERI18NTABLE);
	DEFINE_TAG(SIGSIZE);
	DEFINE_TAG(SIGPGP);
	DEFINE_TAG(SIGMD5);
	DEFINE_TAG(SIGGPG);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEFINE_TAG(PUBKEYS);
	DEFINE_TAG(DSAHEADER);
	DEFINE_TAG(RSAHEADER);
#endif
	DEFINE_TAG(SHA1HEADER);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEFINE_TAG(HDRID);
#endif
	DEFINE_TAG(NAME);
	DEFINE_TAG(VERSION);
	DEFINE_TAG(RELEASE);
	DEFINE_TAG(EPOCH);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEFINE_TAG(N);
	DEFINE_TAG(V);
	DEFINE_TAG(R);
	DEFINE_TAG(E);
#endif
	DEFINE_TAG(SUMMARY);
	DEFINE_TAG(DESCRIPTION);
	DEFINE_TAG(BUILDTIME);
	DEFINE_TAG(BUILDHOST);
	DEFINE_TAG(INSTALLTIME);
	DEFINE_TAG(SIZE);
	DEFINE_TAG(DISTRIBUTION);
	DEFINE_TAG(VENDOR);
	DEFINE_TAG(GIF);
	DEFINE_TAG(XPM);
	DEFINE_TAG(LICENSE);
	DEFINE_TAG(PACKAGER);
	DEFINE_TAG(GROUP);
	DEFINE_TAG(SOURCE);
	DEFINE_TAG(PATCH);
	DEFINE_TAG(URL);
	DEFINE_TAG(OS);
	DEFINE_TAG(ARCH);
	DEFINE_TAG(PREIN);
	DEFINE_TAG(POSTIN);
	DEFINE_TAG(PREUN);
	DEFINE_TAG(POSTUN);
	DEFINE_TAG(FILESIZES);
	DEFINE_TAG(FILESTATES);
	DEFINE_TAG(FILEMODES);
	DEFINE_TAG(FILERDEVS);
	DEFINE_TAG(FILEMTIMES);
	DEFINE_TAG(FILEMD5S);
	DEFINE_TAG(FILELINKTOS);
	DEFINE_TAG(FILEFLAGS);
	DEFINE_TAG(FILEUSERNAME);
	DEFINE_TAG(FILEGROUPNAME);
	DEFINE_TAG(ICON);
	DEFINE_TAG(SOURCERPM);
	DEFINE_TAG(FILEVERIFYFLAGS);
	DEFINE_TAG(ARCHIVESIZE);
	DEFINE_TAG(PROVIDENAME);
	DEFINE_TAG(REQUIREFLAGS);
	DEFINE_TAG(REQUIRENAME);
	DEFINE_TAG(REQUIREVERSION);
	DEFINE_TAG(CONFLICTFLAGS);
	DEFINE_TAG(CONFLICTNAME);
	DEFINE_TAG(CONFLICTVERSION);
	DEFINE_TAG(EXCLUDEARCH);
	DEFINE_TAG(EXCLUDEOS);
	DEFINE_TAG(EXCLUSIVEARCH);
	DEFINE_TAG(EXCLUSIVEOS);
	DEFINE_TAG(RPMVERSION);
	DEFINE_TAG(TRIGGERSCRIPTS);
	DEFINE_TAG(TRIGGERNAME);
	DEFINE_TAG(TRIGGERVERSION);
	DEFINE_TAG(TRIGGERFLAGS);
	DEFINE_TAG(TRIGGERINDEX);
	DEFINE_TAG(VERIFYSCRIPT);
	DEFINE_TAG(CHANGELOGTIME);
	DEFINE_TAG(CHANGELOGNAME);
	DEFINE_TAG(CHANGELOGTEXT);
	DEFINE_TAG(PREINPROG);
	DEFINE_TAG(POSTINPROG);
	DEFINE_TAG(PREUNPROG);
	DEFINE_TAG(POSTUNPROG);
	DEFINE_TAG(BUILDARCHS);
	DEFINE_TAG(OBSOLETENAME);
	DEFINE_TAG(VERIFYSCRIPTPROG);
	DEFINE_TAG(TRIGGERSCRIPTPROG);
	DEFINE_TAG(COOKIE);
	DEFINE_TAG(FILEDEVICES);
	DEFINE_TAG(FILEINODES);
	DEFINE_TAG(FILELANGS);
	DEFINE_TAG(PREFIXES);
	DEFINE_TAG(INSTPREFIXES);
	DEFINE_TAG(PROVIDEFLAGS);
	DEFINE_TAG(PROVIDEVERSION);
	DEFINE_TAG(OBSOLETEFLAGS);
	DEFINE_TAG(OBSOLETEVERSION);
	DEFINE_TAG(DIRINDEXES);
	DEFINE_TAG(BASENAMES);
	DEFINE_TAG(DIRNAMES);
	DEFINE_TAG(OPTFLAGS);
	DEFINE_TAG(DISTURL);
	DEFINE_TAG(PAYLOADFORMAT);
	DEFINE_TAG(PAYLOADCOMPRESSOR);
	DEFINE_TAG(PAYLOADFLAGS);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	DEFINE_TAG(MULTILIBS);
#endif
	DEFINE_TAG(INSTALLTID);
	DEFINE_TAG(REMOVETID);
	DEFINE_TAG(RHNPLATFORM);
	DEFINE_TAG(PLATFORM);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEFINE_TAG(CACHECTIME);
	DEFINE_TAG(CACHEPKGPATH);
	DEFINE_TAG(CACHEPKGSIZE);
	DEFINE_TAG(CACHEPKGMTIME);
	DEFINE_TAG(FILECOLORS);
	DEFINE_TAG(FILECLASS);
	DEFINE_TAG(CLASSDICT);
	DEFINE_TAG(FILEDEPENDSX);
	DEFINE_TAG(FILEDEPENDSN);
	DEFINE_TAG(DEPENDSDICT);
	DEFINE_TAG(SOURCEPKGID);
#endif
#undef DEFINE_TAG

#define DEFINE_FILE_STATE(name) \
	rb_define_const(rpm_mRPM, "FILE_STATE_"#name, INT2NUM(RPMFILE_STATE_##name))
	DEFINE_FILE_STATE(NORMAL);
	DEFINE_FILE_STATE(REPLACED);
	DEFINE_FILE_STATE(NOTINSTALLED);
	DEFINE_FILE_STATE(NETSHARED);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEFINE_FILE_STATE(WRONGCOLOR);
#endif
#undef DEFILE_FILE_STATE

#define DEFINE_FILE(name) \
	rb_define_const(rpm_mRPM, "FILE_"#name, INT2NUM(RPMFILE_##name))
	DEFINE_FILE(NONE);
	DEFINE_FILE(CONFIG);
	DEFINE_FILE(DOC);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	DEFINE_FILE(DONOTUSE);
#endif
	DEFINE_FILE(MISSINGOK);
	DEFINE_FILE(NOREPLACE);
	DEFINE_FILE(SPECFILE);
	DEFINE_FILE(GHOST);
	DEFINE_FILE(LICENSE);
	DEFINE_FILE(README);
	DEFINE_FILE(EXCLUDE);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	DEFINE_FILE(MULTILIB_SHIFT);
	DEFINE_FILE(MULTILIB_MASK);
#endif
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEFINE_FILE(UNPATCHED);
	DEFINE_FILE(PUBKEY);
#endif
	DEFINE_FILE(ALL);
#undef DEFINE_FILE

#define DEFINE_SENSE(name) \
	rb_define_const(rpm_mRPM, "SENSE_"#name, INT2NUM(RPMSENSE_##name))
	DEFINE_SENSE(ANY);
#if RPM_VERSION_CODE < RPM_VERSION(4,5,90)
	DEFINE_SENSE(SERIAL);
#endif
	DEFINE_SENSE(LESS);
	DEFINE_SENSE(GREATER);
	DEFINE_SENSE(EQUAL);
	DEFINE_SENSE(PROVIDES);
	DEFINE_SENSE(CONFLICTS);
	DEFINE_SENSE(PREREQ);
	DEFINE_SENSE(OBSOLETES);
	DEFINE_SENSE(INTERP);
	DEFINE_SENSE(SCRIPT_PRE);
	DEFINE_SENSE(SCRIPT_POST);
	DEFINE_SENSE(SCRIPT_PREUN);
	DEFINE_SENSE(SCRIPT_POSTUN);
	DEFINE_SENSE(SCRIPT_VERIFY);
	DEFINE_SENSE(FIND_REQUIRES);
	DEFINE_SENSE(FIND_PROVIDES);
	DEFINE_SENSE(TRIGGERIN);
	DEFINE_SENSE(TRIGGERUN);
	DEFINE_SENSE(TRIGGERPOSTUN);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	DEFINE_SENSE(MULTILIB);
#endif
	DEFINE_SENSE(SCRIPT_PREP);
	DEFINE_SENSE(SCRIPT_BUILD);
	DEFINE_SENSE(SCRIPT_INSTALL);
	DEFINE_SENSE(SCRIPT_CLEAN);
	DEFINE_SENSE(RPMLIB);
	DEFINE_SENSE(TRIGGERPREIN);
	DEFINE_SENSE(KEYRING);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	DEFINE_SENSE(TRIGGER);
#endif
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
#if defined(RPMSENSE_PATCHES)
	DEFINE_SENSE(PATCHES);
#endif
	DEFINE_SENSE(CONFIG);
#endif
#undef DEFINE_SENSE

#define DEF_PROB(name) \
	rb_define_const(rpm_mRPM, "PROB_"#name, INT2NUM(RPMPROB_##name))
	DEF_PROB(BADARCH);
	DEF_PROB(BADOS);
	DEF_PROB(PKG_INSTALLED);
	DEF_PROB(BADRELOCATE);
	DEF_PROB(REQUIRES);
	DEF_PROB(CONFLICT);
	DEF_PROB(NEW_FILE_CONFLICT);
	DEF_PROB(FILE_CONFLICT);
	DEF_PROB(OLDPACKAGE);
	DEF_PROB(DISKSPACE);
	DEF_PROB(DISKNODES);
#if RPM_VERSION_CODE < RPM_VERSION(4,5,90)
	DEF_PROB(BADPRETRANS);
#endif
#undef DEF_PROB

#define DEF_CALLBACK(name) \
	rb_define_const(rpm_mRPM, "CALLBACK_"#name, INT2NUM(RPMCALLBACK_##name))
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEF_CALLBACK(UNKNOWN);
#endif
	DEF_CALLBACK(INST_PROGRESS);
	DEF_CALLBACK(INST_START);
	DEF_CALLBACK(INST_OPEN_FILE);
	DEF_CALLBACK(INST_CLOSE_FILE);
	DEF_CALLBACK(TRANS_PROGRESS);
	DEF_CALLBACK(TRANS_START);
	DEF_CALLBACK(TRANS_STOP);
	DEF_CALLBACK(UNINST_PROGRESS);
	DEF_CALLBACK(UNINST_START);
	DEF_CALLBACK(UNINST_STOP);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEF_CALLBACK(REPACKAGE_PROGRESS);
	DEF_CALLBACK(REPACKAGE_START);
	DEF_CALLBACK(REPACKAGE_STOP);
	DEF_CALLBACK(UNPACK_ERROR);
	DEF_CALLBACK(CPIO_ERROR);
#endif
#undef DEF_CALLBACK

#define DEF_TRANS_FLAG(name) \
	rb_define_const(rpm_mRPM, "TRANS_FLAG_"#name, INT2NUM(RPMTRANS_FLAG_##name))
	DEF_TRANS_FLAG(NONE);
	DEF_TRANS_FLAG(TEST);
	DEF_TRANS_FLAG(BUILD_PROBS);
	DEF_TRANS_FLAG(NOSCRIPTS);
	DEF_TRANS_FLAG(JUSTDB);
	DEF_TRANS_FLAG(NOTRIGGERS);
	DEF_TRANS_FLAG(NODOCS);
	DEF_TRANS_FLAG(ALLFILES);
#if RPM_VERSION_CODE < RPM_VERSION(5,0,0)
	DEF_TRANS_FLAG(KEEPOBSOLETE);
#endif
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	DEF_TRANS_FLAG(MULTILIB);
#endif
	DEF_TRANS_FLAG(DIRSTASH);
	DEF_TRANS_FLAG(REPACKAGE);
	DEF_TRANS_FLAG(PKGCOMMIT);
	DEF_TRANS_FLAG(PKGUNDO);
	DEF_TRANS_FLAG(COMMIT);
	DEF_TRANS_FLAG(UNDO);
#if RPM_VERSION_CODE < RPM_VERSION(4,4,8)
	DEF_TRANS_FLAG(REVERSE);
#endif
	DEF_TRANS_FLAG(NOTRIGGERPREIN);
	DEF_TRANS_FLAG(NOPRE);
	DEF_TRANS_FLAG(NOPOST);
	DEF_TRANS_FLAG(NOTRIGGERIN);
	DEF_TRANS_FLAG(NOTRIGGERUN);
	DEF_TRANS_FLAG(NOPREUN);
	DEF_TRANS_FLAG(NOPOSTUN);
	DEF_TRANS_FLAG(NOTRIGGERPOSTUN);
	DEF_TRANS_FLAG(NOPAYLOAD);
	DEF_TRANS_FLAG(APPLYONLY);
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	DEF_TRANS_FLAG(CHAINSAW);
#elif RPM_VERSION_CODE < RPM_VERSION(4,4,8)
	DEF_TRANS_FLAG(ANACONDA);
#endif
/* NOMD5 is not in jbj's 4.4.6 any more - Mandriva uses that */
#ifdef RPMTRANS_FLAG_NOMD5
	DEF_TRANS_FLAG(NOMD5);
#endif
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
#if RPM_VERSION(4,4,8) < RPM_VERSION_CODE
	DEF_TRANS_FLAG(NOSUGGEST);
	DEF_TRANS_FLAG(ADDINDEPS);
	DEF_TRANS_FLAG(NOCONFIGS);
#endif
#endif
#undef DEF_TRANS_FLAG

#define DEF_PROB_FILTER(name) \
	rb_define_const(rpm_mRPM, "PROB_FILTER_"#name, INT2NUM(RPMPROB_FILTER_##name))
	DEF_PROB_FILTER(NONE);
	DEF_PROB_FILTER(IGNOREOS);
	DEF_PROB_FILTER(IGNOREARCH);
	DEF_PROB_FILTER(REPLACEPKG);
	DEF_PROB_FILTER(FORCERELOCATE);
	DEF_PROB_FILTER(REPLACENEWFILES);
	DEF_PROB_FILTER(REPLACEOLDFILES);
	DEF_PROB_FILTER(OLDPACKAGE);
	DEF_PROB_FILTER(DISKSPACE);
	DEF_PROB_FILTER(DISKNODES);
#undef DEF_PROB_FILTER

	rb_define_const(rpm_mRPM, "PROB_FILER_FORCE",
					 INT2NUM(RPMPROB_FILTER_REPLACEPKG
							 | RPMPROB_FILTER_REPLACENEWFILES
							 | RPMPROB_FILTER_REPLACEOLDFILES));
	rb_define_const(rpm_mRPM, "PROB_FILER_REPLACEFILES",
					 INT2NUM(RPMPROB_FILTER_REPLACENEWFILES
							 | RPMPROB_FILTER_REPLACEOLDFILES));
	rb_define_const(rpm_mRPM, "PROB_FILER_IGNORESIZE",
					 INT2NUM(RPMPROB_FILTER_DISKSPACE
							 | RPMPROB_FILTER_DISKNODES));

#define DEF_BUILD(name) \
	rb_define_const(rpm_mRPM, "BUILD_"#name, INT2NUM(RPMBUILD_##name))
	DEF_BUILD(NONE);
	DEF_BUILD(PREP);
	DEF_BUILD(BUILD);
	DEF_BUILD(INSTALL);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEF_BUILD(CHECK);
#endif
	DEF_BUILD(CLEAN);
	DEF_BUILD(FILECHECK);
	DEF_BUILD(PACKAGESOURCE);
	DEF_BUILD(PACKAGEBINARY);
	DEF_BUILD(RMSOURCE);
	DEF_BUILD(RMBUILD);
#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
	DEF_BUILD(STRINGBUF);
#endif
	DEF_BUILD(RMSPEC);
#undef DEF_BUILD

	rb_define_const(rpm_mRPM, "BUILD__UNTIL_PREP",
					INT2NUM(RPMBUILD_PREP));
	rb_define_const(rpm_mRPM, "BUILD__UNTIL_BUILD",
					INT2NUM(RPMBUILD_PREP
							| RPMBUILD_BUILD));
	rb_define_const(rpm_mRPM, "BUILD__UNTIL_INSTALL",
					INT2NUM(RPMBUILD_PREP
							| RPMBUILD_BUILD
							| RPMBUILD_INSTALL));
	rb_define_const(rpm_mRPM, "BUILD__BINARY_PACKAGE",
					INT2NUM(RPMBUILD_PREP
							| RPMBUILD_BUILD
							| RPMBUILD_INSTALL
							| RPMBUILD_PACKAGEBINARY
							| RPMBUILD_CLEAN));
	rb_define_const(rpm_mRPM, "BUILD__SOURCE_PACKAGE",
					INT2NUM(RPMBUILD_PREP
							| RPMBUILD_BUILD
							| RPMBUILD_INSTALL
							| RPMBUILD_PACKAGESOURCE
							| RPMBUILD_CLEAN));
	rb_define_const(rpm_mRPM, "BUILD__ALL_PACKAGE",
					INT2NUM(RPMBUILD_PREP
							| RPMBUILD_BUILD
							| RPMBUILD_INSTALL
							| RPMBUILD_PACKAGEBINARY
							| RPMBUILD_PACKAGESOURCE
							| RPMBUILD_CLEAN));
	rb_define_const(rpm_mRPM, "BUILD__CHECK_FILELIST",
					INT2NUM(RPMBUILD_PREP
							| RPMBUILD_BUILD
							| RPMBUILD_INSTALL
							| RPMBUILD_FILECHECK));

#define DEF_MIRE(name) \
	rb_define_const(rpm_mRPM, "MIRE_"#name, INT2NUM(RPMMIRE_##name))
	DEF_MIRE(DEFAULT);
	DEF_MIRE(STRCMP);
	DEF_MIRE(REGEX);
	DEF_MIRE(GLOB);
#undef DEF_MIRE

	rb_define_module_function(rpm_mRPM, "expand", m_expand, 1);
	rb_define_module_function(rpm_mRPM, "[]", m_aref, 1);
	rb_define_module_function(rpm_mRPM, "[]=", m_aset, 2);
	rb_define_module_function(rpm_mRPM, "readrc", m_readrc, -1);
	rb_define_module_function(rpm_mRPM, "init_macros", m_init_macros, -1);
	rb_define_module_function(rpm_mRPM, "verbosity", m_get_verbosity, 0);
	rb_define_module_function(rpm_mRPM, "verbosity=", m_set_verbosity, 1);

	Init_rpm_DB();
	Init_rpm_dependency();
	Init_rpm_file();
	Init_rpm_package();
	Init_rpm_source();
	Init_rpm_spec();
	Init_rpm_transaction();
	Init_rpm_version();
	Init_rpm_MatchIterator();

	ruby_rpm_temp_format = rbtmpdir;
	temp = ALLOCA_N(char, 32);
	sprintf(temp, "/ruby-rpm.%u.%%d", getpid());
	rb_str_concat(ruby_rpm_temp_format, rb_str_new2(temp));
	rb_gc_register_address(&ruby_rpm_temp_format);

	rpmReadConfigFiles(NULL, NULL);

#if RPM_VERSION_CODE < RPM_VERSION(4,4,8)
	rpmInitMacros(NULL, macrofiles);
#elif RPM_VERSION_CODE < RPM_VERSION(4,5,90)
	rpmInitMacros(NULL, rpmMacrofiles);
#elif RPM_VERSION_CODE < RPM_VERSION(5,0,0)
	rpmInitMacros(NULL, macrofiles);
#else
	rpmInitMacros(NULL, rpmMacrofiles);
#endif
	rpmSetVerbosity(RPMLOG_EMERG);
}
