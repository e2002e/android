/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/*********** Preprocessed module -- do not edit ***************/
/***************** gpre version LI-T3.0.0.31129 Firebird 3.0 Alpha 2 **********************/
/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dba.epp
 *	DESCRIPTION:	Database analysis tool
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2001.08.07 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */


#include "firebird.h"
#include "../common/classes/fb_string.h"
#include <stdio.h>
#include "../common/classes/alloc.h"
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "../jrd/ibsetjmp.h"
#include "../common/classes/timestamp.h"
#include "../jrd/ibase.h"
#include "../jrd/ods.h"
#include "../jrd/btn.h"
#include "../jrd/svc.h"
#include "../jrd/license.h"
#include "../jrd/msg_encode.h"
#include "../common/gdsassert.h"
#include "../utilities/gstat/ppg_proto.h"
#include "../utilities/gstat/dbaswi.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_f_proto.h"
#include "../common/thd.h"
#include "../common/utils_proto.h"
#include "../common/classes/ClumpletWriter.h"
#include "../jrd/constants.h"
#include "../jrd/ods_proto.h"
#include "../common/classes/MsgPrint.h"
#include "../common/classes/UserBlob.h"

using MsgFormat::SafeArg;


#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

using namespace Firebird;
using namespace Jrd;
using namespace Ods;


// For Netware the follow DB handle and isc_status is #defined to be a
// local variable on the stack in main.  This is to avoid multiple
// threading problems with module level statics.
/*DATABASE DB = STATIC "yachts.lnk";*/
/**** GDS Preprocessor Definitions ****/
#ifndef JRD_IBASE_H
#include <ibase.h>
#endif

static const ISC_QUAD
   isc_blob_null = {0, 0};	/* initializer for blobs */
static isc_db_handle
   DB = 0;		/* database handle */

static isc_tr_handle
   gds_trans = 0;		/* default transaction handle */
static ISC_STATUS
   isc_status [20],	/* status vector */
   isc_status2 [20];	/* status vector */
static ISC_LONG
   isc_array_length, 	/* array return size */
   SQLCODE;		/* SQL status code */
static const char
   isc_tpb_0 [4] = {1,8,2,6};

static const short
   isc_1l = 152;
static const char
   isc_1 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 3,0, 
	    blr_quad, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_short, 0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 11, 'R','D','B','$','F','O','R','M','A','T','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
			   blr_parameter, 0, 0,0, 
		     blr_sort, 1, 
			blr_ascending, 
			   blr_field, 0, 10, 'R','D','B','$','F','O','R','M','A','T', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 14, 'R','D','B','$','D','E','S','C','R','I','P','T','O','R', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 0, 10, 'R','D','B','$','F','O','R','M','A','T', 
			   blr_parameter, 1, 2,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_1 */

static const short
   isc_8l = 194;
static const char
   isc_8 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 4,0, 
	    blr_cstring2, 3,0, 32,0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_cstring2, 3,0, 32,0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 11, 'R','D','B','$','I','N','D','I','C','E','S', 0, 
		     blr_boolean, 
			blr_eql, 
			   blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			   blr_parameter, 0, 0,0, 
		     blr_sort, 1, 
			blr_descending, 
			   blr_field, 0, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 14, 'R','D','B','$','I','N','D','E','X','_','N','A','M','E', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 0, 12, 'R','D','B','$','I','N','D','E','X','_','I','D', 
			   blr_parameter, 1, 2,0, 
			blr_assignment, 
			   blr_field, 0, 18, 'R','D','B','$','I','N','D','E','X','_','I','N','A','C','T','I','V','E', 
			   blr_parameter, 1, 3,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_8 */

static const short
   isc_16l = 167;
static const char
   isc_16 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 1, 3,0, 
	    blr_long, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_message, 0, 1,0, 
	    blr_short, 0, 
	 blr_receive, 0, 
	    blr_begin, 
	       blr_for, 
		  blr_rse, 1, 
		     blr_relation, 9, 'R','D','B','$','P','A','G','E','S', 0, 
		     blr_boolean, 
			blr_and, 
			   blr_eql, 
			      blr_field, 0, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
			      blr_parameter, 0, 0,0, 
			   blr_eql, 
			      blr_field, 0, 17, 'R','D','B','$','P','A','G','E','_','S','E','Q','U','E','N','C','E', 
			      blr_literal, blr_long, 0, 0,0,0,0,
		     blr_end, 
		  blr_send, 1, 
		     blr_begin, 
			blr_assignment, 
			   blr_field, 0, 15, 'R','D','B','$','P','A','G','E','_','N','U','M','B','E','R', 
			   blr_parameter, 1, 0,0, 
			blr_assignment, 
			   blr_literal, blr_long, 0, 1,0,0,0,
			   blr_parameter, 1, 1,0, 
			blr_assignment, 
			   blr_field, 0, 13, 'R','D','B','$','P','A','G','E','_','T','Y','P','E', 
			   blr_parameter, 1, 2,0, 
			blr_end, 
	       blr_send, 1, 
		  blr_assignment, 
		     blr_literal, blr_long, 0, 0,0,0,0,
		     blr_parameter, 1, 1,0, 
	       blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_16 */

static const short
   isc_23l = 256;
static const char
   isc_23 [] = {
      blr_version4,
      blr_begin, 
	 blr_message, 0, 10,0, 
	    blr_cstring2, 3,0, 32,0, 
	    blr_cstring2, 0,0, 0,1, 
	    blr_quad, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	    blr_short, 0, 
	 blr_begin, 
	    blr_for, 
	       blr_rse, 1, 
		  blr_relation, 13, 'R','D','B','$','R','E','L','A','T','I','O','N','S', 0, 
		  blr_sort, 1, 
		     blr_descending, 
			blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
		  blr_end, 
	       blr_send, 0, 
		  blr_begin, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','N','A','M','E', 
			blr_parameter, 0, 0,0, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','E','X','T','E','R','N','A','L','_','F','I','L','E', 
			blr_parameter2, 0, 1,0, 7,0, 
		     blr_assignment, 
			blr_field, 0, 12, 'R','D','B','$','V','I','E','W','_','B','L','R', 
			blr_parameter2, 0, 2,0, 8,0, 
		     blr_assignment, 
			blr_literal, blr_long, 0, 1,0,0,0,
			blr_parameter, 0, 3,0, 
		     blr_assignment, 
			blr_field, 0, 15, 'R','D','B','$','R','E','L','A','T','I','O','N','_','I','D', 
			blr_parameter, 0, 4,0, 
		     blr_assignment, 
			blr_field, 0, 17, 'R','D','B','$','R','E','L','A','T','I','O','N','_','T','Y','P','E', 
			blr_parameter, 0, 5,0, 
		     blr_assignment, 
			blr_field, 0, 15, 'R','D','B','$','S','Y','S','T','E','M','_','F','L','A','G', 
			blr_parameter2, 0, 9,0, 6,0, 
		     blr_end, 
	    blr_send, 0, 
	       blr_assignment, 
		  blr_literal, blr_long, 0, 0,0,0,0,
		  blr_parameter, 0, 3,0, 
	    blr_end, 
	 blr_end, 
      blr_eoc
   };	/* end of blr string for request isc_23 */


#define gds_blob_null	isc_blob_null	/* compatibility symbols */
#define gds_status	isc_status
#define gds_status2	isc_status2
#define gds_array_length	isc_array_length
#define gds_count	isc_count
#define gds_slack	isc_slack
#define gds_utility	isc_utility	/* end of compatibility symbols */

#ifndef isc_version4
    Generate a compile-time error.
    Picking up a V3 include file after preprocessing with V4 GPRE.
#endif

/**** end of GPRE definitions ****/

#define DB          db_handle
#define isc_status  status_vector

const SSHORT BUCKETS	= 5;
//#define WINDOW_SIZE	(1 << 17)

struct dba_idx
{
	dba_idx* idx_next;
	SSHORT idx_id;
	ULONG idx_root;
	SSHORT idx_depth;
	ULONG idx_leaf_buckets;
	FB_UINT64 idx_total_duplicates;
	FB_UINT64 idx_max_duplicates;
	FB_UINT64 idx_nodes;
	FB_UINT64 idx_total_length;
	FB_UINT64 idx_prefix_length;
	FB_UINT64 idx_data_length;
	FB_UINT64 idx_unpacked_length;
	FB_UINT64 idx_packed_length;
	FB_UINT64 idx_diff_pages;
	ULONG idx_fill_distribution[BUCKETS];
	SCHAR idx_name[MAX_SQL_IDENTIFIER_SIZE];
};

struct dba_fmt
{
	dba_fmt* fmt_next;
	USHORT fmt_number;
	USHORT fmt_length;
	bool fmt_used;
};

