/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: package.c 45 2004-06-04 15:11:20Z kazuhiko $ */

#include "private.h"
#include <st.h>

#ifndef stpcpy
char *stpcpy( char *dest, const char *source );
#endif

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

inline static VALUE
package_new_from_header(VALUE klass, Header hdr)
{
	VALUE p;
	VALUE signature;

	p = Qnil;
  signature = Qnil;

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
package_s_create(VALUE klass, VALUE name, VALUE version)
{
	Header hdr;
	VALUE pkg;

	if ((TYPE(name) != T_STRING) || (
             rb_obj_is_kind_of(version, rpm_cVersion) == Qfalse))  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	hdr = headerNew();
        headerAddEntry(hdr,RPMTAG_NAME,RPM_STRING_TYPE,RSTRING_PTR(name),1);
        headerAddEntry(hdr,RPMTAG_VERSION,RPM_STRING_TYPE,RSTRING_PTR(rpm_version_get_v(version)),1);
        headerAddEntry(hdr,RPMTAG_RELEASE,RPM_STRING_TYPE,RSTRING_PTR(rpm_version_get_r(version)),1);
        if(!NIL_P(rpm_version_get_e(version))){
		int e = NUM2INT(rpm_version_get_e(version));
        	headerAddEntry(hdr,RPMTAG_EPOCH,RPM_INT32_TYPE,&e,1);
	}
	pkg = package_new_from_header(klass, hdr);
  return pkg;
}

static rpmRC read_header_from_file(FD_t fd, const char *filename, Header *hdr)
{
	rpmRC rc;
//#if RPM_VERSION_CODE < RPM_VERSION(4,6,0)
#if 0
    /* filename is ignored */
    Header sigs;
	rc = rpmReadPackageInfo(fd, &sigs, hdr);
    headerFree(sigs);
#else
	rpmts ts = rpmtsCreate();
	rc = rpmReadPackageFile(ts, fd, filename, hdr);
	rpmtsFree(ts);
#endif
    return rc;
}

static VALUE
package_s_open(VALUE klass, VALUE filename)
{
	VALUE pkg = Qnil;
	FD_t fd;
	Header hdr;
	rpmRC rc;

	if (TYPE(filename) != T_STRING) {
		rb_raise(rb_eTypeError, "illegal argument type");
	}

	fd = Fopen(RSTRING_PTR((filename)), "r");
	if (!fd) {
		rb_raise(rb_eRuntimeError, "can not open file %s",
				 RSTRING_PTR((filename)));
	}

    rc = read_header_from_file(fd, RSTRING_PTR((filename)), &hdr);
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
		//headerFree(sigs);
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
	fd = Fopen(RSTRING_PTR((temp)), "wb+");
	Fwrite(RSTRING_PTR((str)), RSTRING_LEN(str), 1, fd);
	Fseek(fd, 0, SEEK_SET);
	hdr = headerRead(fd, HEADER_MAGIC_YES);
	Fclose(fd);
	unlink(RSTRING_PTR((temp)));

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
#if RPM_VERSION_CODE < RPM_VERSION(4,6,0)
#define rpmTag int_32
#endif
	rpmTag *copy_tags;
	int length = NUM2INT(rb_funcall(tags,rb_intern("length"),0));
	int i;

	copy_tags = ALLOCA_N(rpmTag,length);
	for (i=0;i<length;i++)
		copy_tags[i] = NUM2INT(rb_ary_entry(tags, i));
	headerCopyTags(RPM_HEADER(from),RPM_HEADER(to),copy_tags);
    return Qnil;
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

	name = RSTRING_PTR((rpm_dependency_get_name(dep)));
	evr = RSTRING_PTR((rpm_version_to_vre(rpm_dependency_get_version(dep))));
	flag = NUM2INT(rpm_dependency_get_flags(dep));

	headerAddOrAppendEntry(RPM_HEADER(pkg),nametag,RPM_STRING_ARRAY_TYPE,&name,1);
	headerAddOrAppendEntry(RPM_HEADER(pkg),versiontag,RPM_STRING_ARRAY_TYPE,&evr,1);
	headerAddOrAppendEntry(RPM_HEADER(pkg),flagstag,RPM_INT32_TYPE,&flag,1);
  return Qnil;
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
  return Qnil;
}

VALUE
rpm_package_add_string_array(VALUE pkg,VALUE tag,VALUE val)
{
	if ((TYPE(val) != T_STRING))  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}
	headerAddOrAppendEntry(RPM_HEADER(pkg),NUM2INT(tag),RPM_STRING_ARRAY_TYPE,RARRAY_PTR(val),1);
  return Qnil;
}

