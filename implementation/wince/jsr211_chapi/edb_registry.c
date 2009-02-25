/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

/**
 * @file edb_registry.c
 * @ingroup CHAPI
 * @brief javacall chapi registry access implementation using wince EDB database engine
 */
#define WIN32_LEAN_AND_MEAN

#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

/*defined to use modern EDB API instead of obsolete CEDB*/
#define EDB

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

#include "javacall_chapi_registry.h"
#include "javacall_memory.h"

/* recreate database on index structure change
  should be used only in development time */
/* #define JSR211_RECREATE_ON_FAILURE 1 */

#define CHAPI_HEADER L"CHAPI REGISRTY"
#define CHAPI_DB_VOLUME L"\\chapireg.vol"
#define CHAPI_HANDLERS_DB_NAME L"content_handlers_db"
#define CHAPI_CONTENTTYPE_DB_NAME L"content_types_db"
#define CHAPI_SUFFIX_DB_NAME L"suffixes_db"
#define CHAPI_ACTION_DB_NAME L"actions_db"
#define CHAPI_LACTION_DB_NAME L"local_action_names_db"
#define CHAPI_ACCESS_DB_NAME L"access_allowed_db"

/* Property ID's */

#define MAKEPROP(n,t)    ((n<<16)|CEVT_##t)

#define PROPID_REGISTRY_HANDLER_OID					MAKEPROP(0x100,AUTO_I4)
#define PROPID_REGISTRY_HANDLER_NAME				MAKEPROP(0x101,LPWSTR)
#define PROPID_REGISTRY_HANDLER_APPNAME				MAKEPROP(0x102,LPWSTR)
#define PROPID_REGISTRY_SUITEID						MAKEPROP(0x103,LPWSTR)
#define PROPID_REGISTRY_CLASSNAME					MAKEPROP(0x104,LPWSTR)
#define PROPID_REGISTRY_FLAG						MAKEPROP(0x104,I4)

#define PROPID_CONTENTTYPE_NAME						MAKEPROP(0x201,LPWSTR)
#define PROPID_CONTENTTYPE_HANDLER_OID				MAKEPROP(0x202,I4)

#define PROPID_SUFFIX_NAME							MAKEPROP(0x301,LPWSTR)
#define PROPID_SUFFIX_HANDLER_OID					MAKEPROP(0x302,I4)

#define PROPID_ACTION_OID							MAKEPROP(0x400,AUTO_I4)
#define PROPID_ACTION_NAME							MAKEPROP(0x401,LPWSTR)
#define PROPID_ACTION_HANDLER_OID					MAKEPROP(0x402,I4)

#define PROPID_LACTION_ACTION_OID					MAKEPROP(0x500,I4)
#define PROPID_LACTION_LOCALE						MAKEPROP(0x501,LPWSTR)
#define PROPID_LACTION_ACTION_NAME					MAKEPROP(0x502,LPWSTR)

#define PROPID_ACCESS_CALLER_ID						MAKEPROP(0x601,LPWSTR)
#define PROPID_ACCESS_HANDLER_OID					MAKEPROP(0x602,I4)

static CEGUID volGUID = {-1L,-1L,-1L,-1L};

#define NO_SORT -1L

/* used to indicate that this result was already issued */
#define CHAPI_REPEAT_RESULT 0xEE00
#define CHAPI_PREPARED 0xAA00

typedef struct enum_pos
{
	HANDLE m_hDB; /* searched database handle */
	CEPROPVAL* m_pPropVal; /* last read property */
	DWORD m_dwSize;
} enum_pos, *enum_pos_ptr;


/**
* Log out win32 error message
* @param dwError - win32 error code
*/
static void log_win32_error(DWORD dwError)
{
	LPWSTR lpMsgBuf;
	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwError,
		0, /* Default language */
		(LPWSTR) &lpMsgBuf,
		0,
		NULL 
		);
	fwprintf(stderr, L"[%s ERROR (%#x)]: %s\n", CHAPI_HEADER, dwError, lpMsgBuf);
}

/**
* Log out error message
* @param msg - message pattern with argument list
*/
static void log_error(LPWSTR msg, ...)
{
	va_list args;
	va_start(args,msg);
	fwprintf(stderr, L"%s ERROR: ", CHAPI_HEADER);
	vfwprintf(stderr, msg, args);
	fwprintf(stderr, L"\n", CHAPI_HEADER);
	va_end(args);
}


/**
* Common block for unsuccessful record seeking
* @param hDb - handle of database for closing
* @param result - result to return if no other error caused failure
*/
static javacall_result bad_seek(HANDLE hDb, javacall_result result)
{
	DWORD dwError = GetLastError();

	CloseHandle(hDb);

	if (dwError == ERROR_SEEK || dwError == ERROR_NO_MORE_ITEMS)
	{
		return result;
	} else {
		log_win32_error(dwError);
		return JAVACALL_FAIL;
	}
}

/*
*	Copy result to client buffer, return length of result
*	@param src - source string to copy
*	@param dest - pointer to client buffer
*	@param length - pointer to integer containing original length of client buffer in characters or
*          zero for calculating what length needed
*	@return JAVACALL_OK on success or JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if length is not enough, 
*           length will contain size of source string in characters including terminating zero
*/
static javacall_result copy_result(CEPROPVAL* pPropVal,  javacall_utf16*  dest, int* length)
{
	int len;
	javacall_utf16* src;
	if (!pPropVal || pPropVal->wFlags == CEDB_PROPNOTFOUND || !pPropVal->val.lpwstr) src = L""; else src = pPropVal->val.lpwstr;
	len = wcslen(src) + 1;
	if (*length < len) 
	{
		*length = len;
		return JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL;
	}
	memcpy(dest, src, len * sizeof(javacall_utf16));
	return JAVACALL_OK;
}

/*
*	Opens handlers database, create if not exists
*	@param hSession - handle on session can be NULL
*	@param iSort - sort index NO_SORT for none
*	@param phDB - pointer to output handle
*	@return 0 on success win32 error code on failure
*/
static DWORD open_handlers_db(IN HANDLE hSession, int iSort, BOOL bNotCreate, OUT HANDLE*  phDB)
{
	SORTORDERSPECEX* pSort = NULL;
	CEDBASEINFOEX     DBInfo = {0};
	DWORD dwOID = 0;
	DWORD dwErr;

	*phDB = INVALID_HANDLE_VALUE;

	DBInfo.wVersion = CEDBASEINFOEX_VERSION;
	DBInfo.dwFlags = CEDB_VALIDDBFLAGS | CEDB_VALIDNAME | CEDB_VALIDSORTSPEC;
	DBInfo.wNumSortOrder = 3;

	DBInfo.rgSortSpecs[0].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[0].wKeyFlags = CEDB_SORT_PRIMARYKEY;
	DBInfo.rgSortSpecs[0].wNumProps = 1;
	DBInfo.rgSortSpecs[0].rgPropID[0] = PROPID_REGISTRY_HANDLER_OID;
	DBInfo.rgSortSpecs[0].rgdwFlags[0] = 0;

	DBInfo.rgSortSpecs[1].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[1].wKeyFlags = 0;
	DBInfo.rgSortSpecs[1].wNumProps = 1;
	DBInfo.rgSortSpecs[1].rgPropID[0] = PROPID_REGISTRY_HANDLER_NAME;
	DBInfo.rgSortSpecs[1].rgdwFlags[0] = 0;

	DBInfo.rgSortSpecs[2].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[2].wKeyFlags = 0;
	DBInfo.rgSortSpecs[2].wNumProps = 1;
	DBInfo.rgSortSpecs[2].rgPropID[0] = PROPID_REGISTRY_SUITEID;
	DBInfo.rgSortSpecs[2].rgdwFlags[0] = 0;


	wcscpy(DBInfo.szDbaseName, CHAPI_HANDLERS_DB_NAME);

	if (iSort != NO_SORT) pSort = &DBInfo.rgSortSpecs[iSort];
	
	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, DBInfo.szDbaseName, pSort, 0, NULL);
	if (*phDB != INVALID_HANDLE_VALUE) return 0;

	dwErr = GetLastError();
	if (dwErr)
	{
#ifdef JSR211_RECREATE_ON_FAILURE
		if (dwErr == ERROR_INVALID_PARAMETER)
		{
			/* database has invalid index structure - recreate it */
			HANDLE hDb = CeOpenDatabaseInSession(NULL, &volGUID, &dwOID, DBInfo.szDbaseName, NULL, 0, NULL);
			if (!dwOID) return GetLastError();
			CloseHandle(hDb);
			/* delete */
			log_error(L"Database %S has invalid structure and being recreated..", DBInfo.szDbaseName);
			if (!CeDeleteDatabaseEx(&volGUID,dwOID)) return GetLastError();
			dwErr = ERROR_FILE_NOT_FOUND;
		}
#endif
		if (bNotCreate || dwErr != ERROR_FILE_NOT_FOUND) 
			return dwErr;
		else
		{
		/* database does not exist - create */
			const DWORD cProps = 6;
			CEPROPSPEC	prgProps[6] = {0};

			prgProps[0].wVersion = CEPROPSPEC_VERSION;
			prgProps[0].propid = PROPID_REGISTRY_HANDLER_OID;
			prgProps[0].dwFlags = DB_PROP_NOTNULL;
			prgProps[0].pwszPropName = L"HandlerOID";
			prgProps[0].cchPropName = wcslen(prgProps[0].pwszPropName);

			prgProps[1].wVersion = CEPROPSPEC_VERSION;
			prgProps[1].propid = PROPID_REGISTRY_HANDLER_NAME;
			prgProps[1].dwFlags = DB_PROP_NOTNULL;
			prgProps[1].pwszPropName = L"HandlerName";
			prgProps[1].cchPropName = wcslen(prgProps[1].pwszPropName);

			prgProps[2].wVersion = CEPROPSPEC_VERSION;
			prgProps[2].propid = PROPID_REGISTRY_HANDLER_APPNAME;
			prgProps[2].dwFlags = 0;
			prgProps[2].pwszPropName = L"HandlerAppName";
			prgProps[2].cchPropName = wcslen(prgProps[2].pwszPropName);

			prgProps[3].wVersion = CEPROPSPEC_VERSION;
			prgProps[3].propid = PROPID_REGISTRY_SUITEID;
			prgProps[3].dwFlags = 0;
			prgProps[3].pwszPropName = L"SuiteID";
			prgProps[3].cchPropName = wcslen(prgProps[3].pwszPropName);

			prgProps[4].wVersion = CEPROPSPEC_VERSION;
			prgProps[4].propid = PROPID_REGISTRY_CLASSNAME;
			prgProps[4].dwFlags = 0;
			prgProps[4].pwszPropName = L"ClassName";
			prgProps[4].cchPropName = wcslen(prgProps[4].pwszPropName);

			prgProps[5].wVersion = CEPROPSPEC_VERSION;
			prgProps[5].propid = PROPID_REGISTRY_FLAG;
			prgProps[5].dwFlags = 0;
			prgProps[5].pwszPropName = L"TypeFlag";
			prgProps[5].cchPropName = wcslen(prgProps[5].pwszPropName);

			if(0 == (dwOID = CeCreateDatabaseWithProps(&volGUID, &DBInfo, cProps, prgProps)))
			{
				return GetLastError();
			}
		}
	}

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, NULL, pSort, 0, NULL);
	if (*phDB == INVALID_HANDLE_VALUE) return GetLastError();
	return 0;
}

