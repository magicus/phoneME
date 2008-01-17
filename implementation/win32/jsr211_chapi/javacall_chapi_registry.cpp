/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/**
 * @file
 * @brief Content Handler Database stubs.
 */


#include "javacall_chapi_registry.h"
#include "apr_sdbm.h" 
#include <assert.h>
#include <malloc.h>

#ifdef NULL
	#undef NULL
	#define NULL 0
#endif

// database file pathname
#define REGISTRY_DB_NAME "\chappi_handlers_db"

static apr_pool_t *memPool = NULL;
static apr_sdbm_t *db = NULL;

static void PrintAprError( apr_status_t code ){
	char buf[256];
	puts("Apache Portable Runtime Library Error: " );
	apr_strerror( code, buf, sizeof(buf) );
	puts( buf );
	puts("\n" );
}


javacall_result javacall_chapi_init_registry(void){
	apr_status_t ret;

	assert( memPool == NULL ); // do not initialize twice

	ret = apr_pool_initialize();
	if ( ret != APR_SUCCESS )
		PrintAprError( ret );
		return JAVACALL_FAIL;

	ret = apr_pool_create( &memPool, NULL ); 	
	if ( ret != APR_SUCCESS ){
		PrintAprError( ret );
		apr_pool_terminate();
		return JAVACALL_FAIL;
	}
	assert( memPool != NULL );

	// open or create DB with default file permissions
	ret = apr_sdbm_open( &db, REGISTRY_DB_NAME, APR_SHARELOCK | APR_CREATE | APR_WRITE, APR_FPROT_OS_DEFAULT, memPool );
	if ( ret != APR_SUCCESS ){
		PrintAprError( ret );
		apr_pool_destroy( memPool );
		apr_pool_terminate();
		return JAVACALL_FAIL;
	}
	assert( db != NULL );
}

void javacall_chapi_finalize_registry(void){
	apr_status_t ret;

	assert( db != NULL ); 
	ret = apr_sdbm_close( db );   
	if ( ret != APR_SUCCESS )
		PrintAprError( ret );

	assert( memPool != NULL ); // initialize first
	apr_pool_destroy( memPool );
	memPool = NULL;
	apr_pool_terminate();
}

class ContentHandlerInfo
{
public:
	class Enumerator{
	public:
		/// will return next string or NULL if no more strings left
		javacall_const_utf16_string getNext();
	private:
		Enumerator( javacall_utf16_string s );
		javacall_utf16_string mPos; //< current enumerator position
		friend class ContentHandlerInfo;
	};
	/// will copy serialized data to internal buffer
	ContentHandlerInfo( apr_sdbm_datum_t *serializedData );
	// fetch from db by id
	ContentHandlerInfo( javacall_const_utf16_string id );
	ContentHandlerInfo( 
		javacall_const_utf16_string content_handler_friendly_appname, 
		javacall_const_utf16_string suite_id, 
		javacall_const_utf16_string class_name, 
		javacall_chapi_handler_registration_type flag, 
		javacall_const_utf16_string *content_types, 
		int nTypes, 
		javacall_const_utf16_string *suffixes, 
		int nSuffixes, 
		javacall_const_utf16_string *actions, 
		int nActions, 
		javacall_const_utf16_string *locales, 
		int nLocales, 
		javacall_const_utf16_string *action_names, 
		int nActionNames, 
		javacall_const_utf16_string *access_allowed_ids, 
		int nAccesses
	);
	~ContentHandlerInfo();
	javacall_const_utf16_string getFriendlyAppName();
	javacall_const_utf16_string getClassName();
	javacall_const_utf16_string getSuiteID();
	javacall_chapi_handler_registration_type getRegistrationType();
	Enumerator getContentTypes();                  
	Enumerator getSuffixes();                  
	Enumerator getActions();                  
	Enumerator getActionNames();                  
	Enumerator getLocales();                  
	Enumerator getAccessAllowedIDs();
	/// returned object will be invalid if instance of this class is not exist anymore
	apr_sdbm_datum_t serialize();
private:
	javacall_chapi_handler_registration_type mRegistrationType;
	javacall_utf16_string mFriendlyAppName;
	javacall_utf16_string mClassName;
	javacall_utf16_string mSuiteID;
	// below is series of null terminated strings, ends with double nulls (like 3 strings A, B and C 'A\0B\0C\0\0')
	javacall_utf16_string mContentTypes;
	javacall_utf16_string mSuffixes;
	javacall_utf16_string mActions;
	javacall_utf16_string mActionNames;
	javacall_utf16_string mLocales;
	javacall_utf16_string mAccessAllowedIDs;