VALUE
rpm_package_add_string(VALUE pkg,VALUE tag,VALUE val)
{
	if ((TYPE(val) != T_STRING))  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}
	headerAddEntry(RPM_HEADER(pkg),NUM2INT(tag),RPM_STRING_TYPE,RSTRING_PTR((val)),1);
  return Qnil;
}

VALUE
rpm_package_add_binary(VALUE pkg,VALUE tag,VALUE val)
{
	if ((TYPE(val) != T_STRING))  {
		rb_raise(rb_eTypeError, "illegal argument type");
	}
	headerAddEntry(RPM_HEADER(pkg),NUM2INT(tag),RPM_BIN_TYPE,RSTRING_PTR((val)),RSTRING_LEN(val));
  return Qnil;
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

#if RPM_VERSION_CODE < RPM_VERSION(4,6,0)
VALUE
rpm_package_aref(VALUE pkg, VALUE tag)
{
	rpmTag tagval = NUM2INT(tag);
	VALUE val = Qnil;
	void* data;
	rpmTagType type;
	int_32 count;
	register int i;
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
  default:
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
#else
VALUE
rpm_package_aref(VALUE pkg, VALUE tag)
{
	rpmTag tagval = NUM2INT(tag);
	VALUE val = Qnil;
    rpmtd tagc = rpmtdNew();
	int ary_p = 0;
	int i18n_p = 0;

	if (!headerGet(RPM_HEADER(pkg), tagval, tagc, HEADERGET_MINMEM)) {
        rpmtdFree(tagc);
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
    default:
        break;
	}

	switch (rpmtdType(tagc)) {
	case RPM_BIN_TYPE:
		val = rb_str_new2(rpmtdGetString(tagc));
		break;

	case RPM_CHAR_TYPE:
	case RPM_INT8_TYPE:
		if (rpmtdCount(tagc) == 1 && !ary_p) {
			val = INT2NUM(*rpmtdGetChar(tagc));
		} else {
			val = rb_ary_new();
			rpmtdInit(tagc);
			while ( rpmtdNext(tagc) != -1 ) {
				rb_ary_push(val, INT2NUM(*rpmtdGetChar(tagc)));
			}
		}
		break;

	case RPM_INT16_TYPE:
		if (rpmtdCount(tagc) == 1 && !ary_p) {
			val = INT2NUM(*rpmtdGetUint16(tagc));
		} else {
			val = rb_ary_new();
			rpmtdInit(tagc);
			while ( rpmtdNext(tagc) != -1 ) {
				rb_ary_push(val, INT2NUM(*rpmtdGetUint16(tagc)));
			}
		}
		break;

	case RPM_INT32_TYPE:
		if (rpmtdCount(tagc) == 1 && !ary_p) {
			val = INT2NUM(*rpmtdGetUint32(tagc));
		} else {
			val = rb_ary_new();
			rpmtdInit(tagc);
			while ( rpmtdNext(tagc) != -1) {
				rb_ary_push(val, INT2NUM(*rpmtdGetUint32(tagc)));
			}
		}
		break;

	case RPM_STRING_TYPE:
		if (rpmtdCount(tagc) == 1 && !ary_p) {
			val = rb_str_new2(rpmtdGetString(tagc));
		} else {
			val = rb_ary_new();
			while ( rpmtdNext(tagc) != -1) {
				rb_ary_push(val, rb_str_new2(rpmtdGetString(tagc)));
			}
		}
		rpmtdFree(tagc);
		break;

	case RPM_STRING_ARRAY_TYPE:
		{
			if (i18n_p) {
                rpmtd i18ntd = rpmtdNew();

				if (!headerGet(RPM_HEADER(pkg), HEADER_I18NTABLE, i18ntd, HEADERGET_MINMEM)) {
					goto strary;
				}

				val = rb_hash_new();
				rpmtdInit(tagc);
				while ( rpmtdNext(tagc) != -1) {
					VALUE lang = rb_str_new2(rpmtdNextString(i18ntd));
					VALUE str = rb_str_new2(rpmtdGetString(tagc));
					rb_hash_aset(val, lang, str);
				}
                rpmtdFree(i18ntd);
			} else {
			strary:
				val = rb_ary_new();
				rpmtdInit(tagc);
				while ( rpmtdNext(tagc) != -1) {
					rb_ary_push(val, rb_str_new2(rpmtdGetString(tagc)));
				}
			}
            rpmtdFree(tagc);
		}
		break;

	default:
		goto leave;
	}
 leave:
	return val;
}
#endif

VALUE
rpm_package_sprintf(VALUE pkg, VALUE fmt)
{
	const char *errstr = "(unknown error)";
	const char *str;

	str = headerSprintf(RPM_HEADER(pkg), StringValueCStr(fmt),
			rpmTagTable, rpmHeaderFormats, &errstr);
	if (str == NULL) {
		rb_raise(rb_eRuntimeError, "incorrect format: %s",
				 errstr);
	}
	return rb_str_new2(str);
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
rpm_package_get_arch(VALUE pkg)
{
	VALUE arch;
	const char* a;
	headerNEVRA(RPM_HEADER(pkg), NULL, NULL, NULL, NULL, &a);
	arch = a ? rb_str_new2(a) : Qnil;

	return arch;
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
		for (i = 0; i < RARRAY_LEN(basenames); i++) {
			static char buf[BUFSIZ];
			VALUE file;
			buf[0] = '\0';
			stpcpy(stpcpy(buf, RSTRING_PTR((RARRAY_PTR(dirnames))[
				           NUM2INT(RARRAY_PTR(diridxs)[i])])),
					   RSTRING_PTR((RARRAY_PTR(basenames)[i])));
			file = rpm_file_new(
				buf,
				RSTRING_PTR((RARRAY_PTR(md5list)[i])),
				(NIL_P(linklist)
				 ? NULL
				 : RSTRING_PTR((RARRAY_PTR(linklist)[i]))),
				NUM2UINT(RARRAY_PTR(sizelist)[i]),
				NUM2INT(RARRAY_PTR(mtimelist)[i]),
				(NIL_P(ownerlist)
				 ? NULL
				 : RSTRING_PTR((RARRAY_PTR(ownerlist)[i]))),
				(NIL_P(grouplist)
				 ? NULL
				 : RSTRING_PTR((RARRAY_PTR(grouplist)[i]))),
				NUM2UINT(RARRAY_PTR(rdevlist)[i]),
				NUM2UINT(RARRAY_PTR(modelist)[i]),
				(NIL_P(flaglist)
				 ? RPMFILE_NONE
				 : NUM2INT(RARRAY_PTR(flaglist)[i])),
				(NIL_P(statelist)
				 ? RPMFILE_STATE_NORMAL
				 : NUM2INT(RARRAY_PTR(statelist)[i])));
			rb_ary_push(files, file);
		}
	}
	return files;
}

#if RPM_VERSION_CODE < RPM_VERSION(4,6,0)
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
#else
VALUE
rpm_package_get_dependency(VALUE pkg,int nametag,int versiontag,int flagtag,VALUE (*dependency_new)(const char*,VALUE,int,VALUE))
{
	VALUE deps;

    rpmtd nametd = rpmtdNew();
    rpmtd versiontd = rpmtdNew();
    rpmtd flagtd = rpmtdNew();

	deps = rb_ary_new();

	if (!headerGet(RPM_HEADER(pkg), nametag, nametd, HEADERGET_MINMEM)) {
		goto leave;
	}
	if (!headerGet(RPM_HEADER(pkg), versiontag, versiontd, HEADERGET_MINMEM)) {
		goto leave;
		return deps;
	}
	if (!headerGet(RPM_HEADER(pkg), flagtag, flagtd, HEADERGET_MINMEM)) {
		goto leave;
		return deps;
	}

    rpmtdInit(nametd);
    while ( rpmtdNext(nametd) != -1 ) {
        rb_ary_push(deps,dependency_new(rpmtdGetString(nametd),rpm_version_new(rpmtdNextString(versiontd)),*rpmtdNextUint32(flagtd),pkg));
    }
    
 leave:
    rpmtdFree(nametd);
    rpmtdFree(versiontd);
    rpmtdFree(flagtd);
	return deps;
}
#endif

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

#if RPM_VERSION_CODE < RPM_VERSION(4,6,0)
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
			rb_time_new((time_t)times[i], (time_t)0),
			rb_str_new2(names[i]),
			rb_str_new2(texts[i]));
		rb_ary_push(cl, chglog);
	}
	return cl;
}
#else
VALUE
rpm_package_get_changelog(VALUE pkg)
{
	VALUE cl;
	rpmtd timetd = rpmtdNew();
	rpmtd nametd = rpmtdNew();
	rpmtd texttd = rpmtdNew();

	cl = rb_ary_new();

	if (!headerGet(RPM_HEADER(pkg), RPMTAG_CHANGELOGTIME, timetd, HEADERGET_MINMEM)) {
		goto leave;
	}
	if (!headerGet(RPM_HEADER(pkg), RPMTAG_CHANGELOGNAME, nametd, HEADERGET_MINMEM)) {
		goto leave;
	}
	if (!headerGet(RPM_HEADER(pkg), RPMTAG_CHANGELOGTEXT, texttd, HEADERGET_MINMEM)) {
		goto leave;
	}

	rpmtdInit(timetd);
	rpmtdInit(nametd);
	rpmtdInit(texttd);

	while ( rpmtdNext(timetd) != -1 ) {
		VALUE chglog = rb_struct_new(
			rpm_sChangeLog,
			rb_time_new((time_t) rpmtdGetUint32(timetd), (time_t)0),
			rb_str_new2(rpmtdNextString(nametd)),
			rb_str_new2(rpmtdNextString(texttd)));
		 rb_ary_push(cl, chglog);
	}

 leave:
	rpmtdFree(timetd);
	rpmtdFree(nametd);
	rpmtdFree(texttd);
	return cl;
}
#endif

