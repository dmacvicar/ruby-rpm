/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: package.c 27 2004-05-23 04:54:24Z zaki $ */

#include "private.h"

VALUE rpm_cPackage;
VALUE rpm_sChangeLog;

static ID id_signature;
struct st_table *tbl = NULL;

static VALUE
package_clear_cache(VALUE klass){
	st_free_table(tbl);
	tbl = NULL;
	return klass;
}

static VALUE
package_use_cache(VALUE klass,VALUE b){
	if((b==Qtrue)&&(tbl==NULL))
		tbl = (struct st_table*)st_init_numtable();
	else if (b == Qfalse)
		package_clear_cache(klass);
	return klass;
}

static void
package_free(Header hdr)
{
	if(tbl){
		char *sigmd5;
		VALUE signature;
		VALUE p;
		sigmd5 = headerSprintf(hdr,"%{sigmd5}",
				rpmTagTable, rpmHeaderFormats, NULL);
		if(strcmp(sigmd5,"(none)")!=0){
			signature = INT2NUM(rb_intern(sigmd5));
			st_delete(tbl,&signature,&p);
		}
		free(sigmd5);
	}
	headerFree(hdr);
}

static VALUE inline
package_new_from_header(VALUE klass, Header hdr)
{
	VALUE p;
	VALUE signature;

	p = Qnil;
	if (hdr == NULL) {
		return Qnil;
	}

	if(tbl){
		char *sigmd5;
		sigmd5 = headerSprintf(hdr,"%{sigmd5}",
				rpmTagTable, rpmHeaderFormats, NULL);
		if(strcmp(sigmd5,"(none)")!=0){
			signature = INT2NUM(rb_intern(sigmd5));
			st_lookup(tbl,signature,&p);
		}
		free(sigmd5);
	}
	if (NIL_P(p)){
		p = Data_Wrap_Struct(klass, NULL, package_free, headerLink(hdr));
		if(tbl)
			st_insert(tbl,signature,p);
	}
	return p;
}

static VALUE
package_s_create(VALUE klass, VALUE name,VALUE version)
{
	Header hdr;
	VALUE pkg;

	if ((TYPE(name) != T_STRING) || (
             rb_obj_is_kind_of(version, rpm_cVersion) == Qfalse))  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	hdr = headerNew();
        headerAddEntry(hdr,RPMTAG_NAME,RPM_STRING_TYPE,RSTRING(name)->ptr,1);
        headerAddEntry(hdr,RPMTAG_VERSION,RPM_STRING_TYPE,RSTRING(rpm_version_get_v(version))->ptr,1);
        headerAddEntry(hdr,RPMTAG_RELEASE,RPM_STRING_TYPE,RSTRING(rpm_version_get_r(version))->ptr,1);
        if(!NIL_P(rpm_version_get_e(version))){
		int e = NUM2INT(rpm_version_get_e(version));
        	headerAddEntry(hdr,RPMTAG_EPOCH,RPM_INT32_TYPE,&e,1);
	}
	pkg = package_new_from_header(klass, hdr);
}

static VALUE
package_s_open(VALUE klass, VALUE filename)
{
	VALUE pkg;
	FD_t fd;
	Header hdr, sigs;
	rpmRC rc;

	if (TYPE(filename) != T_STRING) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	fd = Fopen(RSTRING(filename)->ptr, "r");
	if (!fd) {
		rb_raise(rb_eRuntimeError, "can not open file %s",
				 RSTRING(filename)->ptr);
	}
	rc = rpmReadPackageInfo(fd, &sigs, &hdr);
	Fclose(fd);

	switch (rc) {
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	case RPMRC_BADSIZE:
#else
	case RPMRC_NOTTRUSTED:
	case RPMRC_NOKEY:
		/* MO_TODO: zaki: should I warn these two status to users? */
#endif
	case RPMRC_OK:
		headerFree(sigs);
		pkg = package_new_from_header(klass, hdr);
		headerFree(hdr);
		break;

#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	case RPMRC_BADMAGIC:
		rb_raise(rb_eRuntimeError, "bad magic");
		break;

#else
	case RPMRC_NOTFOUND:
		rb_raise(rb_eRuntimeError, "bad magic");
		break;

#endif
	case RPMRC_FAIL:
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	case RPMRC_SHORTREAD:
#endif
		rb_raise(rb_eRuntimeError, "error reading pakcage");
		break;
	}

	return pkg;
}