	javacall_utf16_string mBuf;
	int                   mBufSize;  //< in bytes

	// fills all members from mBuf
	void constructFromBuf();
	void constructFromDatum( apr_sdbm_datum_t * );

};

inline ContentHandlerInfo::Enumerator::Enumerator( javacall_utf16_string s ){
	mPos = s;
}

inline javacall_const_utf16_string ContentHandlerInfo::Enumerator::getNext(){
	if ( *mPos == 0 )
		return NULL;
	javacall_utf16_string ret = mPos;
	int len = wcslen( mPos );
	mPos += len + 1; // + 1 -> inlude terminating null
	return ret;
}

/// returns pointer to destination buffer dst immediately after null character of copied string
inline javacall_utf16_string Copy( javacall_utf16_string dst, javacall_const_utf16_string src ){
	int len = wcslen( src );
	wcscpy( dst, src );
	return &dst[len + 1];
}

/// returns size in symbols of sa, including nulls
static int GetDaStringArraySize( javacall_const_utf16_string * sa, int numOfElements ){
	int ret = 0;
	for ( int i = numOfElements; --i >= 0; ){
		ret += wcslen( sa[i] ) + 1;
	}
	return ret;
}

static javacall_utf16_string SerializeDaStringArray( 
	javacall_utf16_string serializeTo, 
	javacall_const_utf16_string * sa, 
	int numOfElements )
{
	javacall_utf16_string buf = serializeTo;
	for ( int i = numOfElements; --i >= 0; ){
		buf = Copy( buf, sa[i] );
	}
	*(buf++) = 0;
	return buf;
}


ContentHandlerInfo::ContentHandlerInfo( 
	javacall_const_utf16_string content_handler_friendly_appname, 
	javacall_const_utf16_string suite_id, 
	javacall_const_utf16_string class_name, 
	javacall_chapi_handler_registration_type flag, 
	javacall_const_utf16_string *content_types, 
	int nTypes, 
	javacall_const_utf16_string *suffixes, 
	int nSuffixes, 
	javacall_const_utf16_string *actions, 
	int nActions, 
	javacall_const_utf16_string *locales, 
	int nLocales, 
	javacall_const_utf16_string *action_names, 
	int nActionNames, 
	javacall_const_utf16_string *access_allowed_ids, 
	int nAccesses )
{
	assert( content_handler_friendly_appname != NULL );
	assert( suite_id						 != NULL );
	assert( class_name                       != NULL );

	int sizeOfBuf = 0;

	sizeOfBuf += wcslen(content_handler_friendly_appname) + 1;
	sizeOfBuf += wcslen(suite_id) + 1;
	sizeOfBuf += wcslen(class_name) + 1;

	sizeOfBuf += GetDaStringArraySize( content_types,      nTypes ) + 1; // +1 for terminating double nulls
	sizeOfBuf += GetDaStringArraySize( suffixes,           nSuffixes ) + 1; 
	sizeOfBuf += GetDaStringArraySize( actions,            nActions ) + 1; 
	sizeOfBuf += GetDaStringArraySize( locales,            nLocales ) + 1; 
	sizeOfBuf += GetDaStringArraySize( action_names,       nActionNames ) + 1; 
	sizeOfBuf += GetDaStringArraySize( access_allowed_ids, nAccesses ) + 1; 

	sizeOfBuf *= 2; // we need size in bytes

	// TODO:
	sizeOfBuf += sizeof( int );
	
	mBuf = (javacall_utf16_string) malloc( sizeOfBuf );
	if ( mBuf == NULL ){
		throw JAVACALL_OUT_OF_MEMORY;
	}
	mBufSize = sizeOfBuf;
	
	// if you want to change this, take care of deserialization constructor

	// int type first to prevent possible misalignment errors on some platforms
	int *tPtr = (int*) mBuf; 
	*(tPtr++) = flag;


	javacall_utf16_string ptr = (javacall_utf16_string) tPtr;

	ptr = Copy( ptr, content_handler_friendly_appname );
	ptr = Copy( ptr, suite_id );
	ptr = Copy( ptr, class_name );

	ptr = SerializeDaStringArray( ptr, content_types, nTypes );
	ptr = SerializeDaStringArray( ptr, suffixes,      nSuffixes );
	ptr = SerializeDaStringArray( ptr, actions,       nActions );
	ptr = SerializeDaStringArray( ptr, locales,       nLocales );
	ptr = SerializeDaStringArray( ptr, action_names,  nActionNames );
	ptr = SerializeDaStringArray( ptr, access_allowed_ids, nAccesses );

	constructFromBuf();
}