/*
*	Opens content types database, create if not exists
*	@param hSession - handle on session can be NULL
*	@param iSort - sort index NO_SORT for none
*	@param phDB - pointer to output handle
*	@return 0 on success win32 error code on failure
*/
static DWORD open_contenttype_db(IN HANDLE hSession, int iSort, BOOL bNotCreate, OUT HANDLE*  phDB)
{
	SORTORDERSPECEX* pSort = NULL;
	CEDBASEINFOEX     DBInfo = {0};
	DWORD dwOID = 0;
	DWORD dwErr;

	*phDB = INVALID_HANDLE_VALUE;

	DBInfo.wVersion = CEDBASEINFOEX_VERSION;
	DBInfo.dwFlags = CEDB_VALIDDBFLAGS | CEDB_VALIDNAME | CEDB_VALIDSORTSPEC;
	DBInfo.wNumSortOrder = 2;

	DBInfo.rgSortSpecs[0].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[0].wKeyFlags = 0;
	DBInfo.rgSortSpecs[0].wNumProps = 2;
	DBInfo.rgSortSpecs[0].rgPropID[0] = PROPID_CONTENTTYPE_NAME;
	DBInfo.rgSortSpecs[0].rgdwFlags[0] = CEDB_SORT_CASEINSENSITIVE;
	DBInfo.rgSortSpecs[0].rgPropID[1] = PROPID_CONTENTTYPE_HANDLER_OID;
	DBInfo.rgSortSpecs[0].rgdwFlags[1] = 0;

	DBInfo.rgSortSpecs[1].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[1].wKeyFlags = 0;
	DBInfo.rgSortSpecs[1].wNumProps = 2;
	DBInfo.rgSortSpecs[1].rgPropID[0] = PROPID_CONTENTTYPE_HANDLER_OID;
	DBInfo.rgSortSpecs[1].rgdwFlags[0] = 0;
	DBInfo.rgSortSpecs[1].rgPropID[1] = PROPID_CONTENTTYPE_NAME;
	DBInfo.rgSortSpecs[1].rgdwFlags[1] = CEDB_SORT_CASEINSENSITIVE;

	wcscpy(DBInfo.szDbaseName, CHAPI_CONTENTTYPE_DB_NAME);

	if (iSort != NO_SORT) pSort = &DBInfo.rgSortSpecs[iSort];

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, DBInfo.szDbaseName, pSort, 0, NULL);
	if (*phDB != INVALID_HANDLE_VALUE) return 0;

	dwErr = GetLastError();
	if (dwErr)
	{
#ifdef JSR211_RECREATE_ON_FAILURE
		if (dwErr == ERROR_INVALID_PARAMETER)
		{
			/* database has invalid index structure - recreate it */
			HANDLE hDb = CeOpenDatabaseInSession(NULL, &volGUID, &dwOID, DBInfo.szDbaseName, NULL, 0, NULL);
			if (!dwOID) return GetLastError();
			CloseHandle(hDb);
			/* delete */
			log_error(L"Database %S has invalid structure and being recreated..", DBInfo.szDbaseName);
			if (!CeDeleteDatabaseEx(&volGUID,dwOID)) return GetLastError();
			dwErr = ERROR_FILE_NOT_FOUND;
		}
#endif
		if (bNotCreate || dwErr != ERROR_FILE_NOT_FOUND) 
			return dwErr;
		else
		{
			/* database does not exist - create */
			const DWORD cProps = 2;
			CEPROPSPEC	prgProps[2] = {0};

			prgProps[0].wVersion = CEPROPSPEC_VERSION;
			prgProps[0].propid = PROPID_CONTENTTYPE_NAME;
			prgProps[0].dwFlags = DB_PROP_NOTNULL;
			prgProps[0].pwszPropName = L"ContentType";
			prgProps[0].cchPropName = wcslen(prgProps[0].pwszPropName);

			prgProps[1].wVersion = CEPROPSPEC_VERSION;
			prgProps[1].propid = PROPID_CONTENTTYPE_HANDLER_OID;
			prgProps[1].dwFlags = DB_PROP_NOTNULL;
			prgProps[1].pwszPropName = L"HandlerOID";
			prgProps[1].cchPropName = wcslen(prgProps[1].pwszPropName);

			if(0 == (dwOID = CeCreateDatabaseWithProps(&volGUID, &DBInfo, cProps, prgProps)))
			{
				return GetLastError();
			}
		}
	}

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, NULL, pSort, 0, NULL);
	if (*phDB == INVALID_HANDLE_VALUE) return GetLastError();
	return 0;
}


/*
*	Opens suffixes database, create if not exists
*	@param hSession - handle on session can be NULL
*	@param iSort - sort index NO_SORT for none
*	@param phDB - pointer to output handle
*	@return 0 on success win32 error code on failure
*/
static DWORD open_suffixes_db(IN HANDLE hSession, int iSort, BOOL bNotCreate, OUT HANDLE*  phDB)
{
	SORTORDERSPECEX* pSort = NULL;
	CEDBASEINFOEX     DBInfo = {0};
	DWORD dwOID = 0;
	DWORD dwErr;

	*phDB = INVALID_HANDLE_VALUE;

	DBInfo.wVersion = CEDBASEINFOEX_VERSION;
	DBInfo.dwFlags = CEDB_VALIDDBFLAGS | CEDB_VALIDNAME | CEDB_VALIDSORTSPEC;
	DBInfo.wNumSortOrder = 2;

	DBInfo.rgSortSpecs[0].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[0].wKeyFlags = 0;
	DBInfo.rgSortSpecs[0].wNumProps = 2;
	DBInfo.rgSortSpecs[0].rgPropID[0] = PROPID_SUFFIX_NAME;
	DBInfo.rgSortSpecs[0].rgdwFlags[0] = CEDB_SORT_CASEINSENSITIVE;
	DBInfo.rgSortSpecs[0].rgPropID[1] = PROPID_SUFFIX_HANDLER_OID;
	DBInfo.rgSortSpecs[0].rgdwFlags[1] = 0;

	DBInfo.rgSortSpecs[1].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[1].wKeyFlags = 0;
	DBInfo.rgSortSpecs[1].wNumProps = 2;
	DBInfo.rgSortSpecs[1].rgPropID[0] = PROPID_SUFFIX_HANDLER_OID;
	DBInfo.rgSortSpecs[1].rgdwFlags[0] = 0;
	DBInfo.rgSortSpecs[1].rgPropID[1] = PROPID_SUFFIX_NAME;
	DBInfo.rgSortSpecs[1].rgdwFlags[1] = CEDB_SORT_CASEINSENSITIVE;

	wcscpy(DBInfo.szDbaseName, CHAPI_SUFFIX_DB_NAME);

	if (iSort != NO_SORT) pSort = &DBInfo.rgSortSpecs[iSort];

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, DBInfo.szDbaseName, pSort, 0, NULL);
	if (*phDB != INVALID_HANDLE_VALUE) return 0;

	dwErr = GetLastError();
	if (dwErr)
	{
#ifdef JSR211_RECREATE_ON_FAILURE
		if (dwErr == ERROR_INVALID_PARAMETER)
		{
			/* database has invalid index structure - recreate it */
			HANDLE hDb = CeOpenDatabaseInSession(NULL, &volGUID, &dwOID, DBInfo.szDbaseName, NULL, 0, NULL);
			if (!dwOID) return GetLastError();
			CloseHandle(hDb);
			/* delete */
			log_error(L"Database %S has invalid structure and being recreated..", DBInfo.szDbaseName);
			if (!CeDeleteDatabaseEx(&volGUID,dwOID)) return GetLastError();
			dwErr = ERROR_FILE_NOT_FOUND;
		}
#endif
		if (bNotCreate || dwErr != ERROR_FILE_NOT_FOUND) 
			return dwErr;
		else
		{
			/* database does not exist - create */
			const DWORD cProps = 2;
			CEPROPSPEC	prgProps[2] = {0};

			prgProps[0].wVersion = CEPROPSPEC_VERSION;
			prgProps[0].propid = PROPID_SUFFIX_NAME;
			prgProps[0].dwFlags = DB_PROP_NOTNULL;
			prgProps[0].pwszPropName = L"Suffix";
			prgProps[0].cchPropName = wcslen(prgProps[0].pwszPropName);

			prgProps[1].wVersion = CEPROPSPEC_VERSION;
			prgProps[1].propid = PROPID_SUFFIX_HANDLER_OID;
			prgProps[1].dwFlags = DB_PROP_NOTNULL;
			prgProps[1].pwszPropName = L"HandlerOID";
			prgProps[1].cchPropName = wcslen(prgProps[1].pwszPropName);

			if(0 == (dwOID = CeCreateDatabaseWithProps(&volGUID, &DBInfo, cProps, prgProps)))
			{
				return GetLastError();
			}
		}
	}

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, NULL, pSort, 0, NULL);
	if (*phDB == INVALID_HANDLE_VALUE) return GetLastError();
	return 0;
}

/*
*	Opens access allowed callers database, create if not exists
*	@param hSession - handle on session can be NULL
*	@param iSort - sort index NO_SORT for none
*	@param phDB - pointer to output handle
*	@return 0 on success win32 error code on failure
*/
static DWORD open_access_db(IN HANDLE hSession, int iSort, BOOL bNotCreate, OUT HANDLE*  phDB)
{
	SORTORDERSPECEX* pSort = NULL;
	CEDBASEINFOEX     DBInfo = {0};
	DWORD dwOID = 0;
	DWORD dwErr;

	*phDB = INVALID_HANDLE_VALUE;

	DBInfo.wVersion = CEDBASEINFOEX_VERSION;
	DBInfo.dwFlags = CEDB_VALIDDBFLAGS | CEDB_VALIDNAME | CEDB_VALIDSORTSPEC;
	DBInfo.wNumSortOrder = 2;

	DBInfo.rgSortSpecs[0].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[0].wKeyFlags = 0;
	DBInfo.rgSortSpecs[0].wNumProps = 2;
	DBInfo.rgSortSpecs[0].rgPropID[0] = PROPID_ACCESS_CALLER_ID;
	DBInfo.rgSortSpecs[0].rgdwFlags[0] = 0;
	DBInfo.rgSortSpecs[0].rgPropID[1] = PROPID_ACCESS_HANDLER_OID;
	DBInfo.rgSortSpecs[0].rgdwFlags[1] = 0;

	DBInfo.rgSortSpecs[1].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[1].wKeyFlags = 0;
	DBInfo.rgSortSpecs[1].wNumProps = 2;
	DBInfo.rgSortSpecs[1].rgPropID[0] = PROPID_ACCESS_HANDLER_OID;
	DBInfo.rgSortSpecs[1].rgdwFlags[0] = 0;
	DBInfo.rgSortSpecs[1].rgPropID[1] = PROPID_ACCESS_CALLER_ID;
	DBInfo.rgSortSpecs[1].rgdwFlags[1] = 0;

	wcscpy(DBInfo.szDbaseName, CHAPI_ACCESS_DB_NAME);

	if (iSort != NO_SORT) pSort = &DBInfo.rgSortSpecs[iSort];

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, DBInfo.szDbaseName, pSort, 0, NULL);
	if (*phDB != INVALID_HANDLE_VALUE) return 0;

	dwErr = GetLastError();
	if (dwErr)
	{
#ifdef JSR211_RECREATE_ON_FAILURE
		if (dwErr == ERROR_INVALID_PARAMETER)
		{
			/* database has invalid index structure - recreate it */
			HANDLE hDb = CeOpenDatabaseInSession(NULL, &volGUID, &dwOID, DBInfo.szDbaseName, NULL, 0, NULL);
			if (!dwOID) return GetLastError();
			CloseHandle(hDb);
			/* delete */
			log_error(L"Database %S has invalid structure and being recreated..", DBInfo.szDbaseName);
			if (!CeDeleteDatabaseEx(&volGUID,dwOID)) return GetLastError();
			dwErr = ERROR_FILE_NOT_FOUND;
		}
#endif
		if (bNotCreate || dwErr != ERROR_FILE_NOT_FOUND) 
			return dwErr;
		else
		{
			/* database does not exist - create */
			const DWORD cProps = 2;
			CEPROPSPEC	prgProps[2] = {0};

			prgProps[0].wVersion = CEPROPSPEC_VERSION;
			prgProps[0].propid = PROPID_ACCESS_CALLER_ID;
			prgProps[0].dwFlags = DB_PROP_NOTNULL;
			prgProps[0].pwszPropName = L"CallerID";
			prgProps[0].cchPropName = wcslen(prgProps[0].pwszPropName);

			prgProps[1].wVersion = CEPROPSPEC_VERSION;
			prgProps[1].propid = PROPID_ACCESS_HANDLER_OID;
			prgProps[1].dwFlags = DB_PROP_NOTNULL;
			prgProps[1].pwszPropName = L"HandlerOID";
			prgProps[1].cchPropName = wcslen(prgProps[1].pwszPropName);

			if(0 == (dwOID = CeCreateDatabaseWithProps(&volGUID, &DBInfo, cProps, prgProps)))
			{
				return GetLastError();
			}
		}
	}

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, NULL, pSort, 0, NULL);
	if (*phDB == INVALID_HANDLE_VALUE) return GetLastError();
	return 0;
}