static VALUE
package_s_load(VALUE klass, VALUE str)
{
	VALUE pkg, temp;
	Header hdr;
	FD_t fd;

	Check_Type(str, T_STRING);

	temp = ruby_rpm_make_temp_name();
	fd = Fopen(RSTRING(temp)->ptr, "wb+");
	Fwrite(RSTRING(str)->ptr, RSTRING(str)->len, 1, fd);
	Fseek(fd, 0, SEEK_SET);
	hdr = headerRead(fd, HEADER_MAGIC_YES);
	Fclose(fd);
	Unlink(RSTRING(temp)->ptr);

	if (!hdr) {
		rb_raise(rb_eArgError, "unable load RPM::Package");
	}

	pkg = package_new_from_header(klass, hdr);
	headerFree(hdr);

	return pkg;
}

VALUE
rpm_package_copy_tags(VALUE from,VALUE to,VALUE tags)
{
	int_32 *copy_tags;
	int length = NUM2INT(rb_funcall(tags,rb_intern("length"),0));
	int i;

	copy_tags = ALLOCA_N(int_32,length);
	for (i=0;i<length;i++)
		copy_tags[i] = NUM2INT(rb_ary_entry(tags, i));
	headerCopyTags(RPM_HEADER(from),RPM_HEADER(to),copy_tags);
}

VALUE
rpm_package_new_from_header(Header hdr)
{
	return package_new_from_header(rpm_cPackage, hdr);
}

#if RPM_VERSION(4,1,0) <= RPM_VERSION_CODE
VALUE
rpm_package_new_from_N_EVR(VALUE name, VALUE version)
{
	return package_s_create(rpm_cPackage, name, version);
}
#endif

VALUE
rpm_package_add_dependency(VALUE pkg,VALUE dep)
{
	int nametag;
	int versiontag;
	int flagstag;
	char* name;
	char* evr;
	int flag;

	if ( rb_obj_is_kind_of(dep, rpm_cDependency) == Qfalse )  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	nametag = NUM2INT(rpm_dependency_get_nametag(dep));
	versiontag = NUM2INT(rpm_dependency_get_versiontag(dep));
	flagstag = NUM2INT(rpm_dependency_get_flagstag(dep));

	name = RSTRING(rpm_dependency_get_name(dep))->ptr;
	evr = RSTRING(rpm_version_to_vre(rpm_dependency_get_version(dep)))->ptr;
	flag = NUM2INT(rpm_dependency_get_flags(dep));

	headerAddOrAppendEntry(RPM_HEADER(pkg),nametag,RPM_STRING_ARRAY_TYPE,&name,1);
	headerAddOrAppendEntry(RPM_HEADER(pkg),versiontag,RPM_STRING_ARRAY_TYPE,&evr,1);
	headerAddOrAppendEntry(RPM_HEADER(pkg),flagstag,RPM_INT32_TYPE,&flag,1);
}

VALUE
rpm_package_add_int32(VALUE pkg,VALUE tag,VALUE val)
{
	int_32 v;
	if (TYPE(val) == T_FIXNUM) {
		v = (int_32) FIX2LONG(val);
	}
	else if (TYPE(val) == T_BIGNUM) {
		v = (int_32) NUM2LONG(val);
	}
	else {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	headerAddOrAppendEntry(RPM_HEADER(pkg),NUM2INT(tag),RPM_INT32_TYPE,&v,1);
}

VALUE
rpm_package_add_string_array(VALUE pkg,VALUE tag,VALUE val)
{
	if ((TYPE(val) != T_STRING))  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}
	headerAddOrAppendEntry(RPM_HEADER(pkg),NUM2INT(tag),RPM_STRING_ARRAY_TYPE,&RSTRING(val)->ptr,1);
}

VALUE
rpm_package_add_string(VALUE pkg,VALUE tag,VALUE val)
{
	if ((TYPE(val) != T_STRING))  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}
	headerAddEntry(RPM_HEADER(pkg),NUM2INT(tag),RPM_STRING_TYPE,RSTRING(val)->ptr,1);
}