inline ContentHandlerInfo::ContentHandlerInfo( apr_sdbm_datum_t *serializedData ){
	constructFromDatum( serializedData );
}

ContentHandlerInfo::ContentHandlerInfo( javacall_const_utf16_string id ){
	mBuf = NULL;
	apr_status_t stat;
	apr_sdbm_datum_t key, value;
	key.dptr = (char*) id;
	key.dsize = wcslen( id ) * sizeof(*id); 

	stat = apr_sdbm_fetch( db, &value, key);
	if ( stat != APR_SUCCESS ){
		PrintAprError( stat );
		assert( 0 );
		throw JAVACALL_FAIL;
	}	
	constructFromDatum( &value );
}


ContentHandlerInfo::~ContentHandlerInfo(){
	if ( mBuf != NULL )
		free( mBuf );
}

void ContentHandlerInfo::constructFromDatum( apr_sdbm_datum_t *serializedData ){
	assert( serializedData != NULL );
	mBuf = (javacall_utf16_string) malloc( serializedData->dsize );
	if ( mBuf == NULL ){
		throw JAVACALL_OUT_OF_MEMORY;
	}
	memcpy( mBuf, serializedData->dptr, serializedData->dsize ); 
	constructFromBuf();
}

void ContentHandlerInfo::constructFromBuf(){
	int *tPtr = (int*) mBuf; 
	mRegistrationType = (javacall_chapi_handler_registration_type) *(tPtr++);

	javacall_utf16_string ptr = (javacall_utf16_string) tPtr;
	
	mFriendlyAppName = ptr;
	ptr += wcslen( ptr ) + 1;
	mSuiteID = ptr;
	ptr += wcslen( ptr ) + 1;
	mClassName = ptr;
	ptr += wcslen( ptr ) + 1;

	for ( mContentTypes     = ptr; *ptr != 0; ptr += wcslen( ptr ) + 1 );
	ptr++;
	for ( mSuffixes         = ptr; *ptr != 0; ptr += wcslen( ptr ) + 1 );
	ptr++;
	for ( mActions          = ptr; *ptr != 0; ptr += wcslen( ptr ) + 1 );
	ptr++;
	for ( mLocales          = ptr; *ptr != 0; ptr += wcslen( ptr ) + 1 );
	ptr++;
	for ( mActionNames      = ptr; *ptr != 0; ptr += wcslen( ptr ) + 1 );
	ptr++;
	for ( mAccessAllowedIDs = ptr; *ptr != 0; ptr += wcslen( ptr ) + 1 );
}

// returned object will be invalid if instance of this class is not exist anymore
apr_sdbm_datum_t ContentHandlerInfo::serialize(){
	apr_sdbm_datum_t d;
	d.dptr  = (char*) mBuf;
	d.dsize = mBufSize;
	return d;
}

inline javacall_const_utf16_string ContentHandlerInfo::getFriendlyAppName(){
	return mFriendlyAppName;
}

inline javacall_const_utf16_string ContentHandlerInfo::getClassName(){
	return mClassName;
}

inline javacall_const_utf16_string ContentHandlerInfo::getSuiteID(){
	return mSuiteID;
}
inline javacall_chapi_handler_registration_type ContentHandlerInfo::getRegistrationType(){
	return mRegistrationType;
}

inline ContentHandlerInfo::Enumerator ContentHandlerInfo::getContentTypes(){
	Enumerator e( mContentTypes ); 
	return e;
}

inline ContentHandlerInfo::Enumerator ContentHandlerInfo::getSuffixes(){
	Enumerator e( mSuffixes ); 
	return e;
}

inline ContentHandlerInfo::Enumerator ContentHandlerInfo::getActions(){
	Enumerator e( mActions ); 
	return e;
}

inline ContentHandlerInfo::Enumerator ContentHandlerInfo::getActionNames(){
	Enumerator e( mActionNames ); 
	return e;
}

inline ContentHandlerInfo::Enumerator ContentHandlerInfo::getLocales(){
	Enumerator e( mLocales ); 
	return e;
}

inline ContentHandlerInfo::Enumerator ContentHandlerInfo::getAccessAllowedIDs(){
	Enumerator e( mAccessAllowedIDs ); 
	return e;
}