/*
*	Opens actions database, create if not exists
*	@param hSession - handle on session can be NULL
*	@param iSort - sort index NO_SORT for none
*	@param phDB - pointer to output handle
*	@return 0 on success win32 error code on failure
*/
static DWORD open_actions_db(IN HANDLE hSession, int iSort, BOOL bNotCreate, OUT HANDLE*  phDB)
{
	SORTORDERSPECEX* pSort = NULL;
	CEDBASEINFOEX     DBInfo = {0};
	DWORD dwOID = 0;
	DWORD dwErr;

	*phDB = INVALID_HANDLE_VALUE;

	DBInfo.wVersion = CEDBASEINFOEX_VERSION;
	DBInfo.dwFlags = CEDB_VALIDDBFLAGS | CEDB_VALIDNAME | CEDB_VALIDSORTSPEC;
	DBInfo.wNumSortOrder = 2;

	DBInfo.rgSortSpecs[0].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[0].wKeyFlags = 0;
	DBInfo.rgSortSpecs[0].wNumProps = 2;
	DBInfo.rgSortSpecs[0].rgPropID[0] = PROPID_ACTION_NAME;
	DBInfo.rgSortSpecs[0].rgdwFlags[0] = 0;
	DBInfo.rgSortSpecs[0].rgPropID[1] = PROPID_ACTION_HANDLER_OID;
	DBInfo.rgSortSpecs[0].rgdwFlags[1] = 0;

	DBInfo.rgSortSpecs[1].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[1].wKeyFlags = 0;
	DBInfo.rgSortSpecs[1].wNumProps = 2;
	DBInfo.rgSortSpecs[1].rgPropID[0] = PROPID_ACTION_HANDLER_OID;
	DBInfo.rgSortSpecs[1].rgdwFlags[0] = 0;
	DBInfo.rgSortSpecs[1].rgPropID[1] = PROPID_ACTION_NAME;
	DBInfo.rgSortSpecs[1].rgdwFlags[1] = 0;

	wcscpy(DBInfo.szDbaseName, CHAPI_ACTION_DB_NAME);

	if (iSort != NO_SORT) pSort = &DBInfo.rgSortSpecs[iSort];

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, DBInfo.szDbaseName, pSort, 0, NULL);
	if (*phDB != INVALID_HANDLE_VALUE) return 0;

	dwErr = GetLastError();
	if (dwErr)
	{
#ifdef JSR211_RECREATE_ON_FAILURE
		if (dwErr == ERROR_INVALID_PARAMETER)
		{
			/* database has invalid index structure - recreate it */
			HANDLE hDb = CeOpenDatabaseInSession(NULL, &volGUID, &dwOID, DBInfo.szDbaseName, NULL, 0, NULL);
			if (!dwOID) return GetLastError();
			CloseHandle(hDb);
			/* delete */
			log_error(L"Database %S has invalid structure and being recreated..", DBInfo.szDbaseName);
			if (!CeDeleteDatabaseEx(&volGUID,dwOID)) return GetLastError();
			dwErr = ERROR_FILE_NOT_FOUND;
		}
#endif
		if (bNotCreate || dwErr != ERROR_FILE_NOT_FOUND) 
			return dwErr;
		else
		{
			/* database does not exist - create */
			const DWORD cProps = 3;
			CEPROPSPEC	prgProps[3] = {0};

			prgProps[0].wVersion = CEPROPSPEC_VERSION;
			prgProps[0].propid = PROPID_ACTION_OID;
			prgProps[0].dwFlags = DB_PROP_NOTNULL;
			prgProps[0].pwszPropName = L"ActionOID";
			prgProps[0].cchPropName = wcslen(prgProps[0].pwszPropName);

			prgProps[1].wVersion = CEPROPSPEC_VERSION;
			prgProps[1].propid = PROPID_ACTION_NAME;
			prgProps[1].dwFlags = DB_PROP_NOTNULL;
			prgProps[1].pwszPropName = L"ActionName";
			prgProps[1].cchPropName = wcslen(prgProps[1].pwszPropName);

			prgProps[2].wVersion = CEPROPSPEC_VERSION;
			prgProps[2].propid = PROPID_ACTION_HANDLER_OID;
			prgProps[2].dwFlags = DB_PROP_NOTNULL;
			prgProps[2].pwszPropName = L"HandlerOID";
			prgProps[2].cchPropName = wcslen(prgProps[2].pwszPropName);

			if(0 == (dwOID = CeCreateDatabaseWithProps(&volGUID, &DBInfo, cProps, prgProps)))
			{
				return GetLastError();
			}
		}
	}

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, NULL, pSort, 0, NULL);
	if (*phDB == INVALID_HANDLE_VALUE) return GetLastError();
	return 0;
}

/*
*	Opens action local names database, create if not exists
*	@param hSession - handle on session can be NULL
*	@param iSort - sort index NO_SORT for none
*	@param phDB - pointer to output handle
*	@return 0 on success win32 error code on failure
*/
static DWORD open_action_local_names_db(IN HANDLE hSession, int iSort, BOOL bNotCreate, OUT HANDLE*  phDB)
{
	SORTORDERSPECEX* pSort = NULL;
	CEDBASEINFOEX     DBInfo = {0};
	DWORD dwOID = 0;
	DWORD dwErr;

	*phDB = INVALID_HANDLE_VALUE;

	DBInfo.wVersion = CEDBASEINFOEX_VERSION;
	DBInfo.dwFlags = CEDB_VALIDDBFLAGS | CEDB_VALIDNAME | CEDB_VALIDSORTSPEC;
	DBInfo.wNumSortOrder = 1;

	DBInfo.rgSortSpecs[0].wVersion = SORTORDERSPECEX_VERSION;
	DBInfo.rgSortSpecs[0].wKeyFlags = 0;
	DBInfo.rgSortSpecs[0].wNumProps = 2;
	DBInfo.rgSortSpecs[0].rgPropID[0] = PROPID_LACTION_ACTION_OID;
	DBInfo.rgSortSpecs[0].rgdwFlags[0] = 0;
	DBInfo.rgSortSpecs[0].rgPropID[1] = PROPID_LACTION_LOCALE;
	DBInfo.rgSortSpecs[0].rgdwFlags[1] = 0;


	wcscpy(DBInfo.szDbaseName, CHAPI_LACTION_DB_NAME);

	if (iSort != NO_SORT) pSort = &DBInfo.rgSortSpecs[iSort];

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, DBInfo.szDbaseName, pSort, 0, NULL);
	if (*phDB != INVALID_HANDLE_VALUE) return 0;

	dwErr = GetLastError();
	if (dwErr)
	{
#ifdef JSR211_RECREATE_ON_FAILURE
		if (dwErr == ERROR_INVALID_PARAMETER)
		{
			/* database has invalid index structure - recreate it */
			HANDLE hDb = CeOpenDatabaseInSession(NULL, &volGUID, &dwOID, DBInfo.szDbaseName, NULL, 0, NULL);
			if (!dwOID) return GetLastError();
			CloseHandle(hDb);
			/* delete */
			log_error(L"Database %S has invalid structure and being recreated..", DBInfo.szDbaseName);
			if (!CeDeleteDatabaseEx(&volGUID,dwOID)) return GetLastError();
			dwErr = ERROR_FILE_NOT_FOUND;
		}
#endif
		if (bNotCreate || dwErr != ERROR_FILE_NOT_FOUND) 
			return dwErr;
		else
		{
			/* database does not exist - create */
			const DWORD cProps = 3;
			CEPROPSPEC	prgProps[3] = {0};

			prgProps[0].wVersion = CEPROPSPEC_VERSION;
			prgProps[0].propid = PROPID_LACTION_ACTION_OID;
			prgProps[0].dwFlags = DB_PROP_NOTNULL;
			prgProps[0].pwszPropName = L"ActionOID";
			prgProps[0].cchPropName = wcslen(prgProps[0].pwszPropName);

			prgProps[1].wVersion = CEPROPSPEC_VERSION;
			prgProps[1].propid = PROPID_LACTION_LOCALE;
			prgProps[1].dwFlags = DB_PROP_NOTNULL;
			prgProps[1].pwszPropName = L"Locale";
			prgProps[1].cchPropName = wcslen(prgProps[1].pwszPropName);

			prgProps[2].wVersion = CEPROPSPEC_VERSION;
			prgProps[2].propid = PROPID_LACTION_ACTION_NAME;
			prgProps[2].dwFlags = DB_PROP_NOTNULL;
			prgProps[2].pwszPropName = L"LocalName";
			prgProps[2].cchPropName = wcslen(prgProps[2].pwszPropName);

			if(0 == (dwOID = CeCreateDatabaseWithProps(&volGUID, &DBInfo, cProps, prgProps)))
			{
				return GetLastError();
			}
		}
	}

	*phDB = CeOpenDatabaseInSession(hSession, &volGUID, &dwOID, NULL, pSort, 0, NULL);
	if (*phDB == INVALID_HANDLE_VALUE) return GetLastError();
	return 0;
}

/*
*	Search handlers database for particular handler, returns its database OID if found
*	@param pszHandlerName - name of handler to search
*	@param pdwOid - pointer on variable receiving handler's OID
*	@return 0 on success error code on failure
*/
static DWORD get_handler_by_name(LPCWSTR pszHandlerName, DWORD OUT *pdwOid, DWORD OUT *pdwRecOid)
{
	HANDLE hRegDB = INVALID_HANDLE_VALUE;
	CEPROPVAL propValName = {PROPID_REGISTRY_HANDLER_NAME};
	CEPROPID propIds[1] = {PROPID_REGISTRY_HANDLER_OID};
	CEPROPVAL* pPropVals = NULL;
	DWORD dwSize = 0;
	DWORD dwRes;
	WORD nProps = 1;

	if (!pszHandlerName || !pdwOid) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
	dwRes = open_handlers_db(NULL, 1, TRUE, &hRegDB);
	if (dwRes)
	{
		if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
		log_win32_error(dwRes);
		return JAVACALL_FAIL;
	}

	propValName.val.lpwstr = (LPWSTR)pszHandlerName;
	if (!CeSeekDatabaseEx(hRegDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValName, 1, NULL))
		return bad_seek(hRegDB,JAVACALL_CHAPI_ERROR_NOT_FOUND);
	

	dwRes = CeReadRecordPropsEx(hRegDB, CEDB_ALLOWREALLOC, &nProps, propIds, (LPBYTE*)&pPropVals, &dwSize, GetProcessHeap());
	if (!dwRes)
	{
		dwRes = GetLastError();
		log_win32_error(dwRes);
		return JAVACALL_FAIL;
	}
	if (pdwRecOid) *pdwRecOid = dwRes;
	if (pdwOid) *pdwOid = pPropVals->val.ulVal;
	HeapFree(GetProcessHeap(),0,pPropVals);
	CloseHandle(hRegDB);
	return JAVACALL_OK;
}

/*
*	Search handlers database for handler with given database OID and returns its Handler ID
*	@param dwOid - handler's database OID
*	@param pPropVal - pointer to variable receiving handler's name property, should point to zero pointer
*	@return JAVACALL_OK on success error otherwise
*/
static javacall_result get_handler_by_oid(DWORD dwOid, CEPROPVAL **ppPropVal, DWORD* pdwSize)
{
	HANDLE hRegDB = INVALID_HANDLE_VALUE;
	CEPROPVAL propValOID = {0};
	DWORD propId[1] = {PROPID_REGISTRY_HANDLER_NAME};
	WORD count = 1 ;

	/* open sorted by oid */
	DWORD dwRes = open_handlers_db(NULL,0, TRUE, &hRegDB);
	if (dwRes)
	{
		if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
		log_win32_error(dwRes);
		return JAVACALL_FAIL;
	}

	propValOID.propid = PROPID_REGISTRY_HANDLER_OID;
	propValOID.val.ulVal = dwOid;

	/* search handler */
	if (!CeSeekDatabaseEx(hRegDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 1, NULL))
		return bad_seek(hRegDB,JAVACALL_CHAPI_ERROR_NOT_FOUND);

	if (!CeReadRecordPropsEx(hRegDB, CEDB_ALLOWREALLOC, &count, 
		(CEPROPID*)propId, (LPBYTE*)ppPropVal, pdwSize, GetProcessHeap()))
	{
		log_win32_error(GetLastError());
		CloseHandle(hRegDB);
		return JAVACALL_FAIL;
	}

	CloseHandle(hRegDB);
	return JAVACALL_OK;
}