struct dba_rel
{
	dba_rel* rel_next;
	dba_idx* rel_indexes;
	dba_fmt* rel_formats;
	ULONG rel_index_root;
	ULONG rel_pointer_page;
	ULONG rel_slots;
	ULONG rel_pointer_pages;
	ULONG rel_data_pages;
	ULONG rel_full_pages;
	ULONG rel_primary_pages;
	ULONG rel_swept_pages;
	ULONG rel_blob_pages;
	ULONG rel_bigrec_pages;
	FB_UINT64 rel_records;
	FB_UINT64 rel_record_space;
	FB_UINT64 rel_versions;
	FB_UINT64 rel_version_space;
	FB_UINT64 rel_max_versions;
	FB_UINT64 rel_fragments;
	FB_UINT64 rel_fragment_space;
	FB_UINT64 rel_max_fragments;
	FB_UINT64 rel_blobs_level_0;
	FB_UINT64 rel_blobs_level_1;
	FB_UINT64 rel_blobs_level_2;
	FB_UINT64 rel_blob_space;
	ULONG rel_fill_distribution[BUCKETS];
	FB_UINT64 rel_format_space;
	FB_UINT64 rel_total_space;
	USHORT rel_total_formats;
	USHORT rel_used_formats;
	SSHORT rel_id;
	SCHAR rel_name[MAX_SQL_IDENTIFIER_SIZE];
};

// kidnapped from jrd/pio.h and abused

struct dba_fil
{
	dba_fil* fil_next;			// Next file in database
	ULONG fil_min_page;			// Minimum page number in file
	ULONG fil_max_page;			// Maximum page number in file
	USHORT fil_fudge;			// Fudge factor for page relocation
#ifdef WIN_NT
	void *fil_desc;
#else
	int fil_desc;
#endif
	USHORT fil_length;			// Length of expanded file name
	SCHAR fil_string[1];		// Expanded file name
};

static char* alloc(size_t);
static void analyze_blob(dba_rel*, const blh*, int length);
static void analyze_data(dba_rel*, bool);
static bool analyze_data_page(dba_rel*, const data_page*, bool);
static ULONG analyze_fragments(dba_rel*, const rhdf*);
static ULONG analyze_versions(dba_rel*, const rhdf*);
static void analyze_index(const dba_rel*, dba_idx*);

#if (defined WIN_NT)
static void db_error(SLONG);
#else
static void db_error(int);
#endif

static USHORT get_format_length(ISC_STATUS*, isc_db_handle, isc_tr_handle, ISC_QUAD&);

static dba_fil* db_open(const char*, USHORT);
static const pag* db_read(SLONG page_number, bool ok_enc = false);
#ifdef WIN_NT
static void db_close(void* file_desc);
#else
static void db_close(int);
#endif
static void dba_error(USHORT, const SafeArg& arg = SafeArg());
static void dba_print(bool, USHORT, const SafeArg& arg = SafeArg());
static void print_distribution(const SCHAR*, const ULONG*);
static void print_help();


#include "../common/db_alias.h"

#include <fcntl.h>
#if (defined WIN_NT)
#include <share.h>
#endif

#include "../utilities/gstat/dba_proto.h"

struct open_files
{
#ifdef WIN_NT
	void* desc;
#else
	int desc;
#endif
	open_files* open_files_next;
};

struct dba_mem
{
	char* memory;
	dba_mem* mem_next;
};

// threading declarations for thread data

class tdba : public ThreadData
{
public:
	explicit tdba(Firebird::UtilSvc* us)
		: ThreadData(tddDBA), uSvc(us)
	{
		//dba_throw = false;
		files = 0;
		relations = 0;
		page_size = 0;
		dp_per_pp = 0;
		max_records = 0;
		page_number = 0;
		buffer1 = 0;
		buffer2 = 0;
		global_buffer = 0;
		exit_code = 0;
		head_of_mem_list = 0;
		head_of_files_list = 0;
		memset(dba_status_vector, 0, sizeof (dba_status_vector));
		dba_status = dba_status_vector;
	}

	//bool dba_throw;
	Firebird::UtilSvc* uSvc;
	dba_fil* files;
	dba_rel* relations;
	SSHORT page_size;
	USHORT dp_per_pp;
	USHORT max_records;
	SLONG page_number;
	pag* buffer1;
	pag* buffer2;
	pag* global_buffer;
	int exit_code;
	dba_mem *head_of_mem_list;
	open_files *head_of_files_list;
	ISC_STATUS *dba_status;
	ISC_STATUS_ARRAY dba_status_vector;

	static inline tdba* getSpecific()
	{
		return (tdba*) ThreadData::getSpecific();
	}
	static inline void putSpecific(tdba* &tddba, tdba* thd_context)
	{
		tddba = thd_context;
		tddba->ThreadData::putSpecific();
	}
	static inline void restoreSpecific()
	{
		ThreadData::restoreSpecific();
	}
};


void inline dba_exit(int code, tdba* tddba)
{
	tddba->exit_code = code;
	// Throw this kind of exception, because gstat uses status vector (and stuff_exception) to
    // handle errors
	Firebird::LongJump::raise();
}

namespace
{
	// all this stuff gets activated ONLY in utility mode
	// therefore we can safely use static variables here

	bool shutdownRequested = false;

	int gstatShutdown(const int reason, const int, void*)
	{
		if (reason == fb_shutrsn_signal)
		{
			shutdownRequested = true;
			return FB_FAILURE;
		}
		return FB_SUCCESS;
	}

	void checkForShutdown(tdba* tddba)
	{
		if (shutdownRequested)
		{
			dba_exit(FINI_OK, tddba);
		}
	}
} // namespace

const USHORT GSTAT_MSG_FAC	= 21;


int main_gstat(Firebird::UtilSvc* uSvc)
{
/**********************************************
 *
 *    m a i n _ g s t a t
 *
 **********************************************
 * Functional Description:
 *   Entrypoint for GSTAT via the services manager
 **********************************************/
	int exit_code = FINI_OK;

	try {
		exit_code = gstat(uSvc);
	}
	catch (const Firebird::Exception& e)
	{
		ISC_STATUS_ARRAY status;
		e.stuff_exception(status);
		uSvc->initStatus();
		uSvc->setServiceStatus(status);
		exit_code = FB_FAILURE;
	}

	return exit_code;
}