javacall_result javacall_chapi_register_handler(
	javacall_const_utf16_string content_handler_id, 
	javacall_const_utf16_string content_handler_friendly_appname, 
	javacall_const_utf16_string suite_id, 
	javacall_const_utf16_string class_name, 
	javacall_chapi_handler_registration_type flag, 
	javacall_const_utf16_string *content_types, 
	int nTypes, 
	javacall_const_utf16_string *suffixes, 
	int nSuffixes, 
	javacall_const_utf16_string *actions, 
	int nActions, 
	javacall_const_utf16_string *locales, 
	int nLocales, 
	javacall_const_utf16_string *action_names, 
	int nActionNames, 
	javacall_const_utf16_string *access_allowed_ids, 
	int nAccesses )
{
	assert( nActionNames == nActions * nLocales );
	try{
		ContentHandlerInfo chi(
			content_handler_friendly_appname, 
			suite_id, 
			class_name, 
			flag, 
			content_types, 
			nTypes, 
			suffixes, 
			nSuffixes, 
			actions, 
			nActions, 
			locales, 
			nLocales, 
			action_names, 
			nActionNames, 
			access_allowed_ids, 
			nAccesses 
		);

		apr_status_t stat;
		apr_sdbm_datum_t key, value;
		key.dptr = (char*) content_handler_id;
		key.dsize = wcslen( content_handler_id ) * sizeof( *content_handler_id );
		value = chi.serialize();

		stat = apr_sdbm_store ( db, key, value, APR_SDBM_REPLACE );
		if ( stat != APR_SUCCESS ){
			PrintAprError( stat );
			return JAVACALL_FAIL;
		}
	}
	catch ( javacall_result e ) {
		return e;
	}
	return JAVACALL_OK;
}

static javacall_result GetNextKey( apr_sdbm_datum_t **key )
{
	apr_status_t stat;
	if ( *key == 0 ){
		stat = apr_sdbm_lock( db, APR_FLOCK_SHARED ); 	
		if ( stat != APR_SUCCESS ){
			PrintAprError( stat );
			return JAVACALL_FAIL;
		}
		apr_sdbm_datum_t *newKey;
		newKey = (apr_sdbm_datum_t *) malloc( sizeof(apr_sdbm_datum_t) );
		if ( newKey == NULL ){
			return JAVACALL_OUT_OF_MEMORY;
		}
		stat = apr_sdbm_firstkey( db, newKey ); 	
		if ( stat != APR_SUCCESS ){
			PrintAprError( stat );
			free( newKey );
			return JAVACALL_FAIL;
		}
		*key = newKey;
	}
	else {
		stat = apr_sdbm_nextkey( db, *key ); 	
		if ( stat != APR_SUCCESS ){
			PrintAprError( stat );
			return JAVACALL_FAIL;
		}
	}
	if ( (*key)->dptr == NULL ){ //we've run out of elements in db
		return (javacall_result) JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	}
	return JAVACALL_OK;
}
static javacall_result CopyHandlerID( apr_sdbm_datum_t *id, javacall_utf16 *handler_id_out, int *length )
{
	assert( id->dsize % sizeof(*handler_id_out) == 0 );
	int buffLength = *length;
	int idLength   = 1 +  id->dsize / sizeof( *handler_id_out );
	*length = idLength;
	if ( idLength > buffLength ){
		return (javacall_result) JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL;
	}
	memcpy( handler_id_out, id->dptr, id->dsize );
	handler_id_out[ id->dsize/sizeof(*handler_id_out) ] = 0;
	return JAVACALL_OK;
}

javacall_result javacall_chapi_enum_handlers (int *pos_id, javacall_utf16 *handler_id_out, int *length)
{
	assert( sizeof( int ) == sizeof(void*) ); //true for most platforms
	apr_sdbm_datum_t *key = (apr_sdbm_datum_t*) *pos_id;

	javacall_result res;
	res = GetNextKey( &key );
	if ( res != JAVACALL_OK )
		return res;
	res = CopyHandlerID( key, handler_id_out, length);
	if ( res != JAVACALL_OK )
		return res;

	*pos_id = (int) key;
	return JAVACALL_OK;
}

class Searcher {
public:
	virtual bool found( 
		apr_sdbm_datum_t *key,
		apr_sdbm_datum_t *value,
		javacall_utf16_string out,
		int *length,
		javacall_result *retCode ) = 0;
};

javacall_result Iterate( 
	Searcher *sr,	 
	int *pos_id, 
	javacall_utf16 *out, 
	int *length)
{
	try{
		apr_sdbm_datum_t *key = (apr_sdbm_datum_t*) *pos_id;
		bool found = false;
		javacall_result res;
		while ( !found ){
			res = GetNextKey( &key );
			if ( res != JAVACALL_OK )
				return res;

			apr_sdbm_datum_t value;
 			apr_status_t stat = apr_sdbm_fetch( db, &value, *key);
			if ( stat != APR_SUCCESS ){
				PrintAprError( stat );
				return JAVACALL_FAIL;
			}
		
			found = sr->found( key, &value, out, length, &res );
		}
		if ( res != JAVACALL_OK )
			return res;
		if ( !found )
			return (javacall_result) JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			
		*pos_id = (int) key;
		return JAVACALL_OK;
	}
	catch ( javacall_result e ) {
		return e;
	}
}