/*
*   Enumerate registered content handlers that has corresponding indexed value in given database
*   see javacall_chapi_enum_handlers_by_* methods for more info
*	@param hDB - databse opened by needed index on first iteration or null on each next
*	@param indexed_value - indexed value to search on first iteration or null on each next
*	@param valuePropID - id of indexed value property
*	@param handlerPropID - id of HANDLER_ID property
*	@param pos_id - see javacall_chapi_enum_handlers_by_*
*	@param handler_id_out - see javacall_chapi_enum_handlers_by_*
*	@param length - see javacall_chapi_enum_handlers_by_*
*	@return JAVACALL_OK on success error otherwise
*/
static javacall_result enum_handlers_by_indexed_value(HANDLE hDB, javacall_const_utf16_string indexed_value, DWORD valuePropID, DWORD dwSeekFlag, DWORD handlerPropID, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length, int noCloseDbOnSeek)
{
	DWORD dwRes;
	WORD count = 1;
	CEPROPVAL* pPropVal = NULL;
	DWORD dwSize = 0;
	enum_pos_ptr epos = (enum_pos_ptr)*pos_id;

	if (!epos)
	{
		CEPROPVAL propIndexedVal = {0};

		/* search first record with matched value */
		propIndexedVal.propid = valuePropID;
		propIndexedVal.val.lpwstr = (LPWSTR)indexed_value;

		if (!CeSeekDatabaseEx(hDB, dwSeekFlag,(DWORD)&propIndexedVal, 1, NULL))
		{
			if (!noCloseDbOnSeek) CloseHandle(hDB);
			return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
		}

		/* read first record */
		if (!CeReadRecordPropsEx(hDB, CEDB_ALLOWREALLOC, &count, 
			(CEPROPID*)&handlerPropID, (LPBYTE*)&pPropVal, &dwSize, GetProcessHeap()))
		{
			log_win32_error(GetLastError());
			CloseHandle(hDB);
			return JAVACALL_FAIL;
		}

		/* allocate position structure */
		epos = (enum_pos_ptr)javacall_malloc(sizeof(enum_pos));
		if (!epos)
		{
			log_error(L"out of memory");
			CloseHandle(hDB);
			return JAVACALL_CHAPI_ERROR_NO_MEMORY;
		}

		epos->m_hDB = hDB;
		epos->m_pPropVal = pPropVal;
		epos->m_pPropVal->wFlags = CHAPI_PREPARED;
		epos->m_dwSize = dwSize;
		*pos_id = (int)epos;
	} 

	if (!epos->m_pPropVal || epos->m_pPropVal->wFlags != CHAPI_REPEAT_RESULT)
	{
			if (epos->m_pPropVal && epos->m_pPropVal->wFlags != CHAPI_PREPARED)
			{
				// search either next equal or just next sorted
				if (!CeSeekDatabaseEx(epos->m_hDB, dwSeekFlag == CEDB_SEEK_VALUEGREATEROREQUAL ? CEDB_SEEK_CURRENT : CEDB_SEEK_VALUENEXTEQUAL, 1, 0, NULL))
					return bad_seek(epos->m_hDB,JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS);
			}

			if (!CeReadRecordPropsEx(epos->m_hDB, CEDB_ALLOWREALLOC, &count, 
				(CEPROPID*)&handlerPropID, (LPBYTE*)&(epos->m_pPropVal), &(epos->m_dwSize), GetProcessHeap()))
			{
				log_win32_error(GetLastError());
				return JAVACALL_FAIL;
			}

		if (handlerPropID != PROPID_REGISTRY_HANDLER_NAME)
		{
			/* convert handler's OID to name */
			dwRes = get_handler_by_oid(epos->m_pPropVal->val.ulVal, &(epos->m_pPropVal),&epos->m_dwSize);
			if (dwRes && dwRes != JAVACALL_CHAPI_ERROR_NOT_FOUND) return dwRes;
		}
	}

	dwRes = copy_result(epos->m_pPropVal,handler_id_out,length);
	epos->m_pPropVal->wFlags = dwRes == JAVACALL_OK ? 0 : CHAPI_REPEAT_RESULT;

	return dwRes;
}