int gstat(Firebird::UtilSvc* uSvc)
{
   struct isc_4_struct {
          ISC_QUAD isc_5;	/* RDB$DESCRIPTOR */
          short isc_6;	/* isc_utility */
          short isc_7;	/* RDB$FORMAT */
   } isc_4;
   struct isc_2_struct {
          short isc_3;	/* RDB$RELATION_ID */
   } isc_2;
   struct isc_11_struct {
          char  isc_12 [32];	/* RDB$INDEX_NAME */
          short isc_13;	/* isc_utility */
          short isc_14;	/* RDB$INDEX_ID */
          short isc_15;	/* RDB$INDEX_INACTIVE */
   } isc_11;
   struct isc_9_struct {
          char  isc_10 [32];	/* RDB$RELATION_NAME */
   } isc_9;
   struct isc_19_struct {
          ISC_LONG isc_20;	/* RDB$PAGE_NUMBER */
          short isc_21;	/* isc_utility */
          short isc_22;	/* RDB$PAGE_TYPE */
   } isc_19;
   struct isc_17_struct {
          short isc_18;	/* RDB$RELATION_ID */
   } isc_17;
   struct isc_24_struct {
          char  isc_25 [32];	/* RDB$RELATION_NAME */
          char  isc_26 [256];	/* RDB$EXTERNAL_FILE */
          ISC_QUAD isc_27;	/* RDB$VIEW_BLR */
          short isc_28;	/* isc_utility */
          short isc_29;	/* RDB$RELATION_ID */
          short isc_30;	/* RDB$RELATION_TYPE */
          short isc_31;	/* gds__null_flag */
          short isc_32;	/* gds__null_flag */
          short isc_33;	/* gds__null_flag */
          short isc_34;	/* RDB$SYSTEM_FLAG */
   } isc_24;
/**************************************
 *
 *	g s t a t
 *
 **************************************
 *
 * Functional description
 *	Gather information from system relations to do analysis
 *	of a database.
 *
 **************************************/
	isc_db_handle db_handle = 0;

	TEXT pass_buff[128], user_buff[128], *password = pass_buff, *username = user_buff;
	MOVE_CLEAR(user_buff, sizeof(user_buff));
	MOVE_CLEAR(pass_buff, sizeof(pass_buff));

	// TEXT tr_buff[128], *tr_user = tr_buff;
	// MOVE_CLEAR(tr_buff, sizeof(tr_buff));
	// bool trusted_role = false;

#ifdef TRUSTED_AUTH
	bool trusted_auth = false;
#endif

	tdba thd_context(uSvc), *tddba;
	tdba::putSpecific(tddba, &thd_context);
	int exit_code = FINI_OK;

	const int argc = uSvc->argv.getCount();
	const char** argv = uSvc->argv.begin();

	ISC_STATUS* status_vector = NULL;

	try {

	tddba->dba_status = tddba->dba_status_vector;
	status_vector = tddba->dba_status;

	if (argc == 1) // no parameters at all.
	{
		dba_error(45);	// use gstat -? to get help'
	}

	bool sw_system = false;
	bool sw_data = false;
	bool sw_index = false;
	bool sw_version = false;
	bool sw_header = false;
	bool sw_enc = false;
	//bool sw_log = false;
	bool sw_record = false;
	bool sw_relation = false;
	bool sw_nocreation = false;

	const Switches switches(dba_in_sw_table, FB_NELEM(dba_in_sw_table), false, true);
	const char* name = NULL;

	const TEXT* const* const end = argv + argc;
	++argv;
	while (argv < end)
	{
		const TEXT* str = *argv++;
		if (*str != '-')
		{
			if (name)
			{
				dba_error(40);	// database name was already specified
			}

			name = str;
			continue;
		}

		const Switches::in_sw_tab_t* in_sw_tab = switches.findSwitch(str);
		if (!in_sw_tab)
		{
			if (!str[1])
				str = "-*NONE*";
			dba_print(true, 20, SafeArg() << (str + 1));	// msg 20: unknown switch "%s"
			print_help();
			dba_error(1);	// msg 1: found unknown switch
			break; // redundant
		}

		switch (in_sw_tab->in_sw)
		{
		case IN_SW_DBA_HELP:
			print_help();
			dba_exit(FINI_OK, tddba);
			break;
		case IN_SW_DBA_USERNAME:
			if (argv < end)
				fb_utils::copy_terminate(username, *argv++, sizeof(user_buff));
			break;
		case IN_SW_DBA_PASSWORD:
			if (argv < end)
			{
				uSvc->hidePasswd(uSvc->argv, argv - uSvc->argv.begin());
				fb_utils::copy_terminate(password, *argv++, sizeof(pass_buff));
			}
			break;
		case IN_SW_DBA_FETCH_PASS:
			if (argv < end)
			{
				const char* passwd = NULL;
				if (fb_utils::fetchPassword(*argv++, passwd) == fb_utils::FETCH_PASS_OK)
				{
					fb_utils::copy_terminate(password, passwd, sizeof(pass_buff));
				}
			}
			break;
		/***
		case IN_SW_DBA_TRUSTED_USER:
			uSvc->checkService();
			if (argv < end)
				fb_utils::copy_terminate(tr_user, *argv++, sizeof(tr_buff));
			break;
		case IN_SW_DBA_TRUSTED_ROLE:
			uSvc->checkService();
			trusted_role = true;
			break;
		***/

#ifdef TRUSTED_AUTH
		case IN_SW_DBA_TRUSTEDAUTH:
			trusted_auth = true;
			break;
#endif
		case IN_SW_DBA_SYSTEM:
			sw_system = true;
			break;
		case IN_SW_DBA_DATA:
			sw_data = true;
			break;
		case IN_SW_DBA_INDEX:
			sw_index = true;
			break;
		case IN_SW_DBA_VERSION:
			sw_version = true;
			break;
		case IN_SW_DBA_HEADER:
			sw_header = true;
			break;
		case IN_SW_DBA_ENCRYPTION:
			sw_enc = true;
			break;
//			case IN_SW_DBA_LOG:
//				sw_log = true;
//				break;
		case IN_SW_DBA_DATAIDX:
			sw_index = sw_data = true;
			break;
		case IN_SW_DBA_RECORD:
			sw_record = true;
			break;
		case IN_SW_DBA_RELATION:
			sw_relation = true;
			while (argv < end && **argv != '-')
			{
				if (strlen(*argv) > MAX_SQL_IDENTIFIER_LEN)
				{
					char tbname[MAX_SQL_IDENTIFIER_LEN + 5];
					fb_utils::copy_terminate(tbname, *argv, MAX_SQL_IDENTIFIER_SIZE);
					memcpy(tbname + MAX_SQL_IDENTIFIER_LEN, "...\0", 4);
					dba_error(42, SafeArg() << tbname);	// option -t got a too long table name @1
					break;
				}

				dba_rel* relation = (dba_rel*) alloc(sizeof(struct dba_rel));
				strcpy(relation->rel_name, *argv++);
				fb_utils::exact_name(relation->rel_name);
				relation->rel_id = -1;
				dba_rel** next = &tddba->relations;
				while (*next) {
					next = &(*next)->rel_next;
				}
				*next = relation;

				// CVC: If the db name wasn't given yet, only one table name can be specified
				// after -t to avoid ambiguity.
				if (!name)
					break;
			}
			break;
		case IN_SW_DBA_NOCREATION:
			sw_nocreation = true;
			break;
		}
	}

	if (sw_version)
		dba_print(false, 5, SafeArg() << FB_VERSION);	// msg 5: gstat version %s

	if (sw_header && (sw_system || sw_data || sw_index || sw_record || sw_relation))
	{
		dba_error(38);	//option -h is incompatible with options -a, -d, -i, -r, -s and -t
	}

	if (sw_enc && (sw_header || sw_system || sw_data || sw_index || sw_record || sw_relation))
	{
		dba_error(50);	//option -e is incompatible with options -a, -d, -h, -i, -r, -s and -t
	}

	if (!name)
	{
		dba_error(2);	// msg 2: please retry, giving a database name
	}

	if (sw_relation && !tddba->relations)
	{
		dba_error(41);	// option -t needs a table name.
	}

	if (!sw_data && !sw_index)
		sw_data = sw_index = true;

	if (sw_record && !sw_data)
		sw_data = true;

	// Open database and go to work

	Firebird::PathName fileName = name;
	const Firebird::PathName connName = fileName;
	Firebird::PathName tempStr;

#ifdef WIN_NT
	if (!ISC_analyze_pclan(fileName, tempStr))
#endif
	{
		if (!ISC_analyze_tcp(fileName, tempStr))
		{
#ifndef NO_NFS
			if (!ISC_analyze_nfs(fileName, tempStr))
#endif
			{
				fileName = connName;
			}
		}
	}

	expandDatabaseName(fileName, tempStr, NULL);
	fileName = tempStr;

	dba_fil* current = db_open(fileName.c_str(), fileName.length());

	SCHAR temp[1024];
	tddba->page_size = sizeof(temp);
	tddba->global_buffer = (pag*) temp;
	tddba->page_number = -1;
	const header_page* header = (const header_page*) db_read((SLONG) 0);

	uSvc->started();

	if (!Ods::isSupported(header->hdr_ods_version, header->hdr_ods_minor))
	{
		const int oversion = (header->hdr_ods_version & ~ODS_FIREBIRD_FLAG);
		dba_error(3, SafeArg() << ODS_VERSION << oversion);
		// msg 3: Wrong ODS version, expected %d, encountered %d?
	}

	char file_name[1024];
	fileName.copyTo(file_name, sizeof(file_name));

	dba_print(false, 6, SafeArg() << file_name);	// msg 6: \nDatabase \"%s\"\n

	tddba->page_size = header->hdr_page_size;
	tddba->dp_per_pp = Ods::dataPagesPerPP(tddba->page_size);
	tddba->max_records = Ods::maxRecsPerDP(tddba->page_size);
	tddba->buffer1 = (pag*) alloc(tddba->page_size);
	tddba->buffer2 = (pag*) alloc(tddba->page_size);
	tddba->global_buffer = (pag*) alloc(tddba->page_size);
	tddba->page_number = -1;

	// gather continuation files

	ULONG page = HEADER_PAGE;
	do {
		if (page != HEADER_PAGE)
			current = db_open(file_name, strlen(file_name));
		do {
			header = (const header_page*) db_read(page);
			if (current != tddba->files)
				current->fil_fudge = 1;	// ignore header page once read it
			*file_name = '\0';
			const UCHAR* vp = header->hdr_data;
			for (const UCHAR* const vend = vp + header->hdr_page_size;
				 vp < vend && *vp != HDR_end; vp += 2 + vp[1])
			{
				if (*vp == HDR_file)
				{
					memcpy(file_name, vp + 2, vp[1]);
					*(file_name + vp[1]) = '\0';
				}
				if (*vp == HDR_last_page) {
					memcpy(&current->fil_max_page, vp + 2, sizeof(current->fil_max_page));
				}
			}
		} while (page = header->hdr_next_page);
		page = current->fil_max_page + 1;	// first page of next file
	} while (*file_name);

	// Print header page

	page = HEADER_PAGE;
	do {
		header = (const header_page*) db_read(page);
		PPG_print_header(header, page, sw_nocreation, uSvc);
		page = header->hdr_next_page;
	} while (page);

	if (sw_header)
		dba_exit(FINI_OK, tddba);

	if (sw_enc)
	{
		class Statist
		{
		public:
			Statist()
				: enc(0), non(0)
			{ }

			void print(USHORT messageNo)
			{
				dba_print(false, messageNo, SafeArg() << enc + non << enc << non);
				// msg 5[2-4]: <TYPE> pages: total @1, encrypted @2, non-crypted @3
			}

			void log(UCHAR flags)
			{
				if (flags & crypted_page)
					++enc;
				else
					++non;
			}

		private:
			ULONG enc, non;
		};
		Statist data, index, blob;

		for (page = 0; true; ++page)
		{
			const pag* p = db_read(page, true);
			if (!p)
			{
				break;
			}

			switch(p->pag_type)
			{
			case pag_data:
				data.log(p->pag_flags);
				break;
			case pag_index:
				index.log(p->pag_flags);
				break;
			case pag_blob:
				blob.log(p->pag_flags);
				break;
			}
		}

		uSvc->printf(false, "\n");
		data.print(52);
		index.print(53);
		blob.print(54);

		dba_exit(FINI_OK, tddba);
	}

	// print continuation file sequence

	dba_print(false, 7);
	// msg 7: \n\nDatabase file sequence:
	for (current = tddba->files; current->fil_next; current = current->fil_next)
	{
		dba_print(false, 8, SafeArg() << current->fil_string << current->fil_next->fil_string);
		// msg 8: File %s continues as file %s
	}
	dba_print(false, 9, SafeArg() << current->fil_string << ((current == tddba->files) ? "only" : "last"));
	// msg 9: File %s is the %s file\n

	// Check to make sure that the user accessing the database is either
	// SYSDBA or owner of the database

	Firebird::ClumpletWriter dpb(Firebird::ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);
	uSvc->fillDpb(dpb);
	dpb.insertTag(isc_dpb_gstat_attach);
	dpb.insertTag(isc_dpb_no_garbage_collect);

	const unsigned char* authBlock;
	unsigned int authBlockSize = uSvc->getAuthBlock(&authBlock);
	if (authBlockSize)
	{
		dpb.insertBytes(isc_dpb_auth_block, authBlock, authBlockSize);
	}
	else
	{
		if (*username)
			dpb.insertString(isc_dpb_user_name, username, strlen(username));

		if (*password)
		{
			dpb.insertString(uSvc->isService() ? isc_dpb_password_enc : isc_dpb_password,
							password, strlen(password));
		}

		/***
		if (*tr_user)
		{
			uSvc->checkService();
			dpb.insertString(isc_dpb_trusted_auth, tr_user, strlen(tr_user));
			if (trusted_role)
				dpb.insertString(isc_dpb_trusted_role, ADMIN_ROLE, strlen(ADMIN_ROLE));
		}
		***/

#ifdef TRUSTED_AUTH
		if (trusted_auth)
			dpb.insertTag(isc_dpb_trusted_auth);
#endif
	}

	isc_attach_database(status_vector, 0, connName.c_str(), &DB, dpb.getBufferLength(),
						reinterpret_cast<const char*>(dpb.getBuffer()));
	if (status_vector[1])
		dba_exit(FINI_ERROR, tddba);

	if (sw_version)
		isc_version(&DB, NULL, NULL);

	isc_tr_handle transact1 = 0;
	/*START_TRANSACTION transact1 READ_ONLY;*/
	{
	isc_start_transaction (isc_status, (FB_API_HANDLE*) &transact1, (short) 1, &DB, (short) 4, isc_tpb_0);;
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		dba_exit(FINI_ERROR, tddba);
	/*END_ERROR*/
	   }
	}

	isc_req_handle request1 = 0;
	isc_req_handle request2 = 0;
	isc_req_handle request3 = 0;
	isc_req_handle request4 = 0;

	/*FOR(TRANSACTION_HANDLE transact1 REQUEST_HANDLE request1)
		X IN RDB$RELATIONS SORTED BY DESC X.RDB$RELATION_NAME*/
	{
        if (!request1)
           isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request1, (short) sizeof(isc_23), (char*) isc_23);
	if (request1)
           isc_start_request (isc_status, (FB_API_HANDLE*) &request1, (FB_API_HANDLE*) &transact1, (short) 0);
	if (!isc_status [1]) {
	while (1)
	   {
           isc_receive (isc_status, (FB_API_HANDLE*) &request1, (short) 0, (short) 310, &isc_24, (short) 0);
	   if (!isc_24.isc_28 || isc_status [1]) break;

		if (!sw_system && /*X.RDB$SYSTEM_FLAG*/
				  isc_24.isc_34) {
		  continue;
		}
		if (!/*X.RDB$VIEW_BLR.NULL*/
		     isc_24.isc_33 || !/*X.RDB$EXTERNAL_FILE.NULL*/
     isc_24.isc_32) {
			continue;
		}
		//rel_virtual, rel_global_temp_preserve, rel_global_temp_delete
		if (!/*X.RDB$SYSTEM_FLAG.NULL*/
		     isc_24.isc_31 && /*X.RDB$SYSTEM_FLAG*/
    isc_24.isc_34 == fb_sysflag_system &&
			/*X.RDB$RELATION_TYPE*/
			isc_24.isc_30 >= rel_virtual && /*X.RDB$RELATION_TYPE*/
		   isc_24.isc_30 <= rel_global_temp_delete)
		{
			continue;
		}

		dba_rel* relation;
		if (sw_relation)
		{
			fb_utils::exact_name(/*X.RDB$RELATION_NAME*/
					     isc_24.isc_25);
			for (relation = tddba->relations; relation; relation = relation->rel_next)
			{
                if (!(strcmp(relation->rel_name, /*X.RDB$RELATION_NAME*/
						 isc_24.isc_25)))
                {
					relation->rel_id = /*X.RDB$RELATION_ID*/
							   isc_24.isc_29;
					break;
                }
			}
			if (!relation)
				continue;
		}
		else
		{
			relation = (dba_rel*) alloc(sizeof(struct dba_rel));
			relation->rel_next = tddba->relations;
			tddba->relations = relation;
			relation->rel_id = /*X.RDB$RELATION_ID*/
					   isc_24.isc_29;
			strcpy(relation->rel_name, /*X.RDB$RELATION_NAME*/
						   isc_24.isc_25);
			fb_utils::exact_name(relation->rel_name);
		}

		/*FOR(TRANSACTION_HANDLE transact1 REQUEST_HANDLE request2)
			Y IN RDB$PAGES WITH Y.RDB$RELATION_ID EQ relation->rel_id AND
				Y.RDB$PAGE_SEQUENCE EQ 0*/
		{
                if (!request2)
                   isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request2, (short) sizeof(isc_16), (char*) isc_16);
		isc_17.isc_18 = relation->rel_id;
		if (request2)
                   isc_start_and_send (isc_status, (FB_API_HANDLE*) &request2, (FB_API_HANDLE*) &transact1, (short) 0, (short) 2, &isc_17, (short) 0);
		if (!isc_status [1]) {
		while (1)
		   {
                   isc_receive (isc_status, (FB_API_HANDLE*) &request2, (short) 1, (short) 8, &isc_19, (short) 0);
		   if (!isc_19.isc_21 || isc_status [1]) break;

			if (/*Y.RDB$PAGE_TYPE*/
			    isc_19.isc_22 == pag_pointer) {
				relation->rel_pointer_page = /*Y.RDB$PAGE_NUMBER*/
							     isc_19.isc_20;
			}
            if (/*Y.RDB$PAGE_TYPE*/
		isc_19.isc_22 == pag_root) {
				relation->rel_index_root = /*Y.RDB$PAGE_NUMBER*/
							   isc_19.isc_20;
			}
		/*END_FOR;*/
		   }
		   };
		/*ON_ERROR*/
		if (isc_status [1])
		   {
			dba_exit(FINI_ERROR, tddba);
		/*END_ERROR*/
		   }
		}

		if (sw_index)
		{
			/*FOR(TRANSACTION_HANDLE transact1 REQUEST_HANDLE request3)
				Y IN RDB$INDICES WITH Y.RDB$RELATION_NAME EQ relation->rel_name
					SORTED BY DESC Y.RDB$INDEX_NAME*/
			{
                        if (!request3)
                           isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request3, (short) sizeof(isc_8), (char*) isc_8);
			isc_vtov ((const char*) relation->rel_name, (char*) isc_9.isc_10, 32);
			if (request3)
                           isc_start_and_send (isc_status, (FB_API_HANDLE*) &request3, (FB_API_HANDLE*) &transact1, (short) 0, (short) 32, &isc_9, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, (FB_API_HANDLE*) &request3, (short) 1, (short) 38, &isc_11, (short) 0);
			   if (!isc_11.isc_13 || isc_status [1]) break;
	            if (/*Y.RDB$INDEX_INACTIVE*/
			isc_11.isc_15)
					  continue;
				dba_idx* index = (dba_idx*) alloc(sizeof(struct dba_idx));
				index->idx_next = relation->rel_indexes;
				relation->rel_indexes = index;
				strcpy(index->idx_name, /*Y.RDB$INDEX_NAME*/
							isc_11.isc_12);
				fb_utils::exact_name(index->idx_name);
				index->idx_id = /*Y.RDB$INDEX_ID*/
						isc_11.isc_14 - 1;
	        /*END_FOR;*/
		   }
		   };
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				dba_exit(FINI_ERROR, tddba);
			/*END_ERROR*/
			   }
			}
		}

		if (sw_record)
		{
			/*FOR(TRANSACTION_HANDLE transact1 REQUEST_HANDLE request4)
				Y IN RDB$FORMATS WITH Y.RDB$RELATION_ID EQ X.RDB$RELATION_ID
					SORTED BY Y.RDB$FORMAT*/
			{
                        if (!request4)
                           isc_compile_request (isc_status, (FB_API_HANDLE*) &DB, (FB_API_HANDLE*) &request4, (short) sizeof(isc_1), (char*) isc_1);
			isc_2.isc_3 = isc_24.isc_29;
			if (request4)
                           isc_start_and_send (isc_status, (FB_API_HANDLE*) &request4, (FB_API_HANDLE*) &transact1, (short) 0, (short) 2, &isc_2, (short) 0);
			if (!isc_status [1]) {
			while (1)
			   {
                           isc_receive (isc_status, (FB_API_HANDLE*) &request4, (short) 1, (short) 12, &isc_4, (short) 0);
			   if (!isc_4.isc_6 || isc_status [1]) break;
				dba_fmt* format = (dba_fmt*) alloc(sizeof(struct dba_fmt));
				format->fmt_next = relation->rel_formats;
				relation->rel_formats = format;
				format->fmt_number = /*Y.RDB$FORMAT*/
						     isc_4.isc_7;
				format->fmt_length = get_format_length(status_vector, DB, transact1, /*Y.RDB$DESCRIPTOR*/
												     isc_4.isc_5);
				if (!format->fmt_length)
				{
					dba_exit(FINI_ERROR, tddba);
				}
				format->fmt_used = false;
				++relation->rel_total_formats;
			/*END_FOR;*/
			   }
			   };
			/*ON_ERROR*/
			if (isc_status [1])
			   {
				dba_exit(FINI_ERROR, tddba);
			/*END_ERROR*/
			   }
			}
		}
	/*END_FOR;*/
	   }
	   };
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		dba_exit(FINI_ERROR, tddba);
	/*END_ERROR*/
	   }
	}

	if (request1) {
		isc_release_request(status_vector, &request1);
	}
	if (request2) {
		isc_release_request(status_vector, &request2);
	}
	if (request3) {
		isc_release_request(status_vector, &request3);
	}
	if (request4) {
		isc_release_request(status_vector, &request4);
	}

	/*COMMIT transact1;*/
	{
	isc_commit_transaction (isc_status, (FB_API_HANDLE*) &transact1);;
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		dba_exit(FINI_ERROR, tddba);
	/*END_ERROR*/
	   }
	}

	if (!tddba->uSvc->isService())
	{
		// Now it's time to take care ourselves about shutdown handling
		if (fb_shutdown_callback(status_vector, gstatShutdown, fb_shut_confirmation, NULL))
		{
			dba_exit(FINI_ERROR, tddba);
		}
	}

	// FINISH; error!
	/*FINISH*/
	{
	if (DB)
	   isc_detach_database (isc_status, &DB);;
	/*ON_ERROR*/
	if (isc_status [1])
	   {
		dba_exit(FINI_ERROR, tddba);
	/*END_ERROR*/
	   }
	}

	if (sw_relation)
	{
		for (const dba_rel* relation = tddba->relations; relation; relation = relation->rel_next)
		{
			checkForShutdown(tddba);
			if (relation->rel_id == -1)
			{
				dba_error(44, SafeArg() << relation->rel_name);
				// msg 44: table "@1" not found
			}
		}
	}


	dba_print(false, 10);
	// msg 10: \nAnalyzing database pages ...\n

	for (dba_rel* relation = tddba->relations; relation; relation = relation->rel_next)
	{
		checkForShutdown(tddba);

		// This condition should never happen because relations not found cause an error before.
		if (relation->rel_id == -1)
		{
			fb_assert(sw_relation && relation->rel_id >= 0);
			continue;
		}

		if (sw_data) {
			analyze_data(relation, sw_record);
		}
		for (dba_idx* index = relation->rel_indexes; index; index = index->idx_next)
		{
			checkForShutdown(tddba);
			analyze_index(relation, index);
		}
	}

	// Print results

	UCHAR buf[BUFFER_SMALL], buf2[BUFFER_SMALL];

	for (const dba_rel* relation = tddba->relations; relation; relation = relation->rel_next)
	{
		if (relation->rel_id == -1) {
			continue;
		}

		uSvc->printf(false, "%s (%d)\n", relation->rel_name, relation->rel_id);

		if (sw_data)
		{
			dba_print(false, 11, SafeArg() << relation->rel_pointer_page << relation->rel_index_root);
			// msg 11: "    Primary pointer page: %ld, Index root page: %ld"

			if (sw_record)
			{
				uSvc->printf(false, "    Total formats: %d, used formats: %d\n",
							 relation->rel_total_formats, relation->rel_used_formats);

				double average = relation->rel_records ?
					(double) relation->rel_record_space / relation->rel_records : 0.0;
				sprintf((char*) buf, "%.2f", average);
				uSvc->printf(false, "    Average record length: %s, total records: %" UQUADFORMAT "\n",
							 buf, relation->rel_records);
				// dba_print(false, 18, SafeArg() << buf << relation->rel_records);
				// msg 18: "    Average record length: %s, total records: %ld

				average = relation->rel_versions ?
					(double) relation->rel_version_space / relation->rel_versions : 0.0;
				sprintf((char*) buf, "%.2f", average);
				uSvc->printf(false, "    Average version length: %s, total versions: %" UQUADFORMAT ", max versions: %" UQUADFORMAT "\n",
							 buf, relation->rel_versions, relation->rel_max_versions);
				// dba_print(false, 19, SafeArg() << buf << relation->rel_versions <<
				//			 relation->rel_max_versions);
				// msg 19: " Average version length: %s, total versions: %ld, max versions: %ld

				average = relation->rel_fragments ?
					(double) relation->rel_fragment_space / relation->rel_fragments : 0.0;
				sprintf((char*) buf, "%.2f", average);
				uSvc->printf(false, "    Average fragment length: %s, total fragments: %" UQUADFORMAT ", max fragments: %" UQUADFORMAT "\n",
							 buf, relation->rel_fragments, relation->rel_max_fragments);

				average = relation->rel_records ?
					(double) relation->rel_format_space / relation->rel_records : 0.0;
				sprintf((char*) buf, "%.2f", average);
				average = relation->rel_record_space ?
					(double) relation->rel_format_space / relation->rel_record_space : 0.0;
				sprintf((char*) buf2, "%.2f", average);
				uSvc->printf(false, "    Average unpacked length: %s, compression ratio: %s\n",
							 buf, buf2);

			}

			uSvc->printf(false, "    Pointer pages: %ld, data page slots: %ld\n",
						 relation->rel_pointer_pages, relation->rel_slots);

			const double average = (relation->rel_data_pages) ?
				(double) relation->rel_total_space * 100 /
				((double) relation->rel_data_pages * (tddba->page_size - DPG_SIZE)) : 0.0;
			sprintf((char*) buf, "%.0f%%", average);
			//dba_print(false, 12, SafeArg() << relation->rel_data_pages << relation->rel_slots << buf);
			// msg 12: "    Data pages: %ld, data page slots: %ld, average fill: %s
			uSvc->printf(false, "    Data pages: %ld, average fill: %s\n", relation->rel_data_pages, buf);

			dba_print(false, 46, SafeArg() << relation->rel_primary_pages << relation->rel_full_pages <<
											  relation->rel_swept_pages);
			// msg 46: "    Primary pages: @1, full pages: @2, swept pages: @3

			if (relation->rel_bigrec_pages)
			{
				dba_print(false, 47, SafeArg() << relation->rel_bigrec_pages);
				// msg 47: "    Big record pages: @1
			}

			if (relation->rel_blobs_level_0 || relation->rel_blobs_level_1 || relation->rel_blobs_level_2)
			{
				dba_print(false, 48, SafeArg() << (relation->rel_blobs_level_0 + relation->rel_blobs_level_1 +
												  relation->rel_blobs_level_2) << relation->rel_blob_space <<
												  relation->rel_blob_pages);
				// msg 48: "    Blobs: @1, total length: @2, blob pages: @3

				dba_print(false, 49, SafeArg() << relation->rel_blobs_level_0 << relation->rel_blobs_level_1 <<
												 relation->rel_blobs_level_2);
				// msg 49: "		Level 0: @1, Level 1: @2, Level 2: @3
			}

			dba_print(false, 13);	// msg 13: "    Fill distribution:"
			print_distribution("\t", relation->rel_fill_distribution);
		}
		uSvc->printf(false, "\n");

		for (const dba_idx* index = relation->rel_indexes; index; index = index->idx_next)
		{
			dba_print(false, 14, SafeArg() << index->idx_name << index->idx_id);
			// msg 14: "    Index %s (%d)"
			//dba_print(false, 15, SafeArg() << index->idx_depth << index->idx_leaf_buckets << index->idx_nodes);
			// msg 15: \tDepth: %d, leaf buckets: %ld, nodes: %ld
			uSvc->printf(false, "\tRoot page: %d, depth: %d, leaf buckets: %ld, nodes: %" UQUADFORMAT "\n",
						 index->idx_root, index->idx_depth, index->idx_leaf_buckets, index->idx_nodes);
			double average = (index->idx_nodes) ?
				(double) index->idx_total_length / index->idx_nodes : 0.0;
			sprintf((char*) buf, "%.2f", average);
			// dba_print(false, 16, SafeArg() << buf << index->idx_total_duplicates << index->idx_max_duplicates);
			// msg 16: \tAverage data length: %s, total dup: %ld, max dup: %ld"
			uSvc->printf(false, "\tAverage node length: %s, total dup: %" UQUADFORMAT ", max dup: %" UQUADFORMAT "\n",
						 buf, index->idx_total_duplicates, index->idx_max_duplicates);
			average = (index->idx_nodes) ?
				(double) index->idx_packed_length / index->idx_nodes : 0.0;
			sprintf((char*) buf, "%.2f", average);
			average = index->idx_packed_length ?
				(double) index->idx_unpacked_length / index->idx_packed_length : 0.0;
			sprintf((char*) buf2, "%.2f", average);
			uSvc->printf(false, "\tAverage key length: %s, compression ratio: %s\n", buf, buf2);
			average = (index->idx_nodes) ?
				(double) index->idx_prefix_length / index->idx_nodes : 0.0;
			sprintf((char*) buf, "%.2f", average);
			average = (index->idx_nodes) ?
				(double) index->idx_data_length / index->idx_nodes : 0.0;
			sprintf((char*) buf2, "%.2f", average);
			uSvc->printf(false, "\tAverage prefix length: %s, average data length: %s\n", buf, buf2);
			average = (index->idx_nodes) ?
				(double) index->idx_diff_pages / index->idx_nodes : 0.0;
			sprintf((char*) buf, "%.2f", average);
			uSvc->printf(false, "\tClustering factor: %" UQUADFORMAT ", ratio: %s\n",
						 index->idx_diff_pages, buf);
			dba_print(false, 17);
			// msg 17: \tFill distribution:
			print_distribution("\t    ", index->idx_fill_distribution);
			uSvc->printf(false, "\n");
		}
	}

	dba_exit(FINI_OK, tddba);

	}	// try
	catch (const Firebird::LongJump&)
	{
		// do nothing
	}
	catch (const Firebird::Exception& ex)
	{
		ex.stuff_exception(status_vector);
	}

	// free mem

	if (status_vector[1])
	{
		tddba->uSvc->setServiceStatus(status_vector);
		if (!tddba->uSvc->isService())
		{
			const ISC_STATUS* vector = status_vector;
			SCHAR s[1024];
			if (fb_interpret(s, sizeof(s), &vector))
			{
				tddba->uSvc->printf(true, "\n%s\n", s);
				s[0] = '-';
				while (fb_interpret(s + 1, sizeof(s) - 1, &vector))
					tddba->uSvc->printf(true, "%s\n", s);
			}
		}
	}

	// if there still exists a database handle, disconnect from the server
	// This is isc_detach_database
	/*FINISH*/
	{
	if (DB)
	   isc_detach_database (NULL, &DB);
	};

	uSvc->started();
	dba_mem* alloced = tddba->head_of_mem_list;
	while (alloced != 0)
	{
		delete[] alloced->memory;
		alloced = alloced->mem_next;
	}

	// close files
	open_files* open_file = tddba->head_of_files_list;
	while (open_file)
	{
		db_close(open_file->desc);
		open_file = open_file->open_files_next;
	}

	// free linked lists
	while (tddba->head_of_files_list != 0)
	{
		open_files* tmp1 = tddba->head_of_files_list;
		tddba->head_of_files_list = tddba->head_of_files_list->open_files_next;
		delete tmp1;
	}

	while (tddba->head_of_mem_list != 0)
	{
		dba_mem* tmp2 = tddba->head_of_mem_list;
		tddba->head_of_mem_list = tddba->head_of_mem_list->mem_next;
		delete tmp2;
	}

	exit_code = tddba->exit_code;
	tdba::restoreSpecific();

	if ((exit_code != FINI_OK) && uSvc->isService() && tddba->dba_status[1])
	{
		uSvc->initStatus();
		uSvc->setServiceStatus(tddba->dba_status);
	}

	return exit_code;
}