VALUE
rpm_package_dump(VALUE pkg)
{
	VALUE dump, temp;
	FD_t fd;
	off_t size;
	char* buf;

	temp = ruby_rpm_make_temp_name();
	fd = Fopen(RSTRING_PTR((temp)), "wb+");
	headerWrite(fd, RPM_HEADER(pkg), HEADER_MAGIC_YES);
	size = fdSize(fd);

	buf = mmap(NULL, size, PROT_READ, MAP_PRIVATE, Fileno(fd), 0);
	dump = rb_str_new(buf, size);
	munmap(buf, size);

	Fclose(fd);
	unlink(RSTRING_PTR((temp)));

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
	VALUE arch = rpm_package_get_arch(pkg);

	if (NIL_P(name)) {
		buf[0] = '\0';
	} else if (NIL_P(ver)) {
		snprintf(buf, BUFSIZ, "%s", RSTRING_PTR((name)));
	} else if (NIL_P(arch)) {
		snprintf(buf, BUFSIZ, "%s-%s",
				RSTRING_PTR((name)),
				RSTRING_PTR((rpm_version_to_s(ver))));
	} else {
		snprintf(buf, BUFSIZ, "%s-%s-%s",
				RSTRING_PTR((name)),
				RSTRING_PTR((rpm_version_to_s(ver))),
				RSTRING_PTR((arch)));
	}

	return rb_str_new2(buf);
}