class HandlerSearcher : public Searcher {
public:
	HandlerSearcher( javacall_const_utf16_string s ) : mStr( s ){};
	// will return handler id in "out"
	virtual bool found( 
		apr_sdbm_datum_t *key,
		apr_sdbm_datum_t *value,
		javacall_utf16_string out,
		int *length,
		javacall_result *retCode );
protected:
	virtual ContentHandlerInfo::Enumerator getEnumerator( ContentHandlerInfo & ) = 0;
private:
	javacall_const_utf16_string mStr;
};

bool HandlerSearcher::found( 
		apr_sdbm_datum_t *key,
		apr_sdbm_datum_t *value,
		javacall_utf16_string out,
		int *length,
		javacall_result *retCode )
{
	ContentHandlerInfo chi( value );
	ContentHandlerInfo::Enumerator e = getEnumerator( chi );
	for ( javacall_const_utf16_string s = e.getNext(); s != NULL; s = e.getNext() ){
		assert( mStr != NULL );
		assert( s    != NULL );
		if ( wcsicmp( s, mStr ) == 0 ){
			*retCode = CopyHandlerID( key, out, length);
			return true;
		}
	}
	return false;
}


class BySuffix : public HandlerSearcher{
public:
	BySuffix( javacall_const_utf16_string s ) : HandlerSearcher( s ) {};
protected:
	ContentHandlerInfo::Enumerator getEnumerator( ContentHandlerInfo & );
};

ContentHandlerInfo::Enumerator BySuffix::getEnumerator( ContentHandlerInfo &ch )
{
	return ch.getSuffixes();
}

javacall_result javacall_chapi_enum_handlers_by_suffix (
	javacall_const_utf16_string suffix, 
	int *pos_id, 
	javacall_utf16 *handler_id_out, 
	int *length)
{
	BySuffix s( suffix );
	return Iterate( &s, pos_id, handler_id_out, length );
}


class ByType : public HandlerSearcher{
public:
	ByType( javacall_const_utf16_string s ) : HandlerSearcher( s ){};
protected:
	ContentHandlerInfo::Enumerator getEnumerator( ContentHandlerInfo & );
};

ContentHandlerInfo::Enumerator ByType::getEnumerator( ContentHandlerInfo &ch )
{
	return ch.getContentTypes();
}

javacall_result javacall_chapi_enum_handlers_by_type(
	javacall_const_utf16_string content_type,
	int *pos_id,
	javacall_utf16 *handler_id_out,
	int *length)
{
	ByType s( content_type );
	return Iterate( &s, pos_id, handler_id_out, length );
}


class ByAction : public HandlerSearcher{
public:
	ByAction( javacall_const_utf16_string s ) : HandlerSearcher( s ) {};
protected:
	ContentHandlerInfo::Enumerator getEnumerator( ContentHandlerInfo & );
};

ContentHandlerInfo::Enumerator ByAction::getEnumerator( ContentHandlerInfo &ch )
{
	return ch.getActionNames();
}

javacall_result javacall_chapi_enum_handlers_by_action (
	javacall_const_utf16_string action,
	int *pos_id,
	javacall_utf16 *handler_id_out,
	int *length)
{
	ByAction s( action );
	return Iterate( &s, pos_id, handler_id_out, length );
}


class HandlerBySuiteID : public Searcher {
public:
	HandlerBySuiteID( javacall_const_utf16_string s ) : mStr( s ){};
	bool found( 
		apr_sdbm_datum_t *key,
		apr_sdbm_datum_t *value,
		javacall_utf16_string out,
		int *length,
		javacall_result *retCode );
private:
	javacall_const_utf16_string mStr;
};

bool HandlerBySuiteID::found( 
		apr_sdbm_datum_t *key,
		apr_sdbm_datum_t *value,
		javacall_utf16_string out,
		int *length,
		javacall_result *retCode )
{
	ContentHandlerInfo chi( value );
	javacall_const_utf16_string suiteID = chi.getSuiteID();

	if ( wcsicmp( suiteID, mStr ) == 0 ){
		*retCode = CopyHandlerID( key, out, length);
		return true;
	}
	return false;
}