/*
*   Enumerate registered content handlers that has corresponding indexed value in given database
*   see javacall_chapi_enum_handlers_by_* methods for more info
*	@param hDB - databse of needed values opened by hander_id index
*	@param handlerOID - record's oid of needed hanlder
*	@param handlerPropID - id of HANDLER_ID property
*	@param valuePropID - id of indexed value property
*	@param content_handler_id - content_handler_id witch values to enum
*	@param pos_id - see javacall_chapi_enum_*
*	@param value_out - see javacall_chapi_enum_*
*	@param length - see javacall_chapi_enum_
*	@return JAVACALL_OK on success error otherwise
*/
static javacall_result enum_values_by_handler_id(HANDLE hDB, 
												 DWORD handlerOID,
												 DWORD handlerPropID,
												 DWORD valuePropID,
												 int* pos_id, /*OUT*/ javacall_utf16*  value_out, int* length)
{
	enum_pos_ptr epos;
	DWORD dwRes;
	WORD count = 1;
	CEPROPVAL* pPropVal = NULL;
	DWORD dwSize = 0;
	HANDLE hRegDB = INVALID_HANDLE_VALUE;

	if (!pos_id || !length || (*length && !value_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
	epos = (enum_pos_ptr) *pos_id;

	if (!epos)
	{
		CEPROPVAL filterVal = {0};
		filterVal.propid = handlerPropID;
		filterVal.val.ulVal = handlerOID;

		/* seek to first record */
		if (!CeSeekDatabaseEx(hDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&filterVal, 1, NULL))
		{
			CloseHandle(hDB);
			return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
		}

		/* read first record */
		if (!CeReadRecordPropsEx(hDB, CEDB_ALLOWREALLOC, &count, 
			(CEPROPID*)&valuePropID, (LPBYTE*)&pPropVal, &dwSize, GetProcessHeap()))
		{
			log_win32_error(GetLastError());
			CloseHandle(hDB);
			return JAVACALL_FAIL;
		}

		/* allocate position structure */
		epos = (enum_pos_ptr)javacall_malloc(sizeof(enum_pos));
		if (!epos)
		{
			log_error(L"out of memory");
			CloseHandle(hDB);
			return JAVACALL_CHAPI_ERROR_NO_MEMORY;
		}

		epos->m_hDB = hDB;
		epos->m_pPropVal = pPropVal;
		epos->m_dwSize = dwSize;
		*pos_id = (int)epos;

	} else {
		if (!epos->m_pPropVal || epos->m_pPropVal->wFlags != CHAPI_REPEAT_RESULT)
		{
			if (!CeSeekDatabaseEx(epos->m_hDB, CEDB_SEEK_VALUENEXTEQUAL, 1, 0, NULL))
				return bad_seek(epos->m_hDB,JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS);

			if (!CeReadRecordPropsEx(epos->m_hDB, CEDB_ALLOWREALLOC, &count, 
				(CEPROPID*)&valuePropID, (LPBYTE*)&(epos->m_pPropVal), &(epos->m_dwSize), GetProcessHeap()))
			{
				log_win32_error(GetLastError());
				return JAVACALL_FAIL;
			}
		}
	}

	dwRes = copy_result(epos->m_pPropVal,value_out,length);
	epos->m_pPropVal->wFlags = dwRes == JAVACALL_OK ? 0 : CHAPI_REPEAT_RESULT;
	
	return dwRes;
}

/*
*   Check that szPrefix is prefix of string szString case sensitively
*   szPrefix is not NULL
*	@param szPrefix - searched prefix
*	@param szString - compared string
*	@return 1 if comparison is true 0 if false
*/

static int compare_prefix_with_case(LPCWSTR szPrefix, LPCWSTR szString)
{
	register LPCWSTR a = szPrefix;
	register LPCWSTR b = szString;
	if (!b) return 0;
	while (*a) if (*a++ != *b++) return 0;
	return 1;
}


/************************************************************************/
/*          PUBLIC API                                                  */
/************************************************************************/

/**
* Perform initialization of registry API
*
* @return JAVACALL_OK if initialization was successful, error code otherwise
*/
javacall_result javacall_chapi_init_registry(void)
{
	BOOL bRes;
	memset(&volGUID, 0, sizeof volGUID);

	/* mount database volume, create if does not exist */
	bRes = CeMountDBVolEx( &volGUID, CHAPI_DB_VOLUME,
		NULL,
		OPEN_ALWAYS);

	if (!bRes)
	{
		log_win32_error(GetLastError());
		return JAVACALL_FAIL;
	}

	return JAVACALL_OK;
}

/**
* Finalize API, clean all used resources.
*
* @return nothing
*/
void javacall_chapi_finalize_registry(void)
{
	if (!CHECK_INVALIDGUID(&volGUID))
		CeUnmountDBVol(&volGUID);
}

/**
* Add new Content Handler to Registry
*
* @param content_handler_id unique ID of content handler
*                           implemenation may not check this parameter on existence in registry
*                           if handler id exists, function may be used to update unformation about handler
*                           all needed uniqueness checks made by API callers if needed
* @param content_handler_friendly_appname  the user-friendly application name of this content handler
* @param suite_id identifier of the suite or bundle where content handler is located
* @param class_name content handler class name
* @param flag handler registration type
* @param types handler supported content types array, can be null
* @param nTypes length of content types array
* @param suffixes handler supported suffixes array, can be null
* @param nSuffixes length of suffixes array
* @param actions handler supported actions array, can be null
* @param nActions length of actions array
* @param locales handler supported locales array, can be null
* @param nLocales length of locales array
* @param action_names action names for every supported action 
*                                  and every supported locale
*                                  should contain full list of actions for first locale than for second locale etc...
* @param nActionNames length of action names array. This value must be equal 
* to @link nActions multiplied by @link nLocales .
* @param access_allowed_ids list of caller application ids (or prefixes to ids) that have allowed access to invoke this handler, can be null
* @param nAccesses length of accesses list
* @return JAVACALL_OK if operation was successful, error code otherwise
*/
javacall_result javacall_chapi_register_handler(
	javacall_const_utf16_string content_handler_id,
	javacall_const_utf16_string content_handler_friendly_appname,
	javacall_const_utf16_string suite_id,
	javacall_const_utf16_string class_name,
	javacall_chapi_handler_registration_type flag,
	javacall_const_utf16_string* content_types,     int nTypes,
	javacall_const_utf16_string* suffixes,  int nSuffixes,
	javacall_const_utf16_string* actions,   int nActions,  
	javacall_const_utf16_string* locales,   int nLocales,
	javacall_const_utf16_string* action_names, int nActionNames,
	javacall_const_utf16_string* access_allowed_ids,  int nAccesses
	)
{
	javacall_result jcres = JAVACALL_FAIL;
	BOOL bRes;
	DWORD dwRes;
	HANDLE hSession = INVALID_HANDLE_VALUE;
	HANDLE hRegDB = INVALID_HANDLE_VALUE;
	HANDLE hTypesDB = INVALID_HANDLE_VALUE;
	HANDLE hSuffixesDB = INVALID_HANDLE_VALUE;
	HANDLE hActionsDB = INVALID_HANDLE_VALUE;
	HANDLE hLActionsDB = INVALID_HANDLE_VALUE;
	HANDLE hAccessDB = INVALID_HANDLE_VALUE;
	CEPROPVAL* pPropVals = NULL;
	DWORD dwSize = 0;
	DWORD dwHandlerOID = 0;


	/*checks*/
	if (!content_handler_id || !*content_handler_id) 
	{
		log_error(L"Null or empty content handler name is not allowed!");
		return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
	}

	if (nLocales * nActions != nActionNames)
	{
		log_error(L"Local action names number should be actions number multiplied on locales number!");
		return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
	}

	hSession = CeCreateSession(&volGUID);
	if (hSession == INVALID_HANDLE_VALUE)
	{
		log_win32_error(GetLastError());
		goto final;
	}

	bRes = CeBeginTransaction(hSession,CEDB_ISOLEVEL_DEFAULT);
	if (!bRes)
	{
		log_win32_error(GetLastError());
		goto final;
	}

	dwRes = open_handlers_db(hSession, NO_SORT, FALSE, &hRegDB);
	if (dwRes)
	{
		log_win32_error(dwRes);
		goto final;
	}
	else
	/*add handler record*/
	{
		CEPROPVAL propVals[5] = {0};
		WORD nProps = 0;
		CEPROPID propIds[1] = {PROPID_REGISTRY_HANDLER_OID};
		int i;

		propVals[nProps].propid = PROPID_REGISTRY_HANDLER_NAME;
		propVals[nProps++].val.lpwstr = (LPWSTR)content_handler_id;

		if (content_handler_friendly_appname)
		{
			propVals[nProps].propid = PROPID_REGISTRY_HANDLER_APPNAME;
			propVals[nProps++].val.lpwstr = (LPWSTR)content_handler_friendly_appname;
		}

		if (suite_id)
		{
			propVals[nProps].propid = PROPID_REGISTRY_SUITEID;
			propVals[nProps++].val.lpwstr = (LPWSTR)suite_id;
		}

		if (class_name)
		{
			propVals[nProps].propid = PROPID_REGISTRY_CLASSNAME;
			propVals[nProps++].val.lpwstr = (LPWSTR)class_name;
		}

		if (flag)
		{
			propVals[nProps].propid = PROPID_REGISTRY_FLAG;
			propVals[nProps++].val.lVal = flag;
		}

		dwRes = CeWriteRecordProps(hRegDB, 0, nProps, propVals);
		if (!dwRes)
		{
			log_win32_error(GetLastError());
			goto final;
		}

		/* Read newly create handler OID */
		nProps = 1;
		dwRes = CeReadRecordPropsEx(hRegDB, CEDB_ALLOWREALLOC, &nProps, propIds, (LPBYTE*)&pPropVals, &dwSize, GetProcessHeap());
		if (!dwRes)
		{
			log_win32_error(GetLastError());
			goto final;
		}

		dwHandlerOID = pPropVals[0].val.ulVal;

		if (nTypes && content_types)
		{

			dwRes = open_contenttype_db(hSession, NO_SORT, FALSE, &hTypesDB);
			if (dwRes)
			{
				log_win32_error(dwRes);
				goto final;
			}

			for (i = 0; i < nTypes; ++i)
			{
				nProps = 0;
				propVals[nProps].propid = PROPID_CONTENTTYPE_NAME;
				propVals[nProps++].val.lpwstr = (LPWSTR)content_types[i];

				propVals[nProps].propid = PROPID_CONTENTTYPE_HANDLER_OID;
				propVals[nProps++].val.lVal = dwHandlerOID;

				dwRes = CeWriteRecordProps(hTypesDB, 0, nProps, propVals);
				if (!dwRes)
				{
					log_win32_error(GetLastError());
					goto final;
				}
			}
		}

		if (nSuffixes && suffixes)
		{
			dwRes = open_suffixes_db(hSession, NO_SORT, FALSE, &hSuffixesDB);
			if (dwRes)
			{
				log_win32_error(dwRes);
				goto final;
			}

			for (i = 0; i < nSuffixes; ++i)
			{
				nProps = 0;
				propVals[nProps].propid = PROPID_SUFFIX_NAME;
				propVals[nProps++].val.lpwstr = (LPWSTR)suffixes[i];

				propVals[nProps].propid = PROPID_SUFFIX_HANDLER_OID;
				propVals[nProps++].val.lVal = dwHandlerOID;

				dwRes = CeWriteRecordProps(hSuffixesDB, 0, nProps, propVals);
				if (!dwRes)
				{
					log_win32_error(GetLastError());
					goto final;
				}
			}
		}

		if (nActions && actions)
		{
			CEPROPID propidsA[] = {PROPID_ACTION_OID};
			DWORD dwActionOID;

			dwRes = open_actions_db(hSession, NO_SORT, FALSE, &hActionsDB);
			if (dwRes)
			{
				log_win32_error(dwRes);
				goto final;
			}

			for (i = 0; i < nActions; ++i)
			{
				nProps = 0;
				propVals[nProps].propid = PROPID_ACTION_NAME;
				propVals[nProps++].val.lpwstr = (LPWSTR)actions[i];

				propVals[nProps].propid = PROPID_ACTION_HANDLER_OID;
				propVals[nProps++].val.lVal = dwHandlerOID;

				dwRes = CeWriteRecordProps(hActionsDB, 0, nProps, propVals);
				if (!dwRes)
				{
					log_win32_error(GetLastError());
					goto final;
				}

				if (nLocales && locales && nActionNames && action_names) 
				{
					int j;
					/* Read newly create action OID */
					nProps = 1;
					dwRes = CeReadRecordPropsEx(hActionsDB, CEDB_ALLOWREALLOC, &nProps, propidsA, (LPBYTE*)&pPropVals, &dwSize, GetProcessHeap());
					if (!dwRes)
					{
						log_win32_error(GetLastError());
						goto final;
					}
					dwActionOID	= pPropVals[0].val.ulVal;

					if (hLActionsDB == INVALID_HANDLE_VALUE)
					{
						dwRes = open_action_local_names_db(hSession,NO_SORT, FALSE, &hLActionsDB);
						if (dwRes)
						{
							log_win32_error(dwRes);
							goto final;
						}
					}

					/*save local action names*/
					for ( j = 0; j < nLocales; ++j)
					{
						nProps = 0;
						propVals[nProps].propid = PROPID_LACTION_LOCALE;
						propVals[nProps++].val.lpwstr = (LPWSTR)locales[j];

						propVals[nProps].propid = PROPID_LACTION_ACTION_OID;
						propVals[nProps++].val.lVal = dwActionOID;

						propVals[nProps].propid = PROPID_LACTION_ACTION_NAME;
						propVals[nProps++].val.lpwstr = (LPWSTR)action_names[j * nLocales + i];

						dwRes = CeWriteRecordProps(hLActionsDB, 0, nProps, propVals);
						if (!dwRes)
						{
							log_win32_error(GetLastError());
							goto final;
						}
					}
				}
			}
		}

		if (nAccesses && access_allowed_ids)
		{
			dwRes = open_access_db(hSession, NO_SORT, FALSE, &hAccessDB);
			if (dwRes)
			{
				log_win32_error(GetLastError());
				goto final;
			}

			for (i = 0; i < nAccesses; ++i)
			{
				nProps = 0;
				propVals[nProps].propid = PROPID_ACCESS_CALLER_ID;
				propVals[nProps++].val.lpwstr = (LPWSTR)access_allowed_ids[i];

				propVals[nProps].propid = PROPID_ACCESS_HANDLER_OID;
				propVals[nProps++].val.lVal = dwHandlerOID;

				dwRes = CeWriteRecordProps(hAccessDB, 0, nProps, propVals);
				if (!dwRes)
				{
					log_win32_error(GetLastError());
					goto final;
				}
			}
		}

	}

	/* commit all changes */
	bRes = CeEndTransaction(hSession, TRUE);
	if (!bRes)
	{
		log_win32_error(GetLastError());
		goto final;
	}

	jcres = JAVACALL_OK;
final:
	if (hSession != INVALID_HANDLE_VALUE) CloseHandle(hSession);
	if (hRegDB != INVALID_HANDLE_VALUE) CloseHandle(hRegDB);
	if (hTypesDB != INVALID_HANDLE_VALUE) CloseHandle(hTypesDB);
	if (hSuffixesDB != INVALID_HANDLE_VALUE) CloseHandle(hSuffixesDB);
	if (hActionsDB != INVALID_HANDLE_VALUE) CloseHandle(hActionsDB);
	if (hLActionsDB != INVALID_HANDLE_VALUE) CloseHandle(hLActionsDB);
	if (hAccessDB != INVALID_HANDLE_VALUE) CloseHandle(hAccessDB);
	if (pPropVals) HeapFree(GetProcessHeap(),0,pPropVals);
	
	return jcres;
}


/**
* Enumerate all registered content handlers
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
*				  
* @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/


javacall_result javacall_chapi_enum_handlers(int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length)
{
	enum_pos_ptr epos;
	DWORD dwRes;
	DWORD propId[1] = {PROPID_REGISTRY_HANDLER_NAME};
	WORD count = 1;
	CEPROPVAL* pPropVal = NULL;
	DWORD dwSize = 0;
	HANDLE hRegDB = INVALID_HANDLE_VALUE;

	if (!pos_id || !length || (*length && !handler_id_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
	epos = (enum_pos_ptr) *pos_id;

	if (!epos)
	{
		dwRes = open_handlers_db(NULL,NO_SORT, TRUE, &hRegDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}

		/* seek to first record */
		if (!CeSeekDatabaseEx(hRegDB, CEDB_SEEK_BEGINNING, 0, 0, NULL))
		{
			CloseHandle(hRegDB);
			return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
		}

		/* read first record */
		if (!CeReadRecordPropsEx(hRegDB, CEDB_ALLOWREALLOC, &count, 
			(CEPROPID*)propId, (LPBYTE*)&pPropVal, &dwSize, GetProcessHeap()))
		{
			log_win32_error(GetLastError());
			CloseHandle(hRegDB);
			return JAVACALL_FAIL;
		}

		/* allocate position structure */
		epos = (enum_pos_ptr)javacall_malloc(sizeof(enum_pos));
		if (!epos)
		{
			log_error(L"out of memory");
			CloseHandle(hRegDB);
			return JAVACALL_CHAPI_ERROR_NO_MEMORY;
		}

		epos->m_hDB = hRegDB;
		epos->m_pPropVal = pPropVal;
		epos->m_dwSize = dwSize;
		*pos_id = (int)epos;
	} else {
		if (!epos->m_pPropVal  || epos->m_pPropVal->wFlags != CHAPI_REPEAT_RESULT)
		{
			if (!CeSeekDatabaseEx(epos->m_hDB, CEDB_SEEK_CURRENT, 1, 0, NULL))
				return bad_seek(epos->m_hDB,JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS);

			if (!CeReadRecordPropsEx(epos->m_hDB, CEDB_ALLOWREALLOC, &count, 
				(CEPROPID*)propId, (LPBYTE*)&(epos->m_pPropVal), &(epos->m_dwSize), GetProcessHeap()))
			{
				log_win32_error(GetLastError());
				return JAVACALL_FAIL;
			}
		}
	}

	dwRes = copy_result(epos->m_pPropVal,handler_id_out,length);
	epos->m_pPropVal->wFlags = dwRes == JAVACALL_OK ? 0 : CHAPI_REPEAT_RESULT;
	
	return dwRes;
}

/**
* Enumerate registered content handlers that can handle files with given suffix
* Search is case-insensitive
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @suffix suffix of content data file with dot (for example: ".txt" or ".html") for which handlers are searched
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_handlers_by_suffix(javacall_const_utf16_string suffix, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length)
{
	HANDLE hSuffixesDB = INVALID_HANDLE_VALUE;

	if (!suffix || !pos_id || !length || (*length && !handler_id_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		/* open sorted by suffix */
		DWORD dwRes = open_suffixes_db(NULL,0, TRUE, &hSuffixesDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	return enum_handlers_by_indexed_value(hSuffixesDB, suffix, PROPID_SUFFIX_NAME, CEDB_SEEK_VALUEFIRSTEQUAL, PROPID_SUFFIX_HANDLER_OID, 
		pos_id, handler_id_out, length, 0);
}