VALUE
rpm_package_inspect(VALUE pkg)
{
	char buf[BUFSIZ];
	VALUE name = rpm_package_get_name(pkg);
	VALUE ver  = rpm_package_get_version(pkg);

	if (NIL_P(name)) {
		buf[0] = '\0';
	} else if (NIL_P(ver)) {
		snprintf(buf, BUFSIZ, "#<RPM::Package name=%s>",
				RSTRING_PTR((rb_inspect(name))));
	} else {
		snprintf(buf, BUFSIZ, "#<RPM::Package name=%s, version=%s>",
				RSTRING_PTR((rb_inspect(name))),
				RSTRING_PTR((rb_inspect(ver))));
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
	rb_define_method(rpm_cPackage, "sprintf", rpm_package_sprintf, 1);
	rb_define_method(rpm_cPackage, "signature", rpm_package_get_signature, 0);
	rb_define_method(rpm_cPackage, "arch", rpm_package_get_arch, 0);
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
	rb_define_method(rpm_cPackage, "inspect", rpm_package_inspect, 0);
	rb_define_method(rpm_cPackage, "copy_to", rpm_package_copy_tags, 2);

	rpm_sChangeLog = rb_struct_define(NULL, "time", "name", "text", NULL);
	rb_define_const(rpm_mRPM, "ChangeLog", rpm_sChangeLog);

	id_signature = rb_intern("signature");
	package_use_cache(rpm_cPackage,Qtrue);
}