javacall_result javacall_chapi_enum_handlers_by_suite_id(
	javacall_const_utf16_string suite_id, 
	int *pos_id, 
	javacall_utf16 *handler_id_out, 
	int *length)
{
	HandlerBySuiteID s( suite_id );
	return Iterate( &s, pos_id, handler_id_out, length );
}


class HandlerByPrefix : public Searcher {
public:
	/// if findHandlerThatIsPrefixOf == false, than find handler, that includes s as prefix
	HandlerByPrefix( javacall_const_utf16_string s, bool findHandlerThatIsPrefixOf );
	bool found( 
		apr_sdbm_datum_t *key,
		apr_sdbm_datum_t *value,
		javacall_utf16_string out,
		int *length,
		javacall_result *retCode );
private:
	javacall_utf16_string mStr;
	int                   mStrSize; //< in bytes, not including NULL
	bool                  mStrIsPrefix;
};

inline HandlerByPrefix::HandlerByPrefix( javacall_const_utf16_string s, bool findHandlerWichIsPrefixOf )
{
	assert( s != NULL );
	mStr = (javacall_utf16_string) s;
	mStrSize = wcslen( mStr ) * sizeof( *mStr );
	mStrIsPrefix = ! findHandlerWichIsPrefixOf;
}

bool HandlerByPrefix::found( 
		apr_sdbm_datum_t *key,
		apr_sdbm_datum_t *value,
		javacall_utf16_string out,
		int *length,
		javacall_result *retCode )
{
	assert( key->dptr != NULL );

	if ( mStrIsPrefix ) {
		if ( key->dsize < mStrSize )
			return false;
	}
	else {
		if ( key->dsize > mStrSize )
			return false;
	}

	int cmpSize = key->dsize > mStrSize ? mStrSize : key->dsize;
	if ( memcmp( key->dptr, mStr, cmpSize ) == 0 ){
		*retCode = CopyHandlerID( key, out, length);
		return true;
	}

	return false;
}

javacall_result javacall_chapi_enum_handlers_by_prefix(
	javacall_const_utf16_string id,
	int *pos_id,
	javacall_utf16 *handler_id_out,
	int *length)
{
	HandlerByPrefix s( id, false );
	return Iterate( &s, pos_id, handler_id_out, length );
}

javacall_result javacall_chapi_enum_handlers_prefixes_of(
	javacall_const_utf16_string id,
	int *pos_id,
	javacall_utf16 *handler_id_out, int *length)
{
	HandlerByPrefix s( id, true );
	return Iterate( &s, pos_id, handler_id_out, length );
}


static javacall_result CopyStringToOutParameter(
	javacall_utf16_string outStr,
	int *outLength,
	javacall_const_utf16_string str )
{
	int buffLength = *outLength;
	int strLength = wcslen( str ) + 1;
	*outLength = strLength;
	if ( strLength > buffLength )
		return (javacall_result) JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL;
	wcscpy( outStr, str );
	return JAVACALL_OK;
}


class EnumeratorHolder{
public:
	/** creates new instance through id or gets from pos_id 
		value, returned in pos_id, if any, should be deleted later with free(), *pos_id = 0 in case of error
	*/
	static javacall_result fillNextFromIDOrPos( 
		javacall_const_utf16_string id,
		int *pos_id,
		ContentHandlerInfo::Enumerator (*fn)( ContentHandlerInfo& ),
		javacall_utf16_string out, 
		int *length	);
private:
	EnumeratorHolder(){}
	javacall_utf16_string mCurrent;
};