static char* alloc(size_t size)
{
/**************************************
 *
 *	a l l o c
 *
 **************************************
 *
 * Functional description
 *	Allocate and zero a piece of memory.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();
	char* const block = FB_NEW(*getDefaultMemoryPool()) SCHAR[size];

	if (!block)
	{
		// NOMEM: return error
		dba_error(31);
	}

	memset(block, 0, size);

	dba_mem* mem_list = FB_NEW(*getDefaultMemoryPool()) dba_mem;
	if (!mem_list)
	{
		// NOMEM: return error
		dba_error(31);
	}
	mem_list->memory = block;
	mem_list->mem_next = 0;

	if (tddba->head_of_mem_list == 0)
		tddba->head_of_mem_list = mem_list;
	else
	{
		mem_list->mem_next = tddba->head_of_mem_list;
		tddba->head_of_mem_list = mem_list;
	}

	return block;
}


static void analyze_data( dba_rel* relation, bool sw_record)
{
/**************************************
 *
 *	a n a l y z e _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Analyze data pages associated with relation.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();

	pointer_page* ptr_page = (pointer_page*) tddba->buffer1;

	for (SLONG next_pp = relation->rel_pointer_page; next_pp; next_pp = ptr_page->ppg_next)
	{
		++relation->rel_pointer_pages;
		memcpy(ptr_page, (const SCHAR*) db_read(next_pp), tddba->page_size);
		const ULONG* ptr = ptr_page->ppg_page;
		for (const ULONG* const end = ptr + ptr_page->ppg_count; ptr < end; ptr++)
		{
			++relation->rel_slots;
			if (*ptr)
			{
				++relation->rel_data_pages;
				if (!analyze_data_page(relation, (const data_page*) db_read(*ptr), sw_record))
				{
					dba_print(false, 18, SafeArg() << *ptr);
					// msg 18: "    Expected data on page %ld"
				}
			}
		}
	}

	if (sw_record)
	{
		for (const dba_fmt* format = relation->rel_formats; format; format = format->fmt_next)
		{
			if (format->fmt_used)
			{
				++relation->rel_used_formats;
			}
		}
	}
}


static bool analyze_data_page( dba_rel* relation, const data_page* page, bool sw_record)
{
/**************************************
 *
 *	a n a l y z e _ d a t a _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Analyze space utilization for data page.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();

	if (page->dpg_header.pag_type != pag_data)
		return false;

	if (sw_record)
	{
		memcpy(tddba->buffer2, (const SCHAR*) page, tddba->page_size);
		page = (const data_page*) tddba->buffer2;
	}

	USHORT space = page->dpg_count * sizeof(data_page::dpg_repeat);

	const data_page::dpg_repeat* tail = page->dpg_rpt;
	for (const data_page::dpg_repeat* const end = tail + page->dpg_count; tail < end; tail++)
	{
		if (tail->dpg_offset && tail->dpg_length)
		{
			space += tail->dpg_length;

			if (sw_record)
			{
				const rhdf* header = (const rhdf*) ((SCHAR *) page + tail->dpg_offset);
				if (!(header->rhdf_flags & (rhd_blob | rhd_chain | rhd_fragment)))
				{
					++relation->rel_records;
					relation->rel_record_space += tail->dpg_length;

					for (dba_fmt* format = relation->rel_formats; format; format = format->fmt_next)
					{
						if (format->fmt_number == header->rhdf_format)
						{
							relation->rel_format_space += format->fmt_length;
							format->fmt_used = true;
							break;
						}
					}

					if (header->rhdf_flags & rhd_incomplete)
					{
						relation->rel_record_space -= RHDF_SIZE;
						relation->rel_record_space += analyze_fragments(relation, header);
					}
					else
					{
						relation->rel_record_space -= RHD_SIZE;
					}

					if (header->rhdf_b_page)
					{
						relation->rel_version_space += analyze_versions(relation, header);
					}
				}

				if (header->rhdf_flags & rhd_blob)
				{
					analyze_blob(relation, (blh*)header, tail->dpg_length);
				}
			}
		}
	}

	relation->rel_total_space += space;
	SSHORT bucket = (space * BUCKETS) / (tddba->page_size - DPG_SIZE);

	if (bucket == BUCKETS)
		--bucket;

	if (page->dpg_header.pag_flags & dpg_full)
		++relation->rel_full_pages;
	if (!(page->dpg_header.pag_flags & dpg_secondary))
		++relation->rel_primary_pages;
	if (page->dpg_header.pag_flags & dpg_swept)
		++relation->rel_swept_pages;
	++relation->rel_fill_distribution[bucket];

	return true;
}


static void analyze_blob(dba_rel* relation, const blh* blob, int length)
{
	relation->rel_blob_space += blob->blh_length;
	if (!blob->blh_level)
	{
		relation->rel_blobs_level_0++;
	}
	else
	{
		const int slots = (length - BLH_SIZE) / sizeof(SLONG);
		relation->rel_blob_pages += slots;
		if (blob->blh_level == 1)
		{
			relation->rel_blobs_level_1++;
		}
		else
		{
			relation->rel_blobs_level_2++;

			SLONG pages[MAX_PAGE_SIZE / sizeof(SLONG)];
			memcpy(pages, blob->blh_page, slots * sizeof(SLONG));

			for (int i = 0; i < slots; i++)
			{
				const blob_page* bpage = (const blob_page*) db_read(pages[i]);
				relation->rel_blob_pages += bpage->blp_length / sizeof(SLONG);
			}
		}
	}
}


static ULONG analyze_fragments(dba_rel* relation, const rhdf* header)
{
/**************************************
 *
 *	a n a l y z e _ f r a g m e n t s
 *
 **************************************
 *
 * Functional description
 *	Analyze space used by a record's fragments.
 *
 **************************************/
	//tdba* tddba = tdba::getSpecific();
	ULONG space = 0, fragments = 0;

	while (header->rhdf_flags & rhd_incomplete)
	{
		const SLONG f_page = header->rhdf_f_page;
		const USHORT f_line = header->rhdf_f_line;
		const data_page* page = (const data_page*) db_read(f_page);
		if (page->dpg_header.pag_type != pag_data || page->dpg_relation != relation->rel_id ||
			page->dpg_count <= f_line)
		{
			break;
		}

		if ((page->dpg_header.pag_flags & (dpg_orphan | dpg_full)) && page->dpg_count == 1) {
			relation->rel_bigrec_pages++;
		}

		const data_page::dpg_repeat* index = &page->dpg_rpt[f_line];
		if (!index->dpg_offset)
			break;
		space += index->dpg_length;
		space -= RHDF_SIZE;
		header = (const rhdf*) ((SCHAR *) page + index->dpg_offset);

		fragments++;
	}

	relation->rel_fragments += fragments;
	relation->rel_fragment_space += space;
	if (fragments > relation->rel_max_fragments) {
		relation->rel_max_fragments = fragments;
	}

	return space;
}