VALUE
rpm_package_add_binary(VALUE pkg,VALUE tag,VALUE val)
{
	if ((TYPE(val) != T_STRING))  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}
	headerAddEntry(RPM_HEADER(pkg),NUM2INT(tag),RPM_BIN_TYPE,RSTRING(val)->ptr,RSTRING(val)->len);
}

VALUE
rpm_package_delete_tag(VALUE pkg, VALUE tag)
{
	rpmTag tagval = NUM2INT(tag);
	VALUE val;

	val = rpm_package_aref(pkg, tag);
	headerRemoveEntry(RPM_HEADER(pkg), tagval);
	return val;
}

VALUE
rpm_package_aref(VALUE pkg, VALUE tag)
{
	rpmTag tagval = NUM2INT(tag);
	VALUE val = Qnil;
	void* data;
	rpmTagType type;
	int_32 count;
	register int i;
	register const char* p;
	int ary_p = 0;
	int i18n_p = 0;

	if (!headerGetEntryMinMemory(RPM_HEADER(pkg), tagval, (hTYP_t)&type,
									 (hPTR_t*)&data, (hCNT_t)&count)) {
		goto leave;
	}
	switch (tagval) {
	case RPMTAG_DIRINDEXES:
	case RPMTAG_FILESIZES:
	case RPMTAG_FILESTATES:
	case RPMTAG_FILEMODES:
	case RPMTAG_FILERDEVS:
	case RPMTAG_FILEMTIMES:
	case RPMTAG_FILEMD5S:
	case RPMTAG_FILEFLAGS:
	case RPMTAG_FILEUSERNAME:
	case RPMTAG_FILEGROUPNAME:
	case RPMTAG_PROVIDEFLAGS:
	case RPMTAG_REQUIREFLAGS:
	case RPMTAG_CONFLICTFLAGS:
	case RPMTAG_OBSOLETEFLAGS:
		ary_p = 1;
		break;

	case RPMTAG_GROUP:
	case RPMTAG_SUMMARY:
	case RPMTAG_DISTRIBUTION:
	case RPMTAG_VENDOR:
	case RPMTAG_LICENSE:
	case RPMTAG_PACKAGER:
	case RPMTAG_DESCRIPTION:
		i18n_p = 1;
		break;
	}

	switch (type) {
	case RPM_BIN_TYPE:
		val = rb_str_new(data, count);
		break;

	case RPM_CHAR_TYPE:
	case RPM_INT8_TYPE:
		if (count == 1 && !ary_p) {
			val = INT2NUM(*(int_8*)data);
		} else {
			val = rb_ary_new();
			for (i = 0; i < count; i++) {
				rb_ary_push(val, INT2NUM(((int_8*)data)[i]));
			}
		}
		break;

	case RPM_INT16_TYPE:
		if (count == 1 && !ary_p) {
			val = INT2NUM(*(int_16*)data);
		} else {
			val = rb_ary_new();
			for (i = 0; i < count; i++) {
				rb_ary_push(val, INT2NUM(((int_16*)data)[i]));
			}
		}
		break;

	case RPM_INT32_TYPE:
		if (count == 1 && !ary_p) {
			val = INT2NUM(*(int_32*)data);
		} else {
			val = rb_ary_new();
			for (i = 0; i < count; i++) {
				rb_ary_push(val, INT2NUM(((int_32*)data)[i]));
			}
		}
		break;

	case RPM_STRING_TYPE:
		if (count == 1 && !ary_p) {
			val = rb_str_new2((char*)data);
		} else {
			char** p = (char**)data;
			val = rb_ary_new();
			for (i = 0; i < count; i++) {
				rb_ary_push(val, rb_str_new2(p[i]));
			}
		}
		release_entry(type, data);
		break;

	case RPM_STRING_ARRAY_TYPE:
		{
			char** p = (char**)data;
			if (i18n_p) {
				char** i18ntab;
				rpmTagType i18nt;
				int_32 i18nc;

				if (!headerGetEntryMinMemory(
						RPM_HEADER(pkg), HEADER_I18NTABLE, (hTYP_t)&i18nt,
						(hPTR_t*)&i18ntab, (hCNT_t)&i18nc)) {
					goto strary;
				}

				val = rb_hash_new();
				for (i = 0; i < count; i++) {
					VALUE lang = rb_str_new2(i18ntab[i]);
					VALUE str = rb_str_new2(p[i]);
					rb_hash_aset(val, lang, str);
				}
				release_entry(i18nt, (void*)i18ntab);
			} else {
			strary:
				val = rb_ary_new();
				for (i = 0; i < count; i++) {
					rb_ary_push(val, rb_str_new2(p[i]));
				}
			}
			release_entry(type, data);
		}
		break;

	default:
		goto leave;
	}
 leave:
	return val;
}