/** 
* Enumerate registered content handlers that can handle content with given content type
* Search is case-insensitive
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @content_type type of content data for which handlers are searched
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_handlers_by_type(javacall_const_utf16_string content_type, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length)
{
	HANDLE hTypesDB = INVALID_HANDLE_VALUE;

	if (!content_type || !pos_id || !length || (*length && !handler_id_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		/* open sorted by content type */
		DWORD dwRes = open_contenttype_db(NULL,0, TRUE, &hTypesDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	return enum_handlers_by_indexed_value(hTypesDB, content_type, PROPID_CONTENTTYPE_NAME, CEDB_SEEK_VALUEFIRSTEQUAL, PROPID_CONTENTTYPE_HANDLER_OID,
		pos_id, handler_id_out, length, 0);
}

/**
* Enumerate registered content handlers that can perform given action
* Search is case-sensitive
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @action action that handler can perform against content
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_handlers_by_action(javacall_const_utf16_string action, int* pos_id, /*OUT*/ javacall_utf16*  handler_id_out, int* length)
{
	HANDLE hActionsDB = INVALID_HANDLE_VALUE;

	if (!action || !pos_id || !length || (*length && !handler_id_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		/* open sorted by action name */
		DWORD dwRes = open_actions_db(NULL,0, TRUE, &hActionsDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	return enum_handlers_by_indexed_value(hActionsDB, action, PROPID_ACTION_NAME, CEDB_SEEK_VALUEFIRSTEQUAL, PROPID_ACTION_HANDLER_OID,
		pos_id, handler_id_out, length, 0);
}


/**
* Enumerate registered content handlers located in suite (bundle) with given suite id
* Search is case-sensitive
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @suite_id suite id for which content handlers are searched
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_handlers_by_suite_id(
	javacall_const_utf16_string suite_id,
	int* pos_id, 
	/*OUT*/ javacall_utf16*  handler_id_out,
	int* length)
{
	HANDLE hHandlerDB = INVALID_HANDLE_VALUE;

	if (!suite_id || !pos_id || !length || (*length && !handler_id_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		/* open sorted by suite_id */
		DWORD dwRes = open_handlers_db(NULL, 2, TRUE, &hHandlerDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	return enum_handlers_by_indexed_value(hHandlerDB, suite_id, PROPID_REGISTRY_SUITEID, CEDB_SEEK_VALUEFIRSTEQUAL, PROPID_REGISTRY_HANDLER_NAME,
		pos_id, handler_id_out, length, 0);
}



/**
* Enumerate registered content handler IDs that have the id parameter as a prefix
* Search is case-sensitive
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @param id      a string used for registered content handlers searching
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeration
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_handlers_by_prefix(javacall_const_utf16_string id, 
													   int* pos_id, /*OUT*/ javacall_utf16* handler_id_out, int* length)
{
	HANDLE hHandlerDB = INVALID_HANDLE_VALUE;
	javacall_result jcres = JAVACALL_OK;

	if (!id || !pos_id || !length || (*length && !handler_id_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		/* open sorted by handler name */
		DWORD dwRes = open_handlers_db(NULL, 1, TRUE, &hHandlerDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	jcres = enum_handlers_by_indexed_value(hHandlerDB, id, PROPID_REGISTRY_HANDLER_NAME, CEDB_SEEK_VALUEGREATEROREQUAL, PROPID_REGISTRY_HANDLER_NAME,
		pos_id, handler_id_out, length, 0);

	if (jcres == JAVACALL_OK || jcres == JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL)
	{
		if (jcres == JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL)
		{
			enum_pos_ptr epos = (enum_pos_ptr)*pos_id;
			if (compare_prefix_with_case(id, epos->m_pPropVal->val.lpwstr)) return jcres;
		} else {
			if (compare_prefix_with_case(id, (LPCWSTR)handler_id_out)) return jcres;
		}
		*length = 0;
		return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	}

	return jcres;
}

/**
* Enumerate registered content handler IDs that are a prefix of the 'id' parameter.
* Content handler ID equals to the 'id' parameter if exists must be included in returned sequence.
* Search is case-sensitive
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @param id      a string used for registered content handlers searching
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeration
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param handler_id_out memory buffer receiving zero terminated string containing single handler id 
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_handlers_prefixes_of(javacall_const_utf16_string id, 
														 int* pos_id, /*OUT*/ javacall_utf16* handler_id_out, int* length)
{
	HANDLE hHandlerDB = INVALID_HANDLE_VALUE;
	javacall_result jcres = JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	int orgLength = *length;
	int len;
	javacall_utf16_string szCutId;
	int new_pos_id;

	if (!id || !pos_id || !length || (*length && !handler_id_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(new_pos_id = (*pos_id)))
	{
		/* open sorted by handler name */
		DWORD dwRes = open_handlers_db(NULL, 1, TRUE, &hHandlerDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	} else {
		enum_pos_ptr epos = (enum_pos_ptr)*pos_id;
		if (epos->m_pPropVal)
		{
			enum_pos_ptr epos = (enum_pos_ptr)*pos_id;
			if (!epos->m_pPropVal->val.lpwstr || !epos->m_pPropVal->val.lpwstr[0]) return  JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			// previous result exists, use it in the next search
			id = (javacall_const_utf16_string) epos->m_pPropVal->val.lpwstr;
		}
	}

	len = wcslen(id);
	szCutId = (javacall_utf16_string)javacall_malloc((len + 1) * sizeof(javacall_utf16));
	if (!szCutId) return JAVACALL_CHAPI_ERROR_NO_MEMORY;
	memcpy(szCutId, id, (len + 1) * sizeof(javacall_utf16));

	while (len) 
	{
		*length = orgLength;
		jcres = enum_handlers_by_indexed_value(hHandlerDB, szCutId, PROPID_REGISTRY_HANDLER_NAME, CEDB_SEEK_VALUEFIRSTEQUAL, PROPID_REGISTRY_HANDLER_NAME,
			&new_pos_id, handler_id_out, length, 1);

		if (jcres != JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS) break;
		
		/* not found such results, will try smaller one */
		if (new_pos_id)
		{
			enum_pos_ptr epos = (enum_pos_ptr)new_pos_id;

			/* store opened db */
			hHandlerDB = epos->m_hDB;
			epos->m_hDB = INVALID_HANDLE_VALUE;

			/* remember last pos_id if it is new */
			if (*pos_id != new_pos_id)
			{
				javacall_chapi_enum_finish(*pos_id);
				*pos_id = new_pos_id;
			}
		}
		/* reset search */
		new_pos_id = 0;

		/* with smaller name */
		szCutId[--len] = 0;
	}

	if (!len && !new_pos_id)
	{
		CloseHandle(hHandlerDB);
	}

	if (*pos_id != new_pos_id)
	{
		javacall_chapi_enum_finish(*pos_id);
		*pos_id = new_pos_id;
	}
	javacall_free(szCutId);
	return jcres;
}

/**
* Enumerate all suffixes of content type data files that can be handled by given content handler or all suffixes 
* registered in registry
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @content_handler_id unique id of content handler for which content data files suffixes are requested
*                     if  content_handler_id is null suffixes for all registered handlers are enumerated
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param suffix_out memory buffer receiving zero terminated string containing content data file suffix
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_suffixes(javacall_const_utf16_string content_handler_id, int* pos_id, /*OUT*/ javacall_utf16*  suffix_out, int* length)
{
	HANDLE hDB = INVALID_HANDLE_VALUE;
	DWORD handlerOID = 0;

	if (!content_handler_id || !content_handler_id[0] || !pos_id || !length || (*length && !suffix_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		DWORD dwRes;

		dwRes = get_handler_by_name(content_handler_id, &handlerOID, NULL);
		if (dwRes) return dwRes;

		/* open sorted by handler oid */
		dwRes = open_suffixes_db(NULL, 1, TRUE, &hDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	return enum_values_by_handler_id(hDB, handlerOID, PROPID_SUFFIX_HANDLER_OID, PROPID_SUFFIX_NAME, pos_id, suffix_out, length);
}

/**
* Enumerate all content data types that can be handled by given content handler or all content types
* registered in registry
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @content_handler_id unique id of content handler for which content data types are requested
*                     if  content_handler_id is null all registered content types are enumerated
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param type_out memory buffer receiving zero terminated string containing single content type
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_types(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16*  type_out, int* length)
{
	HANDLE hDB = INVALID_HANDLE_VALUE;
	DWORD handlerOID = 0;

	if (!content_handler_id || !content_handler_id[0] || !pos_id || !length || (*length && !type_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		DWORD dwRes;

		dwRes = get_handler_by_name(content_handler_id, &handlerOID, NULL);
		if (dwRes) return dwRes;

		/* open sorted by handler oid */
		dwRes = open_contenttype_db(NULL, 1, TRUE, &hDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	return enum_values_by_handler_id(hDB, handlerOID, PROPID_CONTENTTYPE_HANDLER_OID, PROPID_CONTENTTYPE_NAME, pos_id, type_out, length);
}


/**
* Enumerate all actions that can be performed by given content handler with any acceptable content
* or all possible actions mentioned in registry for all registered content handlers and types
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @content_handler_id unique id of content handler for which possible actions are requested
*                     if  content_handler_id is null all registered actions are enumerated
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param action_out memory buffer receiving zero terminated string containing single action name
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_actions(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16*  action_out, int* length)
{
	HANDLE hDB = INVALID_HANDLE_VALUE;
	DWORD handlerOID = 0;

	if (!content_handler_id || !content_handler_id[0] || !pos_id || !length || (*length && !action_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		DWORD dwRes;

		dwRes = get_handler_by_name(content_handler_id, &handlerOID, NULL);
		if (dwRes) return dwRes;

		/* open sorted by handler oid */
		dwRes = open_actions_db(NULL, 1, TRUE, &hDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;

			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	return enum_values_by_handler_id(hDB, handlerOID, PROPID_ACTION_HANDLER_OID, PROPID_ACTION_NAME, pos_id, action_out, length);
}

/**
* Enumerate all locales for witch localized names of actions that can be performed by given handler exist in registry.
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @content_handler_id unique id of content handler for which possible locales are requested
*                     in this method content handler id should not be null
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param locale_out memory buffer receiving zero terminated string containing single supported locale
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_action_locales(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16* locale_out, int* length)
{
	HANDLE hDB = INVALID_HANDLE_VALUE;
	DWORD dwActionOID = 0;

	if (!content_handler_id || !content_handler_id[0] || !pos_id || !length || (*length && !locale_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		DWORD dwRes;
		HANDLE hActionsDB = INVALID_HANDLE_VALUE;
		CEPROPVAL propValOID = {PROPID_ACTION_HANDLER_OID};
		DWORD dwHandlerOID = 0;
		
		

		/* find handle oid by name */
		dwRes = get_handler_by_name(content_handler_id, &dwHandlerOID, NULL);
		if (dwRes) return dwRes;


		/* get any first action id of for this handler */
		dwRes = open_actions_db(NULL, 1, TRUE, &hActionsDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}


		propValOID.val.ulVal = dwHandlerOID;

		if (!CeSeekDatabaseEx(hActionsDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 1, NULL))
		{
			return bad_seek(hActionsDB,JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS);
		}
		else
		{
			CEPROPID propidsA[] = {PROPID_ACTION_OID};
			CEPROPVAL* propVal = NULL;
			DWORD dwSize = 0;
			WORD nProps = 1;

			dwRes = CeReadRecordPropsEx(hActionsDB, CEDB_ALLOWREALLOC, &nProps, propidsA, (LPBYTE*)&propVal, &dwSize, GetProcessHeap());
			if (!dwRes)
			{
				CloseHandle(hActionsDB);
				log_win32_error(GetLastError());
				return JAVACALL_FAIL;
			}

			dwActionOID = propVal->val.ulVal;
			HeapFree(GetProcessHeap(),0,propVal);

			CloseHandle(hActionsDB);
		}

		/* open local names database sorted by action oid */
		dwRes = open_action_local_names_db(NULL, 0, FALSE, &hDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	/* enumerate locales for found action OID */
	return enum_values_by_handler_id(hDB, dwActionOID, PROPID_LACTION_ACTION_OID, PROPID_LACTION_LOCALE, pos_id, locale_out, length);
}

/**
* Get localized name of actions that can be performed by given handler
*
* @content_handler_id unique id of content handler for which localized action name is requested
* @action standard (local neutral) name of action for which localized name is requested
* @locale name of locale for which name of action is requested,
locale consist of two small letters containing ISO-639 language code and two upper letters containg ISO-3166 country code devided by "-" possibly extended by variant
* @param local_action_out memory buffer receiving zero terminated string containing local action name
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_get_local_action_name(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string action, javacall_const_utf16_string locale, javacall_utf16*  local_action_out, int* length)
{
	HANDLE hDB = INVALID_HANDLE_VALUE;
	DWORD dwRes;
	HANDLE hActionsDB = INVALID_HANDLE_VALUE;
	CEPROPVAL propValOID[2] = {{PROPID_ACTION_HANDLER_OID}, {PROPID_ACTION_NAME}};
	DWORD dwHandlerOID = 0;
	DWORD dwActionOID;

	if (!content_handler_id || !content_handler_id[0] || !action || !locale || !length || (*length && !local_action_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	/* find handle oid by name */
	dwRes = get_handler_by_name(content_handler_id, &dwHandlerOID, NULL);
	if (dwRes) return dwRes;

	/* open actions db sorted by handler oid and action name */
	dwRes = open_actions_db(NULL, 1, TRUE, &hActionsDB);
	if (dwRes)
	{
		if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
		log_win32_error(dwRes);
		return JAVACALL_FAIL;
	}

	propValOID[0].val.ulVal = dwHandlerOID;
	propValOID[1].val.lpwstr = (LPWSTR)action;

	if (!CeSeekDatabaseEx(hActionsDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 2, NULL))
	{
		return bad_seek(hActionsDB,JAVACALL_CHAPI_ERROR_NOT_FOUND);
	}
	else
	{
		CEPROPID propidsA[] = {PROPID_ACTION_OID};
		CEPROPVAL* propVal = NULL;
		DWORD dwSize = 0;
		WORD nProps = 1;

		dwRes = CeReadRecordPropsEx(hActionsDB, CEDB_ALLOWREALLOC, &nProps, propidsA, (LPBYTE*)&propVal, &dwSize, GetProcessHeap());
		if (!dwRes)
		{
			CloseHandle(hActionsDB);
			log_win32_error(GetLastError());
			return JAVACALL_FAIL;
		}

		dwActionOID = propVal->val.ulVal;
		CloseHandle(hActionsDB);
		HeapFree(GetProcessHeap(),0,propVal);
		propVal = NULL;
		dwSize = 0;

		/* open local names database sorted by action oid */
		dwRes = open_action_local_names_db(NULL, 0, FALSE, &hDB);
		if (dwRes)
		{
			if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		} else 
		{
			CEPROPVAL propValLActions[2] = {{PROPID_LACTION_ACTION_OID}, {PROPID_LACTION_LOCALE}};
			propValLActions[0].val.ulVal = dwActionOID;
			propValLActions[1].val.lpwstr = (LPWSTR)locale;
			if (!CeSeekDatabaseEx(hDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValLActions, 2, NULL))
			{
				return bad_seek(hDB,JAVACALL_CHAPI_ERROR_NOT_FOUND);
			} else {
				CEPROPID propidsLN[] = {PROPID_LACTION_ACTION_NAME};
				nProps = 1;

				dwRes = CeReadRecordPropsEx(hDB, CEDB_ALLOWREALLOC, &nProps, propidsLN, (LPBYTE*)&propVal, &dwSize, GetProcessHeap());
				if (!dwRes)
				{
					dwRes = GetLastError();
					CloseHandle(hDB);
					log_win32_error(dwRes);
					return JAVACALL_FAIL;
				}
				CloseHandle(hDB);
				dwRes = copy_result(propVal,local_action_out,length);
				HeapFree(GetProcessHeap(),0,propVal);
				return dwRes;
			}
		}
	}
}

/**
* Enumerate all caller names that accessible to invoke given handler
* Function should be called sequentially until JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS is returned
* Returned between calls values are not guaranteed to be unique, it is up to caller to extract unique values if required
*
* @content_handler_id unique id of content handler for which callers list is requested
*                     content handler id should not be null
* @param pos_id  pointer to integer that keeps postion's information in enumeration
*                before first call integer pointed by pos_id must be initialized to zero, 
*                each next call it should have value returned in previous call
*                pos_id value is arbitrary number or pointer to some allocated structure and not is index or position in enumeraion
*                its value should not be interpreted by caller in any way
*                Method javacall_chapi_enum_finish is called after last enum method call allowing implementation to clean allocated data
*                If function returns error value pointed by pos_id MUST not be updated
* @param access_allowed_out memory buffer receiving zero terminated string containing single access allowed caller id
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS if no more elements to return,
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_enum_access_allowed_callers(javacall_const_utf16_string content_handler_id, int* pos_id, /*OUT*/ javacall_utf16*  access_allowed_out, int* length)
{
	HANDLE hDB = INVALID_HANDLE_VALUE;
	DWORD handlerOID = 0;

	if (!content_handler_id || !content_handler_id[0] || !pos_id || !length || (*length && !access_allowed_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!(*pos_id))
	{
		DWORD dwRes;

		dwRes = get_handler_by_name(content_handler_id, &handlerOID, NULL);
		if (dwRes) return dwRes;

		/* open sorted by handler oid */
		dwRes = open_access_db(NULL, 1, FALSE, &hDB);
		if (dwRes)
		{
			log_win32_error(dwRes);
			return JAVACALL_FAIL;
		}
	}

	return enum_values_by_handler_id(hDB, handlerOID, PROPID_ACCESS_HANDLER_OID, PROPID_ACCESS_CALLER_ID, pos_id, access_allowed_out, length);
}



/**
* Get the user-friendly application name of given content handler
*
* @content_handler_id unique id of content handler for which application name is requested
* @param handler_frienfly_appname_out memory buffer receiving zero terminated string containing user-friendly application name of handler
* @param length pointer to integer initialized by caller to length of buffer,
*               on return it set to length of data passing to buffer including the terminating zero
*               if length of buffer is not enough to keep all data, length is set to minimum needed size and
*               JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL is returned
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if buffer too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_get_content_handler_friendly_appname(javacall_const_utf16_string content_handler_id, /*OUT*/ javacall_utf16*  handler_frienfly_appname_out, int* length)
{
	HANDLE hRegDB = INVALID_HANDLE_VALUE;
	CEPROPVAL propValName = {PROPID_REGISTRY_HANDLER_NAME};
	CEPROPID propIds[1] = {PROPID_REGISTRY_HANDLER_APPNAME};
	CEPROPVAL* pPropVals = NULL;
	DWORD dwSize = 0;
	DWORD dwRes;
	WORD nProps = 1;

	if (!content_handler_id || !content_handler_id[0] || !handler_frienfly_appname_out || !length || (*length && !handler_frienfly_appname_out)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;


	dwRes = open_handlers_db(NULL, 1, TRUE, &hRegDB);
	if (dwRes)
	{
		if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
		log_win32_error(dwRes);
		return JAVACALL_FAIL;
	}

	propValName.val.lpwstr = (LPWSTR)content_handler_id;
	if (!CeSeekDatabaseEx(hRegDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValName, 1, NULL))
		return bad_seek(hRegDB,JAVACALL_CHAPI_ERROR_NOT_FOUND);
	

	dwRes = CeReadRecordPropsEx(hRegDB, CEDB_ALLOWREALLOC, &nProps, propIds, (LPBYTE*)&pPropVals, &dwSize, GetProcessHeap());
	if (!dwRes)
	{
		dwRes = GetLastError();
		log_win32_error(dwRes);
		return JAVACALL_FAIL;
	}

	dwRes = copy_result(pPropVals,handler_frienfly_appname_out,length);

	HeapFree(GetProcessHeap(),0,pPropVals);
	CloseHandle(hRegDB);
	return dwRes;
}

/**
* Get the combined location information of given content handler
* returns suite_id, classname, and integer flag describing handler application
*
* @content_handler_id unique id of content handler for which application name is requested
*                     content handler id is case sensitive
* @param suite_id_out buffer receiving suite_id of handler, can be null
*                     for native platform handlers suite_id is zero
*                     if suite_id_out is null suite_id is not retrieved 
* @param classname_out buffer receiving zero terminated string containing classname of handler, can be null
*                      for native platform handlers classname is full pathname to native application
*                      if classname_out is null class name is not retrieved 
* @param classname_len pointer to integer initialized by caller to length of classname buffer
* @param flag_out pointer to integer receiving handler registration type, can be null
*                 if flag_out is null registration flag is not retrieved 
* @return JAVACALL_OK if operation was successful, 
*         JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL if output buffer lenght is too small to keep result
*         error code if failure occurs
*/
javacall_result javacall_chapi_get_handler_info(javacall_const_utf16_string content_handler_id,
												/*OUT*/
												javacall_utf16*  suite_id_out, int* suite_id_len,
												javacall_utf16*  classname_out, int* classname_len,
												javacall_chapi_handler_registration_type *flag_out)
{
	HANDLE hRegDB = INVALID_HANDLE_VALUE;
	CEPROPVAL propValName = {PROPID_REGISTRY_HANDLER_NAME};
	CEPROPID propIds[3];
	CEPROPVAL* pPropVals = NULL;
	DWORD dwSize = 0;
	DWORD dwRes;
	WORD nProps = 0;
	javacall_result jcres = JAVACALL_OK;

	if (!content_handler_id || !content_handler_id[0]) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;


	if (suite_id_out || suite_id_len)
	{
		if (!suite_id_out && suite_id_len && *suite_id_len != 0) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
		if (suite_id_out && !suite_id_len) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
		propIds[nProps++] = PROPID_REGISTRY_SUITEID;
	}

	if (classname_out || classname_len)
	{
		if (!classname_out && classname_len && *classname_len != 0) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
		if (classname_out && !classname_len) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
		propIds[nProps++] = PROPID_REGISTRY_CLASSNAME;
	}

	if (flag_out)
	{
		propIds[nProps++] = PROPID_REGISTRY_FLAG;
	}
	

	dwRes = open_handlers_db(NULL, 1, TRUE, &hRegDB);
	if (dwRes)
	{
		if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
		log_win32_error(dwRes);
		return JAVACALL_FAIL;
	}

	propValName.val.lpwstr = (LPWSTR)content_handler_id;
	if (!CeSeekDatabaseEx(hRegDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValName, 1, NULL))
		return bad_seek(hRegDB,JAVACALL_CHAPI_ERROR_NOT_FOUND);
	

	dwRes = CeReadRecordPropsEx(hRegDB, CEDB_ALLOWREALLOC, &nProps, propIds, (LPBYTE*)&pPropVals, &dwSize, GetProcessHeap());
	if (!dwRes)
	{
		dwRes = GetLastError();
		log_win32_error(dwRes);
		return JAVACALL_FAIL;
	}

	nProps = 0;

	if (suite_id_out || suite_id_len)
	{
		dwRes = copy_result(&pPropVals[nProps++],suite_id_out,suite_id_len);
		if (!JAVACALL_SUCCEEDED(dwRes)) jcres = dwRes;
	}

	if (classname_out || classname_len)
	{
		dwRes = copy_result(&pPropVals[nProps++],classname_out,classname_len);
		if (!JAVACALL_SUCCEEDED(dwRes)) jcres = dwRes;
	}

	if (flag_out)
	{
		*flag_out = pPropVals[nProps].val.ulVal;
	}

	HeapFree(GetProcessHeap(),0,pPropVals);
	CloseHandle(hRegDB);
	return jcres;

}

/**
* Check if given caller has allowed access to invoke content handler
*
* @content_handler_id unique id of content handler
* @caller_id tested caller application name
*            caller have access to invoke handler if its id is prefixed or equal to id from access_allowed list passed to register function
*            if access list provided during @link javacall_chapi_register_handler call was empty, all callers assumed to be trusted
* @return JAVACALL_TRUE if caller is trusted
*         JAVACALL_FALSE if caller is not trusted
*/
javacall_bool javacall_chapi_is_access_allowed(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string caller_id)
{
	HANDLE hDB = INVALID_HANDLE_VALUE;
	DWORD dwRes;
	HANDLE hAccessDB = INVALID_HANDLE_VALUE;
	CEPROPVAL propValOID[1] = {PROPID_ACCESS_HANDLER_OID};
	DWORD dwHandlerOID = 0;
	DWORD dwAccessOID;
	CEPROPVAL* propVal = NULL;
	DWORD dwSize = 0;
	javacall_bool jbRes = JAVACALL_FALSE;

	if (!content_handler_id || !content_handler_id[0] || !caller_id || !caller_id[0]) return JAVACALL_FAIL;

	/* find handle oid by name */
	dwRes = get_handler_by_name(content_handler_id, &dwHandlerOID, NULL);
	if (dwRes) return JAVACALL_FALSE;

	/* open access db sorted by handler oid */
	dwRes = open_access_db(NULL, 1, TRUE, &hAccessDB);
	if (dwRes)
	{
		if (dwRes != ERROR_FILE_NOT_FOUND) 
		{
			log_win32_error(dwRes);
			return JAVACALL_FALSE;
		}

		/* access list is empty */
		return JAVACALL_TRUE;
	}


	propValOID[0].val.ulVal = dwHandlerOID;

	if (!(dwAccessOID = CeSeekDatabaseEx(hAccessDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 1, NULL)))
	{

		dwRes = GetLastError();
		if (dwRes != ERROR_SEEK) 
		{
			log_win32_error(dwRes);
			return JAVACALL_FALSE;
		}
		/* access list is empty */
		return JAVACALL_TRUE;
	}
	else
	{
		while(dwAccessOID)
		{
			CEPROPID propidsA[] = {PROPID_ACCESS_CALLER_ID};
			WORD nProps = 1;

			dwRes = CeReadRecordPropsEx(hAccessDB, CEDB_ALLOWREALLOC, &nProps, propidsA, (LPBYTE*)&propVal, &dwSize, GetProcessHeap());
			if (!dwRes)
			{
				CloseHandle(hAccessDB);
				log_win32_error(GetLastError());
				return JAVACALL_FALSE;
			}

			if (compare_prefix_with_case(propVal->val.lpwstr,caller_id))
			{
				jbRes = JAVACALL_TRUE;
				break;
			}

			dwAccessOID = CeSeekDatabaseEx(hAccessDB, CEDB_SEEK_VALUENEXTEQUAL, 1, 0, NULL);
		}
	}

	CloseHandle(hAccessDB);
	if (propVal) HeapFree(GetProcessHeap(),0,propVal);
	return jbRes;
}

/**
* Check if given action is supported by given content handler
* @content_handler_id unique id of content handler
* @action tested action name (local neutral name)
* @return JAVACALL_TRUE if caller is trusted
*         JAVACALL_FALSE if caller is not trusted
*/
javacall_bool javacall_chapi_is_action_supported(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string action)
{
	HANDLE hDB = INVALID_HANDLE_VALUE;
	DWORD dwRes;
	HANDLE hActionsDB = INVALID_HANDLE_VALUE;
	CEPROPVAL propValOID[2] = {{PROPID_ACTION_HANDLER_OID}, {PROPID_ACTION_NAME}};
	DWORD dwHandlerOID = 0;
	javacall_bool jbres = JAVACALL_FALSE;

	if (!content_handler_id || !content_handler_id[0] || !action) return JAVACALL_FALSE;

	/* find handle oid by name */
	dwRes = get_handler_by_name(content_handler_id, &dwHandlerOID, NULL);
	if (dwRes) return JAVACALL_FALSE;

	/* open actions db sorted by handler oid and action name */
	dwRes = open_actions_db(NULL, 1, TRUE, &hActionsDB);
	if (dwRes)
	{
		if (dwRes == ERROR_FILE_NOT_FOUND) return JAVACALL_FALSE;
		log_win32_error(dwRes);
		return JAVACALL_FALSE;
	}

	propValOID[0].val.ulVal = dwHandlerOID;
	propValOID[1].val.lpwstr = (LPWSTR)action;

	if (CeSeekDatabaseEx(hActionsDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 2, NULL))
	{
		jbres = JAVACALL_TRUE;
	}

	CloseHandle(hActionsDB);
	return jbres;
}

/**
* Remove all information about handler from registry
*
* @param content_handler_id unique ID of content handler
* @return JAVACALL_OK if operation was successful, error code otherwise
*/
javacall_result javacall_chapi_unregister_handler(javacall_const_utf16_string content_handler_id)
{
	javacall_result jcres = JAVACALL_FAIL;
	BOOL bRes;
	DWORD dwRes;
	HANDLE hSession = INVALID_HANDLE_VALUE;
	HANDLE hRegDB = INVALID_HANDLE_VALUE;
	HANDLE hTypesDB = INVALID_HANDLE_VALUE;
	HANDLE hSuffixesDB = INVALID_HANDLE_VALUE;
	HANDLE hActionsDB = INVALID_HANDLE_VALUE;
	HANDLE hLActionsDB = INVALID_HANDLE_VALUE;
	HANDLE hAccessDB = INVALID_HANDLE_VALUE;
	CEPROPVAL* pPropVals = NULL;
	DWORD dwSize = 0;
	CEPROPVAL propValOID = {0};
	DWORD handlerRecOID = 0;

	dwRes = get_handler_by_name(content_handler_id, &(propValOID.val.ulVal), &handlerRecOID);
	if (dwRes) return dwRes;

	hSession = CeCreateSession(&volGUID);
	if (hSession == INVALID_HANDLE_VALUE)
	{
		log_win32_error(GetLastError());
		goto final;
	}

	bRes = CeBeginTransaction(hSession,CEDB_ISOLEVEL_DEFAULT);
	if (!bRes)
	{
		log_win32_error(GetLastError());
		goto final;
	}

	/* clean information in handler db */
	dwRes = open_handlers_db(hSession, NO_SORT, TRUE, &hRegDB);
	if (dwRes && dwRes != ERROR_FILE_NOT_FOUND)
	{
		log_win32_error(dwRes);
		goto final;
	}
	/* remove handler record */
	if (!CeDeleteRecord(hRegDB, handlerRecOID)){
		log_win32_error(GetLastError());
		goto final;
	}

	/* clean information in access db if it exists */
	dwRes = open_access_db(hSession, 1, TRUE, &hAccessDB);
	if (dwRes && dwRes != ERROR_FILE_NOT_FOUND)
	{
		log_win32_error(dwRes);
		goto final;
	}

	propValOID.propid = PROPID_ACCESS_HANDLER_OID;
	if (hAccessDB != INVALID_HANDLE_VALUE) do 
	{
		dwRes = CeSeekDatabaseEx(hAccessDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 1, NULL);
		if (!dwRes) break;

		if (!CeDeleteRecord(hAccessDB, dwRes)){
			log_win32_error(GetLastError());
			goto final;
		}
	} while (dwRes);

	/* clean information in suffixes db if it exists */
	dwRes = open_suffixes_db(hSession, 1, TRUE, &hSuffixesDB);
	if (dwRes && dwRes != ERROR_FILE_NOT_FOUND)
	{
		log_win32_error(dwRes);
		goto final;
	}

	propValOID.propid = PROPID_SUFFIX_HANDLER_OID;
	if (hSuffixesDB != INVALID_HANDLE_VALUE) do 
	{
		dwRes = CeSeekDatabaseEx(hSuffixesDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 1, NULL);
		if (!dwRes) break;

		if (!CeDeleteRecord(hSuffixesDB, dwRes)){
			log_win32_error(GetLastError());
			goto final;
		}
	} while (dwRes);


	/* clean information in types db if it exists */
	dwRes = open_contenttype_db(hSession, 1, TRUE, &hTypesDB);
	if (dwRes && dwRes != ERROR_FILE_NOT_FOUND)
	{
		log_win32_error(dwRes);
		goto final;
	}

	propValOID.propid = PROPID_CONTENTTYPE_HANDLER_OID;
	if (hTypesDB != INVALID_HANDLE_VALUE) do 
	{
		dwRes = CeSeekDatabaseEx(hTypesDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 1, NULL);
		if (!dwRes) break;

		if (!CeDeleteRecord(hTypesDB, dwRes)){
			log_win32_error(GetLastError());
			goto final;
		}
	} while (dwRes);


	/* clean information in Actions db if it exists */
	dwRes = open_actions_db(hSession, 1, TRUE, &hActionsDB);
	if (dwRes && dwRes != ERROR_FILE_NOT_FOUND)
	{
		log_win32_error(dwRes);
		goto final;
	}

	dwRes = open_action_local_names_db(hSession, 0, TRUE, &hLActionsDB);
	if (dwRes && dwRes != ERROR_FILE_NOT_FOUND)
	{
		log_win32_error(dwRes);
		goto final;
	}

	propValOID.propid = PROPID_ACTION_HANDLER_OID;
	if (hActionsDB != INVALID_HANDLE_VALUE) do 
	{
		DWORD dwActionRecOID = CeSeekDatabaseEx(hActionsDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)&propValOID, 1, NULL);
		if (!dwActionRecOID) break;

		if (hLActionsDB != INVALID_HANDLE_VALUE) 
		{
			CEPROPID propidsA[] = {PROPID_ACTION_OID};
			CEPROPVAL* propVal = NULL;
			DWORD dwSize = 0;
			WORD nProps = 1;

			dwRes = CeReadRecordPropsEx(hActionsDB, CEDB_ALLOWREALLOC, &nProps, propidsA, (LPBYTE*)&propVal, &dwSize, GetProcessHeap());
			if (!dwRes)
			{
				log_win32_error(GetLastError());
				goto final;
			}

			do 
			{
				/* remove all local names */
				propVal->propid = PROPID_LACTION_ACTION_OID;
				propVal->wLenData = 0;
				propVal->wFlags = 0;
				dwRes = CeSeekDatabaseEx(hLActionsDB, CEDB_SEEK_VALUEFIRSTEQUAL, (DWORD)propVal, 1, NULL);
				if (!dwRes) break;

				if (!CeDeleteRecord(hLActionsDB, dwRes)){
					log_win32_error(GetLastError());
					goto final;
				}
			} while (dwRes);

			HeapFree(GetProcessHeap(),0,propVal);
		}

		if (!CeDeleteRecord(hActionsDB, dwActionRecOID)){
			log_win32_error(GetLastError());
			goto final;
		}
	} while (1);

	/* commit all changes */
	bRes = CeEndTransaction(hSession, TRUE);
	if (!bRes)
	{
		log_win32_error(GetLastError());
		goto final;
	}

	jcres = JAVACALL_OK;

final:
	if (hSession != INVALID_HANDLE_VALUE) CloseHandle(hSession);
	if (hRegDB != INVALID_HANDLE_VALUE) CloseHandle(hRegDB);
	if (hTypesDB != INVALID_HANDLE_VALUE) CloseHandle(hTypesDB);
	if (hSuffixesDB != INVALID_HANDLE_VALUE) CloseHandle(hSuffixesDB);
	if (hActionsDB != INVALID_HANDLE_VALUE) CloseHandle(hActionsDB);
	if (hLActionsDB != INVALID_HANDLE_VALUE) CloseHandle(hLActionsDB);
	if (hAccessDB != INVALID_HANDLE_VALUE) CloseHandle(hAccessDB);
	if (pPropVals) HeapFree(GetProcessHeap(),0,pPropVals);

	return jcres;
}


/**
* Finish enumeration call. Clean enumeration position handle
* This method is called after caller finished to enumerate by some parameter
* Can be used by implementation to cleanup object referenced by pos_id if required
*
* @param pos_id position handle used by enumeration method call, if pos_id is zero it should be ignored
* @return nothing
*/
void javacall_chapi_enum_finish(int pos_id)
{
	enum_pos_ptr epos = (enum_pos_ptr) pos_id;
	if (!epos) return;
	if (epos->m_hDB != INVALID_HANDLE_VALUE) CloseHandle(epos->m_hDB);
	if (epos->m_pPropVal) HeapFree(GetProcessHeap(),0,epos->m_pPropVal);
	javacall_free(epos);
}