static void analyze_index( const dba_rel* relation, dba_idx* index)
{
/**************************************
 *
 *	a n a l y z e _ i n d e x
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();

	const index_root_page* index_root = (const index_root_page*) db_read(relation->rel_index_root);

	SLONG page;
	if (index_root->irt_count <= index->idx_id ||
		!(page = index_root->irt_rpt[index->idx_id].irt_root))
	{
		return;
	}

	// CVC: The two const_cast's for bucket can go away if BTreeNode's functions
	// are overloaded for constness. They don't modify bucket and pointer's contents.
	const btree_page* bucket = (const btree_page*) db_read(page);
	index->idx_root = page;
	index->idx_depth = bucket->btr_level + 1;

	UCHAR* pointer;
	IndexNode node;
	while (bucket->btr_level)
	{
		pointer = const_cast<UCHAR*>(bucket->btr_nodes) + bucket->btr_jump_size;
		node.readNode(pointer, false);
		bucket = (const btree_page*) db_read(node.pageNumber);
	}

	bool firstLeafNode = true;
	SLONG number;
	FB_UINT64 duplicates = 0;
	// AB: In fact length for KEY should be MAX_KEY (1/4 of used page-size)
	// This value should be kept equal with size declared in btr.h
	//UCHAR key[4096];
	UCHAR* key = (UCHAR*) alloc(tddba->page_size / 4);
	USHORT key_length = 0;
	ULONG prior_pagno = MAX_ULONG;
	while (true)
	{
		++index->idx_leaf_buckets;
		pointer = const_cast<UCHAR*>(bucket->btr_nodes) + bucket->btr_jump_size;
		const UCHAR* const firstNode = pointer;
		while (true)
		{
			pointer = node.readNode(pointer, true);

			if (node.isEndBucket || node.isEndLevel) {
				break;
			}

			++index->idx_nodes;
			index->idx_total_length += pointer - node.nodePointer;
			index->idx_prefix_length += node.prefix;
			index->idx_data_length += node.length;

			size_t specials = 1;
			if (node.prefix > 127)
				specials += 2;
			else if (node.prefix > 0)
				specials += 1;
			if (node.length > 127)
				specials += 2;
			else if (node.length > 1)
				specials += 1;
			index->idx_packed_length += specials + node.length;

			USHORT pp_sequence;
			SSHORT slot, line;
			node.recordNumber.decompose(tddba->max_records, tddba->dp_per_pp, line, slot, pp_sequence);
			const ULONG pagno = pp_sequence * tddba->dp_per_pp + slot;
			if (pagno != prior_pagno)
				++index->idx_diff_pages;
			prior_pagno = pagno;

			const USHORT l = node.length + node.prefix;
			index->idx_unpacked_length += l;

			bool dup;
			if (node.nodePointer == firstNode) {
				dup = node.keyEqual(key_length, key);
			}
			else {
				dup = (!node.length) && (l == key_length);
			}
			if (firstLeafNode)
			{
				dup = false;
				firstLeafNode = false;
			}
			if (dup)
			{
				++index->idx_total_duplicates;
				++duplicates;
			}
			else
			{
				if (duplicates > index->idx_max_duplicates) {
					index->idx_max_duplicates = duplicates;
				}
				duplicates = 0;
			}

			key_length = l;
			if (node.length) {
				memcpy(key + node.prefix, node.data, node.length);
			}
		}

		if (duplicates > index->idx_max_duplicates) {
			index->idx_max_duplicates = duplicates;
		}

		const USHORT header = (USHORT)(firstNode - (UCHAR*) bucket);
		const USHORT space = bucket->btr_length - header;
		USHORT n = (space * BUCKETS) / (tddba->page_size - header);
		if (n == BUCKETS) {
			--n;
		}
		++index->idx_fill_distribution[n];

		if (node.isEndLevel) {
			break;
		}
		number = page;
		page = bucket->btr_sibling;
		bucket = (const btree_page*) db_read(page);
		if (bucket->btr_header.pag_type != pag_index)
		{
			dba_print(false, 19, SafeArg() << page << number);
			// mag 19: "    Expected b-tree bucket on page %ld from %ld"
			break;
		}
	}
}


static ULONG analyze_versions( dba_rel* relation, const rhdf* header)
{
/**************************************
 *
 *	a n a l y z e _ v e r s i o n s
 *
 **************************************
 *
 * Functional description
 *	Analyze space used by a record's back versions.
 *
 **************************************/
	//tdba* tddba = tdba::getSpecific();
	ULONG space = 0, versions = 0;
	SLONG b_page = header->rhdf_b_page;
	USHORT b_line = header->rhdf_b_line;

	while (b_page)
	{
		const data_page* page = (const data_page*) db_read(b_page);
		if (page->dpg_header.pag_type != pag_data || page->dpg_relation != relation->rel_id ||
			page->dpg_count <= b_line)
		{
			break;
		}
		const data_page::dpg_repeat* index = &page->dpg_rpt[b_line];
		if (!index->dpg_offset)
			break;
		space += index->dpg_length;
		++relation->rel_versions;
		++versions;
		header = (const rhdf*) ((SCHAR *) page + index->dpg_offset);
		b_page = header->rhdf_b_page;
		b_line = header->rhdf_b_line;
		if (header->rhdf_flags & rhd_incomplete)
		{
			space -= RHDF_SIZE;
			space += analyze_fragments(relation, header);
		}
		else
			space -= RHD_SIZE;
	}

	if (versions > relation->rel_max_versions)
		relation->rel_max_versions = versions;

	return space;
}