VALUE
rpm_package_get_name(VALUE pkg)
{
	VALUE name;
	const char* n;
	headerNVR(RPM_HEADER(pkg), &n, NULL, NULL);
	name = n ? rb_str_new2(n) : Qnil;

	return name;
}

VALUE
rpm_package_get_signature(VALUE pkg)
{
	VALUE signature = rb_ivar_get(pkg, id_signature);

	if (NIL_P(signature)) {
        	char *sigmd5;
		sigmd5 = headerSprintf(RPM_HEADER(pkg),"%{sigmd5}",
				rpmTagTable, rpmHeaderFormats, NULL);
		if(sigmd5[0]){
			signature = INT2NUM(rb_intern(sigmd5));
			rb_ivar_set(pkg, id_signature, signature);
		}
		free(sigmd5);
	}
	return signature;
}

VALUE
rpm_package_get_version(VALUE pkg)
{
	VALUE ver;
	const char* v;
	const char* r;
	VALUE e;

	headerNVR(RPM_HEADER(pkg), NULL, &v, &r);
	if (!v) {
		ver = Qnil;
	} else if (!r) {
		ver = rpm_version_new(v);
	} else {
		e = rpm_package_aref(pkg, INT2NUM(RPMTAG_EPOCH));
		if (NIL_P(e)) {
			char* buf = ALLOCA_N(char, strlen(v) + strlen(r) + 2);
			sprintf(buf, "%s-%s", v, r);
			ver = rpm_version_new(buf);
		} else {
			ver = rpm_version_new3(v, r, NUM2INT(e));
		}
	}

	return ver;
}

VALUE
rpm_package_get_files(VALUE pkg)
{
	VALUE files;
	VALUE basenames = rpm_package_aref(pkg, INT2NUM(RPMTAG_BASENAMES));
	VALUE dirnames = rpm_package_aref(pkg, INT2NUM(RPMTAG_DIRNAMES));
	VALUE diridxs = rpm_package_aref(pkg, INT2NUM(RPMTAG_DIRINDEXES));
	VALUE statelist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILESTATES));
	VALUE flaglist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILEFLAGS));
	VALUE sizelist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILESIZES));
	VALUE modelist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILEMODES));
	VALUE mtimelist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILEMTIMES));
	VALUE rdevlist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILERDEVS));
	VALUE linklist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILELINKTOS));
	VALUE md5list = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILEMD5S));
	VALUE ownerlist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILEUSERNAME));
	VALUE grouplist = rpm_package_aref(pkg, INT2NUM(RPMTAG_FILEGROUPNAME));
	register int i;

	files = rb_ary_new();
	if (!NIL_P(basenames)) {
		for (i = 0; i < RARRAY(basenames)->len; i++) {
			static char buf[BUFSIZ];
			VALUE file;
			buf[0] = '\0';
			stpcpy(stpcpy(buf, RSTRING(RARRAY(dirnames)->ptr[
				           NUM2INT(RARRAY(diridxs)->ptr[i])])->ptr),
					   RSTRING(RARRAY(basenames)->ptr[i])->ptr);
			file = rpm_file_new(
				buf,
				RSTRING(RARRAY(md5list)->ptr[i])->ptr,
				(NIL_P(linklist)
				 ? NULL
				 : RSTRING(RARRAY(linklist)->ptr[i])->ptr),
				NUM2UINT(RARRAY(sizelist)->ptr[i]),
				NUM2INT(RARRAY(mtimelist)->ptr[i]),
				(NIL_P(ownerlist)
				 ? NULL
				 : RSTRING(RARRAY(ownerlist)->ptr[i])->ptr),
				(NIL_P(grouplist)
				 ? NULL
				 : RSTRING(RARRAY(grouplist)->ptr[i])->ptr),
				NUM2UINT(RARRAY(rdevlist)->ptr[i]),
				NUM2UINT(RARRAY(modelist)->ptr[i]),
				(NIL_P(flaglist)
				 ? RPMFILE_NONE
				 : NUM2INT(RARRAY(flaglist)->ptr[i])),
				(NIL_P(statelist)
				 ? RPMFILE_STATE_NORMAL
				 : NUM2INT(RARRAY(statelist)->ptr[i])));
			rb_ary_push(files, file);
		}
	}
	return files;
}

