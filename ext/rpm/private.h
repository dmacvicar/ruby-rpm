/* -*- mode: C; c-basic-offset: 4; tab-width: 4; -*- */
/* Ruby/RPM
 *
 * Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.
 */

/* $Id: private.h 17 2004-03-19 05:12:19Z zaki $ */

#include <extconf.h>

#define RPM_VERSION(maj,min,pl) (((maj) << 16) + ((min) << 8) + (pl))

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <st.h>

#include <rpm/rpmcli.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include <rpm/rpmbuild.h>
#if HAVE_RPM_RPMLOG_H
#  include <rpm/rpmlog.h>
#else
#  include <rpm/rpmmessages.h>
#endif
#if HAVE_RPM_RPMTS_H
#  include <rpm/rpmts.h>
#endif
#if HAVE_RPM_RPMPS_H
#  include <rpm/rpmps.h>
#endif
#if HAVE_RPM_RPMDS_H
#  include <rpm/rpmds.h>
#endif

#include "ruby-rpm.h"

#define RPM_DB(v) (((rpm_db_t*)DATA_PTR((v)))->db)
#ifdef PKG_CACHE_TEST
#define RPM_HEADER(v) rpm_package_get_header(v)
#else
#define RPM_HEADER(v) ((Header)DATA_PTR((v)))
#endif
#define RPM_MI(v) (((rpm_mi_t*)DATA_PTR((v)))->mi)
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
#define RPM_SPEC(v) ((Spec)DATA_PTR((v)))
#else
#define RPM_SPEC(v) rpmtsSpec((rpmts)DATA_PTR((v)))
#endif
#define RPM_TRANSACTION(v) (((rpm_trans_t*)DATA_PTR((v)))->ts)
#define RPM_SCRIPT_FD(v) (((rpm_trans_t*)DATA_PTR((v)))->script_fd)

#if RPM_VERSION_CODE >= RPM_VERSION(4,5,90)
#define RPMDB_OPAQUE 1
#define RPMPS_OPAQUE 1
#else
#define RPMTS_AVAILABLE 1
#endif

typedef struct {
	rpmdb db;
	int ref_count;
} rpm_db_t;

typedef struct {
#if RPM_VERSION_CODE < RPM_VERSION(4,1,0)
	rpmTransactionSet ts;
#else
	rpmts ts;
#endif
	FD_t script_fd;
	rpm_db_t* db;
} rpm_trans_t;

typedef struct {
	rpmdbMatchIterator mi;
	rpm_db_t* db;
} rpm_mi_t;

/* db.c */
void Init_rpm_DB(void);
void Init_rpm_transaction(void);
void Init_rpm_MatchIterator(void);

/* dependency.c */
void Init_rpm_dependency(void);

/* file.c */
void Init_rpm_file(void);

/* package.c */
#ifdef PKG_CACHE_TEST
Header rpm_package_get_header(VALUE pkg);
#endif
void Init_rpm_package(void);

/* rpm.c */
VALUE ruby_rpm_make_temp_name(void);

/* source.c */
void Init_rpm_source(void);

/* spec.c */
void Init_rpm_spec(void);

/* version.c */
void Init_rpm_version(void);

#if RPM_VERSION_CODE < RPM_VERSION(4,6,0)
static void inline
get_entry(Header hdr, rpmTag tag, rpmTagType* type, void** ptr)
{
	if (!headerGetEntryMinMemory(
			hdr, tag, (hTYP_t)type, (hPTR_t*)ptr, NULL)) {
		*ptr = NULL;
	}
}
#else
static void inline
get_entry(Header hdr, rpmTag tag, rpmtd tc)
{
	headerGet(hdr, tag, tc, HEADERGET_MINMEM);
}
#endif

static void inline
release_entry(rpmTagType type, void* ptr)
{
	ptr = headerFreeData(ptr, type);
}