javacall_result EnumeratorHolder::fillNextFromIDOrPos( 
		javacall_const_utf16_string id,
		int *pos_id,
		ContentHandlerInfo::Enumerator (*fn)( ContentHandlerInfo& ),
		javacall_utf16_string out, 
		int *length			)
{
	assert( id != NULL );
	assert( pos_id != NULL );
	assert( length != NULL );

	EnumeratorHolder *eh;
	bool haveToFreeIfError = false;

	// TODO: check if all exeptions caught
	try {
		if ( *pos_id != 0 ){
			eh = (EnumeratorHolder*) *pos_id;
		}
		else{
			apr_status_t stat;
			stat = apr_sdbm_lock( db, APR_FLOCK_SHARED ); //because javacall_chapi_enum_finish doing unlock and can break nested locks
			if ( stat != APR_SUCCESS ){
				PrintAprError( stat );
				return JAVACALL_FAIL;
			}

			apr_sdbm_datum_t key, value;
			key.dptr = (char*) id;
			key.dsize = wcslen( id ) * sizeof( *id ); 

			stat = apr_sdbm_fetch( db, &value, key);
			if ( stat != APR_SUCCESS ){
				PrintAprError( stat );
				throw JAVACALL_FAIL;
			}

			ContentHandlerInfo chi( &value );

			ContentHandlerInfo::Enumerator e = fn( chi );
			int memSize = 0;
			// calculate size
			for ( javacall_const_utf16_string s = e.getNext(); s != NULL; s = e.getNext() ){
				assert( s != NULL );
				memSize += wcslen( s ) + 1;
			}
			memSize++; //for terminating null for whole array
			memSize *= sizeof( javacall_utf16  );
			memSize += sizeof( EnumeratorHolder );

			eh = (EnumeratorHolder *) malloc( memSize );
			if ( eh == NULL ){
				throw JAVACALL_OUT_OF_MEMORY;
			}
			haveToFreeIfError = true;

			eh->mCurrent = (javacall_utf16_string) ( eh + 1);
			javacall_utf16_string dst = eh->mCurrent;
			e = fn( chi );
			for ( javacall_const_utf16_string ss = e.getNext(); ss != NULL; ss = e.getNext() ){
				wcscpy( dst, ss );
				dst += wcslen( ss ) + 1;
				assert( (int) dst < (int) eh + memSize );
			}
			*dst = 0;
		}
		{ // copy to out string
			if ( *eh->mCurrent == 0 ){
				*length = 0;
				throw JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
			}
			javacall_result rez = CopyStringToOutParameter( out, length, eh->mCurrent  );
			if ( rez != JAVACALL_OK ) 
				throw rez;
		}
	}
	catch ( javacall_result r ){
		apr_status_t stat = apr_sdbm_unlock( db );
		if ( stat != APR_SUCCESS ){
			PrintAprError( stat );
			assert( 0 );
		}
		if ( haveToFreeIfError ) 
			free( eh );
		return r;
	}

	eh->mCurrent += wcslen( eh->mCurrent ) + 1;
	*pos_id = (int) eh;
	return JAVACALL_OK;
}


static ContentHandlerInfo::Enumerator GetSuffixEnum( ContentHandlerInfo& chi )
{
	return chi.getSuffixes();
}

javacall_result javacall_chapi_enum_suffixes(
	javacall_const_utf16_string content_handler_id,
	int *pos_id,
	javacall_utf16 *suffix_out,
	int *length)
{
	return EnumeratorHolder::fillNextFromIDOrPos( 
		content_handler_id,
		pos_id,
		GetSuffixEnum,
		suffix_out,
		length);
}


static ContentHandlerInfo::Enumerator GetContentTypeEnum( ContentHandlerInfo& chi )
{
	return chi.getContentTypes();
}

javacall_result javacall_chapi_enum_types(
	javacall_const_utf16_string content_handler_id,
	int *pos_id,
	javacall_utf16 *type_out,
	int *length)
{
	return EnumeratorHolder::fillNextFromIDOrPos( 
		content_handler_id,
		pos_id,
		GetContentTypeEnum,
		type_out,
		length);
}


static ContentHandlerInfo::Enumerator GetActionsEnum( ContentHandlerInfo& chi )
{
	return chi.getActions();
}

javacall_result javacall_chapi_enum_actions(
	javacall_const_utf16_string content_handler_id,
	int *pos_id,
	javacall_utf16 *action_out,
	int *length)
{
	return EnumeratorHolder::fillNextFromIDOrPos( 
		content_handler_id,
		pos_id,
		GetActionsEnum,
		action_out,
		length);
}


static ContentHandlerInfo::Enumerator GetLocalesEnum( ContentHandlerInfo& chi )
{
	return chi.getLocales();
}

javacall_result javacall_chapi_enum_action_locales(
	javacall_const_utf16_string content_handler_id,
	int *pos_id,
	javacall_utf16 *locale_out,
	int *length)
{
	return EnumeratorHolder::fillNextFromIDOrPos( 
		content_handler_id,
		pos_id,
		GetLocalesEnum,
		locale_out,
		length);
}


static ContentHandlerInfo::Enumerator GetAccessAllowedIDEnum( ContentHandlerInfo& chi )
{
	return chi.getAccessAllowedIDs();
}

javacall_result javacall_chapi_enum_access_allowed_callers(
	javacall_const_utf16_string content_handler_id,
	int *pos_id, 
	javacall_utf16 *access_allowed_out, 
	int *length)
{
	return EnumeratorHolder::fillNextFromIDOrPos( 
		content_handler_id,
		pos_id,
		GetAccessAllowedIDEnum,
		access_allowed_out,
		length);
}