VALUE
rpm_package_get_dependency(VALUE pkg,int nametag,int versiontag,int flagtag,VALUE (*dependency_new)(const char*,VALUE,int,VALUE))
{
	VALUE deps;
	register int i;

	char **names,**versions;
	int_32 *flags;
	rpmTagType nametype,versiontype,flagtype;
	int_32 count;

	deps = rb_ary_new();

	if (!headerGetEntryMinMemory(RPM_HEADER(pkg), nametag, (hTYP_t)&nametype,
						 (hPTR_t*)&names, (hCNT_t)&count)) {
		return deps;
	}
	if (!headerGetEntryMinMemory(RPM_HEADER(pkg), versiontag, (hTYP_t)&versiontype,
						 (hPTR_t*)&versions, (hCNT_t)&count)) {
		release_entry(nametype, (void*)names);
		return deps;
	}
	if (!headerGetEntryMinMemory(RPM_HEADER(pkg), flagtag, (hTYP_t)&flagtype,
						 (hPTR_t*)&flags, (hCNT_t)&count)) {
		release_entry(nametype, (void*)names);
		release_entry(versiontype, (void*)versions);
		return deps;
	}

	for (i = 0; i < count; i++) {
		rb_ary_push(deps,dependency_new(names[i],rpm_version_new(versions[i]),flags[i],pkg));
	}

	release_entry(nametype, (void*)names);
	release_entry(versiontype, (void*)versions);
	release_entry(flagtype, (void*)flags);
	return deps;
}

VALUE
rpm_package_get_provides(VALUE pkg)
{
	return rpm_package_get_dependency(pkg,RPMTAG_PROVIDENAME,RPMTAG_PROVIDEVERSION,RPMTAG_PROVIDEFLAGS,rpm_provide_new);
}

VALUE
rpm_package_get_requires(VALUE pkg)
{
	return rpm_package_get_dependency(pkg,RPMTAG_REQUIRENAME,RPMTAG_REQUIREVERSION,RPMTAG_REQUIREFLAGS,rpm_require_new);
}

VALUE
rpm_package_get_conflicts(VALUE pkg)
{
	return rpm_package_get_dependency(pkg,RPMTAG_CONFLICTNAME,RPMTAG_CONFLICTVERSION,RPMTAG_CONFLICTFLAGS,rpm_conflict_new);
}

VALUE
rpm_package_get_obsoletes(VALUE pkg)
{
	return rpm_package_get_dependency(pkg,RPMTAG_OBSOLETENAME,RPMTAG_OBSOLETEVERSION,RPMTAG_OBSOLETEFLAGS,rpm_obsolete_new);
}

VALUE
rpm_package_get_changelog(VALUE pkg)
{
	VALUE cl;
	register int i;

	char **times,**names,**texts;
	rpmTagType timetype,nametype,texttype;
	int_32 count;

	cl = rb_ary_new();

	if (!headerGetEntryMinMemory(RPM_HEADER(pkg), RPMTAG_CHANGELOGTIME, (hTYP_t)&timetype,
						 (hPTR_t*)&times, (hCNT_t)&count)) {
		return cl;
	}
	if (!headerGetEntryMinMemory(RPM_HEADER(pkg), RPMTAG_CHANGELOGNAME, (hTYP_t)&nametype,
						 (hPTR_t*)&names, (hCNT_t)&count)) {
		release_entry(timetype, (void*)times);
		return cl;
	}
	if (!headerGetEntryMinMemory(RPM_HEADER(pkg), RPMTAG_CHANGELOGTEXT, (hTYP_t)&texttype,
						 (hPTR_t*)&texts, (hCNT_t)&count)) {
		release_entry(timetype, (void*)times);
		release_entry(nametype, (void*)names);
		return cl;
	}

	for (i = 0; i < count; i++) {
		VALUE chglog = rb_struct_new(
			rpm_sChangeLog,
			rb_time_new(times[i]),
			rb_str_new2(names[i]),
			rb_str_new2(texts[i]));
		rb_ary_push(cl, chglog);
	}
	return cl;
}