static USHORT get_format_length(ISC_STATUS* status_vector, isc_db_handle database,
	isc_tr_handle transaction, ISC_QUAD& blob_id)
{
	UserBlob blob(status_vector);

	if (!blob.open(database, transaction, blob_id))
	{
		return 0;
	}

	const UCHAR blob_items[] = {isc_info_blob_total_length};
	UCHAR blob_info[BUFFER_TINY];
	if (!blob.getInfo(sizeof(blob_items), blob_items, sizeof(blob_info), blob_info))
	{
		return 0;
	}

	const UCHAR* p = blob_info;
	if (*p++ != isc_info_blob_total_length)
	{
		return 0;
	}

	const USHORT l = isc_vax_integer((const char*) p, 2);
	p += 2;
	const ULONG length = isc_vax_integer((const char*) p, l);

	size_t return_length = 0;
	Firebird::UCharBuffer buffer;
	if (!blob.getData(length, buffer.getBuffer(length), return_length))
	{
		return 0;
	}

	buffer.resize(return_length);

	const USHORT count = buffer[0] | (buffer[1] << 8);
	const Ods::Descriptor* const desc = (Ods::Descriptor*) &buffer[2] + count - 1;
	return desc->dsc_offset + desc->dsc_length;
}


#ifdef WIN_NT
static void db_close(void* file_desc)
{
/**************************************
 *
 *	d b _ c l o s e ( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *    Close an open file
 *
 **************************************/
	CloseHandle(file_desc);
}