javacall_result javacall_chapi_get_local_action_name(
	javacall_const_utf16_string content_handler_id,
	javacall_const_utf16_string action,
	javacall_const_utf16_string locale,
	javacall_utf16 *local_action_out,
	int *length)
{
	try {
		ContentHandlerInfo chi( content_handler_id );

		ContentHandlerInfo::Enumerator locales = chi.getLocales(); 
		ContentHandlerInfo::Enumerator actions = chi.getActions();
		ContentHandlerInfo::Enumerator actionNames = chi.getActionNames();
		for ( 
			javacall_const_utf16_string currentLocale = locales.getNext();
			currentLocale != NULL; 
			currentLocale = locales.getNext() )
		{
			for ( 
				javacall_const_utf16_string currentAction = actions.getNext();
				currentAction != NULL; 
				currentAction = actions.getNext() )
			{
				javacall_const_utf16_string currentActionName = actionNames.getNext();
				if ( wcscmp( currentLocale, locale ) == 0 && wcscmp( currentAction, action ) == 0 ){
					return CopyStringToOutParameter( local_action_out, length, currentActionName );
				}
			}
		}

	}
	catch ( javacall_result r ){
		return r;
	}
	return JAVACALL_INVALID_ARGUMENT;
}

javacall_result javacall_chapi_get_content_handler_friendly_appname(
	javacall_const_utf16_string content_handler_id,
	javacall_utf16 *handler_frienfly_appname_out,
	int *length)
{
	try {
		ContentHandlerInfo chi( content_handler_id );
		return CopyStringToOutParameter( handler_frienfly_appname_out, length, chi.getFriendlyAppName() );
	}
	catch ( javacall_result r ){
		return r;
	}
}

javacall_result javacall_chapi_get_handler_info(
	javacall_const_utf16_string content_handler_id,
	javacall_utf16 *suite_id_out,
	int *suite_id_len,
	javacall_utf16 *classname_out,
	int *classname_len,
	javacall_chapi_handler_registration_type *flag_out)
{
	try {
		ContentHandlerInfo chi( content_handler_id );
		*flag_out = chi.getRegistrationType();
		javacall_result ra = CopyStringToOutParameter( suite_id_out, suite_id_len, chi.getSuiteID() );
		javacall_result rb = CopyStringToOutParameter( classname_out, classname_len, chi.getClassName() );
		if ( ra != JAVACALL_OK )
			return ra;
		if ( rb != JAVACALL_OK )
			return rb;
	}
	catch ( javacall_result r ){
		return r;
	}
	return JAVACALL_OK;
}

javacall_bool javacall_chapi_is_access_allowed(
	javacall_const_utf16_string content_handler_id,
	javacall_const_utf16_string caller_id )
{
	try {
		ContentHandlerInfo chi( content_handler_id );
		ContentHandlerInfo::Enumerator CIDs = chi.getAccessAllowedIDs();
		for ( 
			javacall_const_utf16_string currentCID = CIDs.getNext();
			currentCID != NULL; 
			currentCID = CIDs.getNext() )
		{
			if ( wcscmp( currentCID, caller_id ) == 0 )
				return JAVACALL_TRUE;
		}
	}
	catch ( javacall_result r ){
		assert(0); // can't return error code. at least do somth in debug
	}

	return JAVACALL_FALSE;
}

javacall_bool javacall_chapi_is_action_supported(
	javacall_const_utf16_string content_handler_id,
	javacall_const_utf16_string action)
{
	try {
		ContentHandlerInfo chi( content_handler_id );
		ContentHandlerInfo::Enumerator actions = chi.getActions();
		for ( 
			javacall_const_utf16_string currentAction = actions.getNext();
			currentAction != NULL; 
			currentAction = actions.getNext() )
		{
			if ( wcscmp( currentAction, action ) == 0 )
				return JAVACALL_TRUE;
		}
	}
	catch ( javacall_result r ){
		assert(0); // can't return error code. at least do somth in debug
	}

	return JAVACALL_FALSE;
}

javacall_result javacall_chapi_unregister_handler(
	javacall_const_utf16_string content_handler_id)
{
	apr_status_t stat;
	apr_sdbm_datum_t key;
	key.dptr = (char*) content_handler_id;
	key.dsize = wcslen( content_handler_id ) * sizeof( *content_handler_id ); 

	stat = apr_sdbm_delete( db, key);
	if ( stat != APR_SUCCESS ){
		PrintAprError( stat );
		return JAVACALL_FAIL;
	}
	return JAVACALL_OK;
}


void javacall_chapi_enum_finish(int pos_id)
{
	if ( pos_id != 0 )
		free( (void*) pos_id );
}