VALUE
rpm_package_dump(VALUE pkg)
{
	VALUE dump, temp;
	FD_t fd;
	off_t size;
	char* buf;

	temp = ruby_rpm_make_temp_name();
	fd = Fopen(RSTRING(temp)->ptr, "wb+");
	headerWrite(fd, RPM_HEADER(pkg), HEADER_MAGIC_YES);
	size = fdSize(fd);

	buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, Fileno(fd), 0);
	dump = rb_str_new(buf, size);
	munmap(buf, size);

	Fclose(fd);
	Unlink(RSTRING(temp)->ptr);

	return dump;
}

VALUE
rpm_package__dump(VALUE pkg, VALUE limit)
{
	return rpm_package_dump(pkg);
}

VALUE
rpm_package_to_s(VALUE pkg)
{
	char buf[BUFSIZ];
	VALUE name = rpm_package_get_name(pkg);
	VALUE ver  = rpm_package_get_version(pkg);

	if (NIL_P(name)) {
		buf[0] = '\0';
	} else if (NIL_P(ver)) {
		sprintf(buf, "%s", RSTRING(name)->ptr);
	} else {
		sprintf(buf, "%s-%s",
				RSTRING(name)->ptr,
				RSTRING(rpm_version_to_s(ver))->ptr);
	}

	return rb_str_new2(buf);
}

void
Init_rpm_package(void)
{
	rpm_cPackage = rb_define_class_under(rpm_mRPM, "Package", rb_cObject);
	rb_define_singleton_method(rpm_cPackage, "open", package_s_open, 1);
	rb_define_singleton_method(rpm_cPackage, "new", package_s_open, 1);
	rb_define_singleton_method(rpm_cPackage, "create", package_s_create, 2);
	rb_define_singleton_method(rpm_cPackage, "load", package_s_load, 1);
	rb_define_alias(rb_singleton_class(rpm_cPackage), "_load", "load");
	rb_define_singleton_method(rpm_cPackage, "clear_cache", package_clear_cache, 0);
	rb_define_singleton_method(rpm_cPackage, "use_cache=", package_use_cache, 1);
	rb_define_method(rpm_cPackage, "[]", rpm_package_aref, 1);
	rb_define_method(rpm_cPackage, "delete_tag", rpm_package_delete_tag, 1);
	rb_define_method(rpm_cPackage, "signature", rpm_package_get_signature, 0);
	rb_define_method(rpm_cPackage, "name", rpm_package_get_name, 0);
	rb_define_method(rpm_cPackage, "version", rpm_package_get_version, 0);
	rb_define_method(rpm_cPackage, "files", rpm_package_get_files, 0);
	rb_define_method(rpm_cPackage, "provides", rpm_package_get_provides, 0);
	rb_define_method(rpm_cPackage, "requires", rpm_package_get_requires, 0);
	rb_define_method(rpm_cPackage, "conflicts", rpm_package_get_conflicts, 0);
	rb_define_method(rpm_cPackage, "obsoletes", rpm_package_get_obsoletes, 0);
	rb_define_method(rpm_cPackage, "changelog", rpm_package_get_changelog, 0);
	rb_define_method(rpm_cPackage, "add_dependency", rpm_package_add_dependency, 1);
	rb_define_method(rpm_cPackage, "add_string", rpm_package_add_string, 2);
	rb_define_method(rpm_cPackage, "add_string_array", rpm_package_add_string_array, 2);
	rb_define_method(rpm_cPackage, "add_int32", rpm_package_add_int32, 2);
	rb_define_method(rpm_cPackage, "add_binary", rpm_package_add_binary, 2);
	rb_define_method(rpm_cPackage, "dump", rpm_package_dump, 0);
	rb_define_method(rpm_cPackage, "_dump", rpm_package__dump, 1);
	rb_define_method(rpm_cPackage, "to_s", rpm_package_to_s, 0);
	rb_define_method(rpm_cPackage, "copy_to", rpm_package_copy_tags, 2);

	rpm_sChangeLog = rb_struct_define(NULL, "time", "name", "text", NULL);
	rb_define_const(rpm_mRPM, "ChangeLog", rpm_sChangeLog);

	id_signature = rb_intern("signature");
	package_use_cache(rpm_cPackage,Qtrue);
}