static void db_error( SLONG status)
{
/**************************************
 *
 *	d b _ e r r o r		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	TEXT s[128];

	tdba* tddba = tdba::getSpecific();
	tddba->page_number = -1;

	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
						NULL,
						status,
						GetUserDefaultLangID(),
						s,
						sizeof(s),
						NULL) &&
		!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
						NULL,
						status,
						0, // TMN: Fallback to system known language
						s,
						sizeof(s),
						NULL))
	{
		sprintf(s, "unknown Windows NT error %ld", status);
	}

	if (!tddba->uSvc->isService())
	{
		tddba->uSvc->printf(true, "%s\n", s);
	}
	dba_exit(FINI_ERROR, tddba);
}


// CVC: This function was using cast to char* for the first param always
// and the callers had to cast their char*'s to UCHAR*, too. Since the
// real parameter is char* and always the usage is char*, I changed signature.
static dba_fil* db_open(const char* file_name, USHORT file_length)
{
/**************************************
 *
 *	d b _ o p e n		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Open a database file.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();

	dba_fil* fil;

	if (tddba->files)
	{
		for (fil = tddba->files; fil->fil_next; fil = fil->fil_next);
		fil->fil_next = (dba_fil*) alloc(sizeof(dba_fil) + file_length + 1);
		fil->fil_next->fil_min_page = fil->fil_max_page + 1;
		fil = fil->fil_next;
	}
	else
	{
		// empty list
		fil = tddba->files = (dba_fil*) alloc(sizeof(dba_fil) + file_length + 1);
		fil->fil_min_page = 0L;
	}

	fil->fil_next = NULL;
	strcpy(fil->fil_string, file_name);
	fil->fil_length = file_length;
	fil->fil_fudge = 0;
	fil->fil_max_page = 0L;

	fil->fil_desc = CreateFile(	file_name,
								GENERIC_READ,
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								0);

	if (fil->fil_desc  == INVALID_HANDLE_VALUE)
	{
		tddba->uSvc->setServiceStatus(GSTAT_MSG_FAC, 29, SafeArg() << file_name);
		// msg 29: Can't open database file %s
		db_error(GetLastError());
	}

	open_files* file_list = FB_NEW(*getDefaultMemoryPool()) open_files;
	if (!file_list)
	{
		// NOMEM: return error
		dba_error(31);
	}
	file_list->desc = fil->fil_desc;
	file_list->open_files_next = 0;

	if (tddba->head_of_files_list == 0)
		tddba->head_of_files_list = file_list;
	else
	{
		file_list->open_files_next = tddba->head_of_files_list;
		tddba->head_of_files_list = file_list;
	}

	return fil;
}


static const pag* db_read( SLONG page_number, bool ok_enc)
{
/**************************************
 *
 *	d b _ r e a d		( W I N _ N T )
 *
 **************************************
 *
 * Functional description
 *	Read a database page.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();

	if (tddba->uSvc->finished())
		dba_exit(FINI_OK, tddba);

	if (tddba->page_number == page_number)
		return tddba->global_buffer;

	tddba->page_number = page_number;

	dba_fil* fil;
	for (fil = tddba->files; page_number > (SLONG) fil->fil_max_page && fil->fil_next;)
	{
		 fil = fil->fil_next;
	}

	page_number -= fil->fil_min_page - fil->fil_fudge;

	LARGE_INTEGER liOffset;
	liOffset.QuadPart = UInt32x32To64((DWORD) page_number, (DWORD) tddba->page_size);
	if (SetFilePointer(fil->fil_desc, (LONG) liOffset.LowPart, &liOffset.HighPart, FILE_BEGIN) ==
		(DWORD) -1)
	{
		int lastError = GetLastError();
		if (lastError != NO_ERROR)
		{
			tddba->uSvc->setServiceStatus(GSTAT_MSG_FAC, 30, SafeArg());
			// msg 30: Can't read a database page
			db_error(lastError);
		}
	}

	SLONG actual_length;
	if (!ReadFile(	fil->fil_desc,
					tddba->global_buffer,
					tddba->page_size,
					reinterpret_cast<LPDWORD>(&actual_length),
					NULL))
	{
		tddba->uSvc->setServiceStatus(GSTAT_MSG_FAC, 30, SafeArg());
		// msg 30: Can't read a database page
		db_error(GetLastError());
	}
	if (actual_length != tddba->page_size)
	{
		if (ok_enc)
		{
			return NULL;
		}
		dba_error(4);
		// msg 4: Unexpected end of database file.
	}

	if (tddba->global_buffer->pag_flags & Ods::crypted_page && !ok_enc)
	{
		dba_error(55);
	}

	return tddba->global_buffer;
}
#endif // ifdef WIN_NT


#ifndef WIN_NT
static void db_close( int file_desc)
{
/**************************************
 *
 *	d b _ c l o s e
 *
 **************************************
 *
 * Functional description
 *    Close an open file
 *
 **************************************/
	close(file_desc);
}

static void db_error( int status)
{
/**************************************
 *
 *	d b _ e r r o r
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();
	tddba->page_number = -1;

	// FIXME: The strerror() function returns the appropriate description
	// string, or an unknown error message if the error code is unknown.
	// EKU: p cannot be NULL!
	if (!tddba->uSvc->isService())
	{
		tddba->uSvc->printf(true, "%s\n", strerror(status));
	}
	dba_exit(FINI_ERROR, tddba);
}


// CVC: This function was using cast to char* for the first param always
// and the callers had to cast their char*'s to UCHAR*, too. Since the
// real parameter is char* and always the usage is char*, I changed signature.
static dba_fil* db_open(const char* file_name, USHORT file_length)
{
/**************************************
 *
 *	d b _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Open a database file.
 *      Put the file on an ordered list.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();

	dba_fil* fil;
	if (tddba->files)
	{
		for (fil = tddba->files; fil->fil_next; fil = fil->fil_next)
			;
		fil->fil_next = (dba_fil*) alloc(sizeof(dba_fil) + file_length + 1);
		fil->fil_next->fil_min_page = fil->fil_max_page + 1;
		fil = fil->fil_next;
	}
	else
	{
		// empty list

		fil = tddba->files = (dba_fil*) alloc(sizeof(dba_fil) + file_length + 1);
		fil->fil_min_page = 0L;
	}

	fil->fil_next = NULL;
	strcpy(fil->fil_string, file_name);
	fil->fil_length = file_length;
	fil->fil_fudge = 0;
	fil->fil_max_page = 0L;

	if ((fil->fil_desc = open(file_name, O_RDONLY)) == -1)
	{
		tddba->uSvc->setServiceStatus(GSTAT_MSG_FAC, 29, SafeArg() << file_name);
		// msg 29: Can't open database file %s
		db_error(errno);
	}

	open_files* file_list = FB_NEW(*getDefaultMemoryPool()) open_files;
	if (!file_list)
	{
		// NOMEM: return error
		dba_error(31);
	}
	file_list->desc = fil->fil_desc;
	file_list->open_files_next = 0;

	if (tddba->head_of_files_list == 0)
		tddba->head_of_files_list = file_list;
	else
	{
		file_list->open_files_next = tddba->head_of_files_list;
		tddba->head_of_files_list = file_list;
	}

	return fil;
}


static const pag* db_read( SLONG page_number, bool ok_enc)
{
/**************************************
 *
 *	d b _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a database page.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();

	if (tddba->page_number == page_number)
		return tddba->global_buffer;

	tddba->page_number = page_number;

	dba_fil* fil;
	for (fil = tddba->files; page_number > (SLONG) fil->fil_max_page && fil->fil_next;)
	{
		fil = fil->fil_next;
	}

	page_number -= fil->fil_min_page - fil->fil_fudge;
	const FB_UINT64 offset = ((FB_UINT64) page_number) * ((FB_UINT64) tddba->page_size);
	if (lseek (fil->fil_desc, offset, 0) == -1)
	{
		tddba->uSvc->setServiceStatus(GSTAT_MSG_FAC, 30, SafeArg());
		// msg 30: Can't read a database page
		db_error(errno);
	}

	SSHORT length = tddba->page_size;
	for (SCHAR* p = (SCHAR *) tddba->global_buffer; length > 0;)
	{
		const SSHORT l = read(fil->fil_desc, p, length);
		if (l < 0)
		{
			tddba->uSvc->setServiceStatus(GSTAT_MSG_FAC, 30, SafeArg());
			// msg 30: Can't read a database page
			db_error(errno);
		}
		if (!l)
		{
			if (ok_enc)
			{
				return NULL;
			}
			dba_error(4);
			// msg 4: Unexpected end of database file.
		}
		p += l;
		length -= l;
	}

	if (tddba->global_buffer->pag_flags & Ods::crypted_page && !ok_enc)
	{
		dba_error(55);
	}

	return tddba->global_buffer;
}
#endif


static void dba_error(USHORT errcode, const SafeArg& arg)
{
/**************************************
 *
 *	d b a _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Format and print an error message, then punt.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();
	tddba->page_number = -1;

	tddba->uSvc->setServiceStatus(GSTAT_MSG_FAC, errcode, arg);
	if (!tddba->uSvc->isService())
	{
		dba_print(true, errcode, arg);
	}
	dba_exit(FINI_ERROR, tddba);
}


static void dba_print(bool err, USHORT number, const SafeArg& arg)
{
/**************************************
 *
 *	d b a _ p r i n t
 *
 **************************************
 *
 * Functional description
 *      Retrieve a message from the error file, format it, and print it.
 *
 **************************************/
	TEXT buffer[256];
	tdba* tddba = tdba::getSpecific();

	fb_msg_format(NULL, GSTAT_MSG_FAC, number, sizeof(buffer), buffer, arg);
	tddba->uSvc->printf(err, "%s\n", buffer);
}


static void print_distribution(const SCHAR* prefix, const ULONG* vector)
{
/**************************************
 *
 *	p r i n t _ d i s t r i b u t i o n
 *
 **************************************
 *
 * Functional description
 *	Print distribution as percentages.
 *
 **************************************/
	tdba* tddba = tdba::getSpecific();

	for (SSHORT n = 0; n < BUCKETS; n++)
	{
		tddba->uSvc->printf(false, "%s%2d - %2d%% = %" ULONGFORMAT "\n",
				prefix, n * 100 / BUCKETS, (n + 1) * 100 / BUCKETS - 1, vector[n]);
	}
}


// Print the help explanation
static void print_help()
{
	dba_print(true, 39);	// msg 39: usage:   gstat [options] <database> or gstat <database> [options]
	dba_print(true, 21);	// msg 21: Available switches:
	for (const Switches::in_sw_tab_t* in_sw_tab = dba_in_sw_table; in_sw_tab->in_sw; in_sw_tab++)
	{
		if (in_sw_tab->in_sw_msg)
			dba_print(true, in_sw_tab->in_sw_msg);
	}
	dba_print(true, 43);	// option -t accepts...
}
