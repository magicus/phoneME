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
 * @file javacall_mailcap.c
 * @ingroup CHAPI
 * @brief javacall registry access implementation for mailcap files (rfc 1524)
 */


// #define DEBUG_OUTPUT 1

#include "javacall_chapi_registry.h"
#include "javacall_chapi_invoke.h"
#include "inc/javautil_str.h"
#include "inc/javautil_storage.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>


#define CHAPI_MALLOC(x) malloc(x)
#define CHAPI_CALLOC(x,s) calloc(x,s)
#define CHAPI_FREE(x) free(x)
#define CHAPI_REALLOC(x,s) realloc(x,s)


#define DATABASE_INCREASE_SIZE 128
#define MAX_BUFFER 512
#define MAX_LINE 1024
#define FILE_SEPARATOR '/'
#define XJAVA_ARRAY_DIVIDER ':'
#define LINE_DIVIDER "\n"
#define LINE_DIVIDER_LENGTH 1

#define TYPE_INFO_COMPOSETYPED 0x1
#define TYPE_INFO_NEEDSTERMINAL 0x2
#define TYPE_INFO_COPIOUSOUTPUT 0x4
#define TYPE_INFO_TEXTUALNEWLINES 0x8
#define TYPE_INFO_JAVA_HANDLER 0x10

#define TYPE_INFO_USE_REFFERENCE 0x100

#define TYPE_INFO_ACTION_MASK 0x2F000000
#define TYPE_INFO_ACTION_VIEW 0x01000000
#define TYPE_INFO_ACTION_EDIT 0x02000000
#define TYPE_INFO_ACTION_PRINT 0x04000000
#define TYPE_INFO_ACTION_COMPOSE 0x08000000
#define TYPE_INFO_ACTION_COMPOSETYPED 0x10000000

#define TYPE_INFO_ACTION_DEFAULT 0x20000000

#define is_view(a)  (a->flag & TYPE_INFO_ACTION_MASK == TYPE_INFO_ACTION_VIEW)
#define is_print(a)  (a->flag & TYPE_INFO_ACTION_MASK == TYPE_INFO_ACTION_PRINT)
#define is_edit(a)  (a->flag & TYPE_INFO_ACTION_MASK == TYPE_INFO_ACTION_EDIT)
#define is_compose(a)  (a->flag & TYPE_INFO_ACTION_MASK == TYPE_INFO_ACTION_COMPOSE)

typedef struct _content_type_info{
	short* type_name;
	short* description;
	short* nametemplate;
	short** suffixes;
	int flag;
	int actions_refcount;
} content_type_info;

typedef struct _handler_info{
	int flag;
	short* handler_id;
	short* handler_friendly_name;
	union {
		short* classname;
		short* appname;
	};
#ifdef SUITE_ID_STRING
	short* suite_id;
#else
	int suite_id;
#endif
	int jflag;
	short** access_list;
} handler_info;

typedef struct _action_info{
	content_type_info* content_type;
	handler_info* handler;
	int flag;
	union {
		short* actionname;
		const short* actionname_const;
	};
	short* params;
	short** locales;
	short** localnames;
} action_info;


#define INFO_INC 128

#define MAILCAP_INDEX 0
#define ACTIONMAP_INDEX 1
#define  ACCESSLIST_INDEX 2

unsigned long mailcap_lastread = 0;
unsigned long actionmap_lastread = 0;
unsigned long accesslist_lastread = 0;

char* mailcaps_path = 0;
char* mailcaps_fname = 0;
char* actionmap_fname = 0;	
char* accesslist_fname = 0;

const char* java_invoker = "${JVM_INVOKER}";
const short* java_type = L"application/x-java-content";

content_type_info** g_content_type_infos = 0;
int g_content_type_infos_used;
int g_content_type_infos_allocated;


handler_info** g_handler_infos = 0;
int g_handler_infos_used;
int g_handler_infos_allocated;

action_info** g_action_infos = 0;
int g_action_infos_used;
int g_action_infos_allocated;

const short* VIEW = L"view";
const short* EDIT = L"edit";
const short* PRINT = L"print";
const short* COMPOSE = L"compose";
const short* COMPOSETYPED = L"composetyped";
const short* OPEN = L"open";
const short* NEW = L"new";
const short* DEFAULT_ACTION = 0;

#define CHAPI_READ 1
#define CHAPI_WRITE 2
#define CHAPI_APPEND 3

#define chricmp(a,b) (((a>='A' && a<='Z') ? a+('a'-'A'): a) == ((b>='A' && b<='Z') ? b+('a'-'A'): b))

static int open_db(int db_index, javautil_storage* file, int flag);
static void close_db(javautil_storage file);
static void free_action_info(action_info* info);
static int read_access_list();
static int read_action_map();
static int read_caps();
static int is_modified(int index);
static void update_lastread(int index);
static void reset_lastread();
static javacall_bool is_access_allowed( const handler_info* info, javacall_const_utf16_string caller_id );

/******************* Type Infos **********************/

static content_type_info* new_content_type_info(short* type_name){
	content_type_info* info;
	if (!g_content_type_infos){
		g_content_type_infos = (content_type_info**)CHAPI_MALLOC(sizeof(content_type_info*)*INFO_INC);
		if (!g_content_type_infos) return 0;
		g_content_type_infos_allocated = INFO_INC;
		g_content_type_infos_used = 0;
	}
	if (g_content_type_infos_used>=g_content_type_infos_allocated){
		content_type_info** tmp = CHAPI_REALLOC(g_content_type_infos,sizeof(content_type_info*)*(g_content_type_infos_allocated+INFO_INC));
		if (!tmp) return 0;
		g_content_type_infos = tmp;
		g_content_type_infos_allocated += INFO_INC;
	}
	info = (content_type_info*)CHAPI_MALLOC(sizeof(content_type_info));
	if (!info) return 0;
	memset(info,0,sizeof(content_type_info));
	g_content_type_infos[g_content_type_infos_used++] = info;
	info->type_name = type_name;
	return info;
}

static void free_list(void** list){
	void **p;
	if (p=list){
		while (*p) CHAPI_FREE(*p++);
		CHAPI_FREE(list);
	}
}

static void free_content_type_info(content_type_info* info){
	if (info->type_name) CHAPI_FREE(info->type_name);
	if (info->description) CHAPI_FREE(info->description);
	if (info->nametemplate) CHAPI_FREE(info->nametemplate);
	free_list(info->suffixes);
	CHAPI_FREE(info);
}

static void pop_content_type_info(){
	if (g_content_type_infos_used) free_content_type_info(g_content_type_infos[--g_content_type_infos_used]);
}

static void release_content_type_info(content_type_info* type){
	if (!--type->actions_refcount){
		int index=g_content_type_infos_used;
		while (--index>=0 && g_content_type_infos[index]!=type);
		if (index>=0 && index<g_content_type_infos_used-1) 
			memmove(&g_content_type_infos[index],g_content_type_infos[index+1],g_content_type_infos_used-1-index);
		free_content_type_info(type);
		g_content_type_infos_used--;
	}
}


/******************* Handler Infos **********************/

static handler_info* new_handler_info(short* handler_id){
	handler_info* info;
	if (!g_handler_infos){
		g_handler_infos = (handler_info**)CHAPI_MALLOC(sizeof(handler_info*)*INFO_INC);
		if (!g_handler_infos) return 0;
		g_handler_infos_allocated = INFO_INC;
		g_handler_infos_used = 0;
	}
	if (g_handler_infos_used>=g_handler_infos_allocated){
		handler_info** tmp = CHAPI_REALLOC(g_handler_infos,sizeof(handler_info*)*(g_handler_infos_allocated+INFO_INC));
		if (!tmp) return 0;
		g_handler_infos = tmp;
		g_handler_infos_allocated += INFO_INC;
	}
	info = (handler_info*)CHAPI_MALLOC(sizeof(handler_info));
	if (!info) return 0;
	memset(info,0,sizeof(handler_info));
	info->handler_id = handler_id;
	g_handler_infos[g_handler_infos_used++] = info;
	return info;
}

static void free_handler_info(handler_info* info){
	if (info->handler_id) CHAPI_FREE(info->handler_id);
	if (info->classname) CHAPI_FREE(info->classname);
#ifdef SUITE_ID_STRING
	if (info->suite_id) CHAPI_FREE(info->suite_id);
#endif
	if (info->flag & TYPE_INFO_JAVA_HANDLER){
		if (info->handler_friendly_name) CHAPI_FREE(info->handler_friendly_name);
		//on not-java handlers handler_friendly_name refers to appname
	}
	if (info->access_list) free_list(info->access_list);
	CHAPI_FREE(info);
}

static void delete_handler_info(int index){
	if (g_handler_infos && g_handler_infos_used<index) {
		int i,j;
		handler_info* info = g_handler_infos[index];
		//delete actions
		if (g_action_infos){
			for (i = j =0 ; i<g_action_infos_used; ++i,++j){
				if (g_action_infos[i]->handler == info){
					if (g_action_infos[i]->content_type) release_content_type_info(g_action_infos[i]->content_type);
					free_action_info(g_action_infos[i]);
					--j;
				} else {
					if (i!=j) g_action_infos[j]=g_action_infos[i];
				}
			}
			g_action_infos_used = j;
		}
		//delete info
		free_handler_info(info);
		if (index<--g_handler_infos_used) 
			memmove(&g_handler_infos[index],g_handler_infos[index+1],g_handler_infos_used-1-index);
	}
}

static void pop_handler_info(){
	if (g_handler_infos_used) delete_handler_info(g_handler_infos_used-1);
}

/******************* Actions **********************/

static action_info* new_action_info(content_type_info* type, handler_info* handler){
	action_info* info;
	if (!g_action_infos){
		g_action_infos = (action_info**)CHAPI_MALLOC(sizeof(action_info*)*INFO_INC);
		if (!g_action_infos) return 0;
		g_action_infos_allocated = INFO_INC;
		g_action_infos_used = 0;
	}
	if (g_action_infos_used>=g_action_infos_allocated){
		action_info** tmp = CHAPI_REALLOC(g_action_infos,sizeof(action_info*)*(g_action_infos_allocated+INFO_INC));
		if (!tmp) return 0;
		g_action_infos = tmp;
		g_action_infos_allocated += INFO_INC;
	}
	info = (action_info*)CHAPI_MALLOC(sizeof(action_info));
	if (!info) return 0;
	memset(info,0,sizeof(action_info));
	info->content_type = type;
	info->handler = handler;
	g_action_infos[g_action_infos_used++] = info;
	type->actions_refcount++; // add ref
	return info;
}

static void free_action_info(action_info* info){
	if (info->actionname && !(info->flag & TYPE_INFO_ACTION_MASK)) {
		CHAPI_FREE(info->actionname);
	}
	if (info->params) {
		CHAPI_FREE(info->params);
	}
	if (!(info->flag & TYPE_INFO_USE_REFFERENCE)){
		free_list(info->locales);
		free_list(info->localnames);
	}
	CHAPI_FREE(info);
}

static void delete_action_info(int index){
	action_info* info = g_action_infos[index];
	if (index<--g_action_infos_used) 
		memmove(&g_action_infos[index],g_action_infos[index+1],g_action_infos_used-1-index);
	if (info->content_type) release_content_type_info(info->content_type);
	free_action_info(info);
}

static void pop_action_info(){
	if (g_action_infos_used) delete_action_info(g_action_infos_used-1);
}

/******************* registry utils **********************/

static void clean_registry(){
	if (g_content_type_infos){
		while (g_content_type_infos_used){
			free_content_type_info(g_content_type_infos[--g_content_type_infos_used]);
		}
		CHAPI_FREE(g_content_type_infos);
		g_content_type_infos = 0;
		g_content_type_infos_allocated = 0;
	}

	if (g_handler_infos){
		while (g_handler_infos_used){
			free_handler_info(g_handler_infos[--g_handler_infos_used]);
		}
		CHAPI_FREE(g_handler_infos);
		g_handler_infos = 0;
		g_handler_infos_allocated = 0;
	}

	if (g_action_infos){
		while (g_action_infos_used){
			free_action_info(g_action_infos[--g_action_infos_used]);
		}
		CHAPI_FREE(g_action_infos);
		g_action_infos = 0;
		g_action_infos_allocated = 0;
	}
}

static int update_registry(){
	int res=0;
	if (is_modified(MAILCAP_INDEX)){
		clean_registry();
		return read_caps();
	}
	if (g_handler_infos_used && is_modified(ACCESSLIST_INDEX)){
		res=read_access_list();
		if (res) return res;
	}
	if (g_action_infos_used && is_modified(ACTIONMAP_INDEX)){
		res=read_action_map();
	}

	return res;
}


/******************* common utils **********************/

static int copy_string(const short* str, /*OUT*/ short*  buffer, int* length){
	int len = (str?javautil_str_wcslen(str):0)+1;
	if (!length || (*length && !buffer)) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (*length < len){
		*length = len;
		return JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL;
	}
	if (len>1) {
		memcpy(buffer,str,len*sizeof(*str));
	} else {
		buffer[0]=0;
	}

	*length = len;
	return 0;
}

static short unhex(short code){
	if (code >= '0' &&  code <= '9') return code - '0';
	if (code >= 'A' &&  code <= 'F') return code - 'A' + 0xa;
	if (code >= 'a' &&  code <= 'f') return code - 'a' + 0xa;
	return -1;
}

// encode unicode string to acsii escape string and return length of encoded string
static int append_string(char* buffer, const short* str){
	const char tohex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	char* buf = buffer;
	--str;--buf;
	while (*++str){
		if ((*str & 0xFF) == *str && *str != '%' && *str != '\'' && *str != ';') {
			*++buf = (char)(*str & 0xFF);
		} else {
			//unicode to ascii
			*++buf = '%';
			*++buf = tohex[(*str >> 12) & 0xF];
			*++buf = tohex[(*str >> 8) & 0xF];
			*++buf = '%';
			*++buf = tohex[(*str >> 4) & 0xF];
			*++buf = tohex[(*str) & 0xF];
		}
	}

	*++buf = 0;
	return buf-buffer;
}


/******************* Mailcap parsing **********************/

/**
*   reads new not empty not commented line from file
*	line and max_size should not be zero
**/
static int get_line(short* line, int max_size, javautil_storage f){
	long pos;
	int i=-1,count=0;
	javautil_storage_getpos(f,&pos);
	while (javautil_storage_read(f,&i,1)==1){
		i&=0xFF;
		if ((short)i=='#'){
			while (javautil_storage_read(f,&i,1)==1 && (short)i!='\n');
		}
		if ((short)i!='\n' && (short)i!='\r') break;
	}
	while (i>0) {
		i &= 0xFF;
		if (i == '\n') break;
		if (count>=max_size-1) {
			//buffer too small return
			javautil_storage_setpos(f,pos,JUS_SEEK_SET);
			return JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL;
		}

		if (i == '%'){ //decode Unicode character
			while(1){
				long pos2;
				int code[4];
				char b[5];
				if (javautil_storage_read(f,&b,1)!=1) break;
				javautil_storage_getpos(f,&pos2);
				if (javautil_storage_read(f,b,5)==5 && b[2]=='%' &&	
					(code[0]=unhex(b[0])>=0) &&
					(code[1]=unhex(b[1])>=0) &&
					(code[2]=unhex(b[3])>=0) &&
					(code[3]=unhex(b[4])>=0) )
				{
					line[count++]= (unsigned short) ((code[0]<<12) + (code[1]<<8) + (code[2]<<4) + code[3]);
					if (javautil_storage_read(f,&i,1)!=1) break;
					continue;
				}
				javautil_storage_setpos(f,pos2,JUS_SEEK_SET);
				break;
			}
		}

		line[count++]=(short)i;
		if (javautil_storage_read(f,&i,1)!=1) break;
	}
	line[count]=0;
	return count;
}

/**
*   reads one line from file
*	line and max_size should not be zero
**/
static int read_line(char* line, int max_size, javautil_storage f){
	long pos=0;
	int i=0,count=0;
	javautil_storage_getpos(f,&pos);
	while (javautil_storage_read(f,&i,1)==1) {
		i &= 0xFF;
		if ((short)i=='\n') break;
		if (count>=max_size-1) {
			javautil_storage_setpos(f,pos,JUS_SEEK_SET);
			return JAVACALL_CHAPI_ERROR_BUFFER_TOO_SMALL;
		}
		line[count++] = i;	
	}
	line[count]=0;
	return count;
}

/**
*   javautil_storage_write one line to file
*	line should not be zero
*   line is perpended by LINE_DIVIDER string
**/
static int write_line(char* line, javautil_storage f){
	int len = strlen(line);
	if (javautil_storage_write(f,line,len) == len){
		if (javautil_storage_write(f, LINE_DIVIDER, LINE_DIVIDER_LENGTH) == LINE_DIVIDER_LENGTH) return 0;
	}
	return JAVACALL_CHAPI_ERROR_IO_FAILURE;
}


//find next token that ends on ; and can contain key and value separated by =
static int next_key(const short* line,const short** kstart,const short** kend, const short** valstart,const short** valend, const short** tokenend){
	int p=0,quot=0,dquot=0,endword=0;
	while (line[p] && (line[p]==' ' || line[p]=='\t')) ++p; //trim left
	if(!line[p]) return 0;
	endword=p;
	if (kstart) *kstart = &line[p];
	if (kend) *kend = &line[p];
	if (valstart) *valstart = 0;
	if (valend) *valend = 0;

	while (line[p]){
		if (line[p]=='=' && !quot && !dquot){ // value found
			if (kend) *kend = &line[endword];
			while (line[++p] && (line[p]==' ')); //trim left
			if (valstart) *valstart = &line[p];
			if (valend) *valend = &line[p];
			endword=p;
			while (line[p]){
				if (line[p]==';' && !quot && !dquot){
					p++;
					break;
				}
				if (line[p]=='\''&& !dquot){ quot = !quot;}
				if (line[p]=='\"'&& !quot){ dquot = !dquot;}
				if (line[p]!=' ' && line[p]!='\t') endword=p; //trim right
				++p;
			}
			if (valend) *valend = &line[endword];
			break;
		}
		if (line[p]==';' && !quot && !dquot){
			if (kend) *kend = &line[endword];
			if (valstart) *valstart = 0;
			if (valend) *valend = 0;
			p++;break;
		}
		if (line[p]=='\'' && !dquot){ quot = !quot;}
		if (line[p]=='\"'&& !quot){ dquot = !dquot;}
		if (line[p]!=' ' && line[p]!='\t') endword=p; //trim right
		++p;
	}
	if (tokenend) *tokenend = &line[p];
	return 1;
}

static int next_key_unquote(const short* line,const short** kstart,const short** kend, const short** valstart,const short** valend, const short** tokenend){
	int res = next_key(line,kstart,kend,valstart,valend, tokenend);
	if (kstart && kend && *kstart && *kend && *kend>*kstart && **kstart=='\'' && **kend=='\'') {
		*kstart = *kstart + 1; *kend = *kend - 1; //unquote
	}
	return res;
}

static int match(const short* p1, const short* pend, const short* p2){
	if (!p1 || !p2) return 0;
	while (*p1 && (p1<=pend) &&  *p2 && chricmp(*p1,*p2))
	{
		++p1;
		++p2;
	}
	return (!*p2);
}

/* search for key in line if found returns -1 and value location, if not returns 0 */
static int find_key(const short* line, const short* key, const short** valstart,const short** valend, const short** tokenend){
	const short* p = line, *ks, *ke;
	while ((next_key_unquote(p, &ks, &ke, valstart,valend,&p))){
		if (match(ks, ke, key)) {
			if (tokenend) *tokenend = p;
			return -1;
		}
	}
	return 0;
}


static int get_id(handler_info* info, /*OUT*/ short*  buffer, int* length){
		return copy_string(info->handler_id, buffer, length);
}


static int extract_handler(const short* cmds, const short* cmde, short** handler, short** params){
	int quoted = 0;
	int dquoted = 0;
	const short* c = cmds;
	const short* start,*end;
	int len;

	while (*c && c<=cmde && (*c==' ' || *c=='\t')) ++c; //trim left
	start = c;

	while (*c && c<=cmde){
		if (!(quoted || dquoted) && (*c==' ' || *c=='\t')) break;
		if (quoted && *c=='\'') quoted=!quoted;
		if (dquoted && *c=='\"') dquoted=!dquoted;
		c++;
	}

	if (handler){
		len = c-start;
		*handler = CHAPI_MALLOC((len+1) * sizeof(*start));
		if (!*handler) return JAVACALL_CHAPI_ERROR_NO_MEMORY;
		memcpy(*handler,start,len*sizeof(*start));
		(*handler)[len]=0;
	}

	while (*c && c<=cmde && (*c==' ' || *c=='\t')) ++c; //trim left
	start = end = c;

	while (*c && c<=cmde){
		if ((*c!=' ' && *c!='\t')) end=c; //trim right
		c++;
	}

	if (params){
		len = end-start+1;
		*params = CHAPI_MALLOC((len+1) * sizeof(*start));
		if (!*params) return JAVACALL_CHAPI_ERROR_NO_MEMORY;
		memcpy(*params,start,len*sizeof(*start));
		(*params)[len]=0;
	}

	return 0;
}


static short* substring(const short* str_begin, const short* str_end){
	int len;
	short* buf;
	len = str_end - str_begin + 1;

	buf = CHAPI_MALLOC((len+1) * sizeof(*str_begin));
	if (!buf) return 0;
	memcpy(buf,str_begin,len * sizeof(*str_begin));
	buf[len] = 0;

	return buf;
}

static short* substring_unquote(const short* str_begin, const short* str_end){
	if ( (*str_begin == '\'' && *str_end == '\'') || (*str_begin == '\"' && *str_end == '\"') ){
		str_begin++;
		str_end--;
	}
	return substring(str_begin,str_end);
}


static short** substringarray(const short* str_begin, const short* str_end, int* pcount){
	short** result = 0,**p;
	const short *s,*s2;
	int len=1;
	if (str_begin == str_end) return 0;
	s = str_begin-1;
	while (++s<str_end) if (*s==XJAVA_ARRAY_DIVIDER) ++len;

	if (pcount) *pcount=len;
	
	result = CHAPI_MALLOC((sizeof(short**)) * (len+1));
	if (!result) return 0;

	s = str_begin;
	s2 = str_begin-1;	
	p = result;

	while (++s2<=str_end) {
	if (*s2==XJAVA_ARRAY_DIVIDER) { 
			if (!(*p++ = substring_unquote(s,s2-1))){
				while (--p>=result) CHAPI_FREE(*p);
				CHAPI_FREE(result);
				return 0;
			}
			s=s2+1;
		}
	}

	if (s<str_end && !(*p++ = substring_unquote(s,str_end))){
		while (--p>=result) CHAPI_FREE(*p);
		CHAPI_FREE(result);
		return 0;
	}

	*p=0;
	return result;
}

static int get_integer(const short* str_begin, const short* str_end){
	int result = 0;
	int minus = 0;
	const short* buf = str_begin;
	if (!buf) return 0;
	if (*buf=='-') {minus=1;++buf;}
	while (*buf && buf <= str_end) {
		if(*buf>='0' && *buf<='9')  result = result * 10 + (*buf - '0');
		++buf;
	}
	return minus ? (-result) : result;
}


static handler_info* find_handler(const short* handler_name){
	int i=g_handler_infos_used;
	while (i){
		if (!javautil_str_wcscmp(g_handler_infos[--i]->handler_id, handler_name)) return g_handler_infos[i];
	}
	return 0;
}

static handler_info* find_handler_n(const short* handler_id,int len){
	int i=g_handler_infos_used;
	while (i){
		if (!javautil_str_wcsncmp(g_handler_infos[--i]->handler_id, handler_id, len)) return g_handler_infos[i];
	}
	return 0;
}


int read_access_list(){
javautil_storage f;
short* p, *ks, *ke, *vs, *ve;
short* line;
int length = MAX_BUFFER, res;
handler_info* info;

	line = CHAPI_MALLOC(length*sizeof(*line));
	if (!line) return JAVACALL_CHAPI_ERROR_NO_MEMORY;

	res = open_db(ACCESSLIST_INDEX,&f,CHAPI_READ);
	if (res) {
		CHAPI_FREE(line);
		//no file to read
		return JAVACALL_OK;
	}

	while ((res=get_line(line,length,f))){

		if (res < 0) { //buffer too small
			length*=2;
			CHAPI_FREE(line);
			line = CHAPI_MALLOC(length*sizeof(*line));
			if (!line) break;
			continue;
		}

		p=line;
		if (!next_key_unquote(p,&ks,&ke,&vs,&ve,&p)) continue;
		if (info = find_handler_n(ks,(int)(ke-ks)+1)){
			if (info->access_list) free_list(info->access_list);
			info->access_list = substringarray(vs,ve,0);
		}
	}

	if (line) CHAPI_FREE(line);
	close_db(f);
	update_lastread(ACCESSLIST_INDEX);

	return 0;
}

static int read_action_map(){
javautil_storage f;
short* p, *ks, *ke;
short* line;
int length = MAX_BUFFER, res, count_localnames, count_locales, ia;
handler_info* handler;
action_info* action,*action2;

	line = CHAPI_MALLOC(length*sizeof(*line));
	if (!line) return JAVACALL_CHAPI_ERROR_NO_MEMORY;

	res = open_db(ACTIONMAP_INDEX,&f,CHAPI_READ);
	if (res) {
		CHAPI_FREE(line);
		//no file to read
		return JAVACALL_OK;
	}
	while ((res=get_line(line,length,f))){
		if (res < 0) { //buffer too small
			length*=2;
			CHAPI_FREE(line);
			line = CHAPI_MALLOC(length*sizeof(*line));
			if (!line) break;
			continue;
		}

		p=line;

		// handler
		if (!next_key_unquote(p,&ks,&ke,0,0,&p)) continue;
		if (!(handler = find_handler_n(ks,(int)(ke-ks)+1))) continue;

		//action
		if (!next_key_unquote(p,&ks,&ke,0,0,&p)) continue;

		action = 0;
		for (ia = 0; ia<g_action_infos_used; ++ia){
			//find action with defined name for defined handler
			if (g_action_infos[ia]->handler == handler && (!javautil_str_wcsincmp(g_action_infos[ia]->actionname,ks,(int)(ke-ks)+1))) {
				action = g_action_infos[ia];
				if (action->locales && !(action->flag & TYPE_INFO_USE_REFFERENCE)) {
					free_list(action->locales);action->locales=0;
				    free_list(action->localnames);action->localnames=0;
				}
				break;
			}
		}
		if (!action) continue;

		//locales
		if (!next_key(p,&ks,&ke,0,0,&p)) continue;
		action->locales = substringarray(ks,ke,&count_locales);

		//names
		if (!next_key(p,&ks,&ke,0,0,&p)) continue;
		action->localnames = substringarray(ks,ke,&count_localnames);

		//check
		if (!action->locales || !action->localnames || count_locales!=count_localnames){

			if (action->locales) free_list(action->locales);
			action->locales = 0;

			if (action->localnames) free_list(action->localnames);
			action->localnames = 0;
			continue;
		}

		//assign names for all actions with same name of this handler
		for (++ia; ia<g_action_infos_used; ++ia){
			action2 = g_action_infos[ia];
			if (action2->handler == handler && (!javautil_str_wcsicmp(action2->actionname,action->actionname))){
				if (action2->locales && !(action2->flag & TYPE_INFO_USE_REFFERENCE)){ 
					free_list(action->locales);
					free_list(action->localnames);
				}
				action2->flag |= TYPE_INFO_USE_REFFERENCE;
				action2->locales=action->locales;
				action2->localnames=action->localnames;
			}
		}
	}

	if (line) CHAPI_FREE(line);
	close_db(f);
	update_lastread(ACTIONMAP_INDEX);
	return 0;
}


//javautil_storage_read mime-type handlers information from mailcaps file according to rfc1343
int read_caps(){
	short* p, *ks, *ke, *vs, *ve;
	short *params=0, *handler_name=0, *type_name=0, *java_key;
	content_type_info* type = 0;
	action_info* action = 0;
	handler_info* handler = 0;
	javautil_storage f;
	int is_java;
	short* line;
	int length = MAX_BUFFER;
	int res = JAVACALL_CHAPI_ERROR_NO_MEMORY;
	
#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::read_registry\n");
#endif

	line = CHAPI_MALLOC(length*sizeof(*line));
	if (!line) return JAVACALL_CHAPI_ERROR_NO_MEMORY;

	res = open_db(MAILCAP_INDEX,&f,CHAPI_READ);
	if (res) {
		CHAPI_FREE(line);
		//no file to read
		return JAVACALL_OK;
	}
	while ((res=get_line(line,length,f))){

		if (res < 0) { //buffer too small
			length*=2;
			CHAPI_FREE(line);
			line = CHAPI_MALLOC(length*sizeof(*line));
			if (!line) break;
			continue;
		}

		is_java=(find_key(line,L"x-java",0,0,&java_key));

		p = line;
		if (!next_key_unquote(p,&ks,&ke,0,0,&p)) continue;

		if (type_name) CHAPI_FREE(type_name);

		if (match(ks, ke,java_type)){
			type_name = CHAPI_MALLOC (sizeof(*type_name));
			*type_name = 0;
		} else {
			type_name = substring(ks,ke);
		}
		
		if (!type_name) break;


		type = new_content_type_info(type_name);
		if (!type) break;
		type_name = 0;

		if (!is_java){
			//javautil_storage_read native content handler
			short *name_pos;

			if (!next_key_unquote(p,&ks,&ke,&vs,&ve,&p) || vs || ve) continue;

			if (params) {CHAPI_FREE(params);params=0;}
			if (handler_name) {CHAPI_FREE(handler_name);handler_name=0;}
			if (extract_handler(ks,ke,&handler_name,&params)) continue;

			if (handler = find_handler(handler_name)){
				CHAPI_FREE (handler_name);
				handler_name = 0;
			} else {
				handler = new_handler_info(handler_name);
				handler->appname = handler_name;
				name_pos = wcsrchr(handler_name,FILE_SEPARATOR);
				if (!name_pos) name_pos = handler_name; else name_pos++;

				handler->handler_friendly_name = name_pos;
			}
			handler_name = 0;

			action = new_action_info(type, handler);
			action->params = params; params = 0;
			action->flag |= TYPE_INFO_ACTION_VIEW;
			action->actionname_const = VIEW;

			while ((next_key_unquote(p,&ks,&ke,&vs,&ve,&p))){
				if (match(ks, ke,VIEW) || match(ks, ke,EDIT) || match(ks, ke,PRINT) || match(ks, ke,COMPOSE) || match(ks, ke,COMPOSETYPED)) {

					if (extract_handler(vs,ve,&handler_name,&params)) continue;

					//new handler associated with action
					if (handler = find_handler(handler_name)){
						CHAPI_FREE (handler_name);
					} else {
						handler = new_handler_info(handler_name);
						handler->appname = handler_name;
						name_pos = wcsrchr(handler_name,FILE_SEPARATOR);
						if (!name_pos) name_pos = handler_name; else name_pos++;

						handler->handler_friendly_name = name_pos;
					}
					handler_name = 0;

					//new action
					action = new_action_info(type, handler);//increase type refs
					action->params = params;params = 0;
					if (match(ks, ke,VIEW)) {action->flag |= TYPE_INFO_ACTION_VIEW;action->actionname_const = VIEW; }
					if (match(ks, ke,EDIT)) {action->flag |= TYPE_INFO_ACTION_EDIT;action->actionname_const = EDIT; }
					if (match(ks, ke,PRINT)) {action->flag |= TYPE_INFO_ACTION_PRINT;action->actionname_const = PRINT; }
					if (match(ks, ke,COMPOSE)) {action->flag |= TYPE_INFO_ACTION_COMPOSE;action->actionname_const = COMPOSE; }
					if (match(ks, ke,COMPOSETYPED)) {action->flag |= TYPE_INFO_COMPOSETYPED;action->actionname_const = COMPOSETYPED; }
					continue;
				}
				if (match(ks, ke,L"description") && !type->description) {type->description = substring_unquote(vs,ve);continue;}
				if (match(ks, ke,L"nametemplate") && !type->nametemplate) {type->nametemplate = substring(vs,ve);continue;}
				if (match(ks, ke,L"textualnewlines")) {type->flag |= TYPE_INFO_TEXTUALNEWLINES; continue;}
				if (match(ks, ke,L"needsterminal")) {action->flag |= TYPE_INFO_NEEDSTERMINAL; continue;}
				if (match(ks, ke,L"copiousoutput")) {action->flag |= TYPE_INFO_COPIOUSOUTPUT; continue;}
			}
		} else {
			if (!find_key(line,L"x-handlerid",&vs,&ve,0)) continue;
			handler_name = substring_unquote(vs,ve);

			if (handler = find_handler(handler_name)){
				CHAPI_FREE (handler_name);
			} else {
				handler = new_handler_info(handler_name);
				handler->flag |= TYPE_INFO_JAVA_HANDLER;
			}
			handler_name = 0;

			p = line;
			action = 0;
						
			while ((next_key_unquote(p,&ks,&ke,&vs,&ve,&p))){
				if (match(ks, ke,L"x-action")) {
					action = new_action_info(type,handler);
					action->actionname = substring_unquote(vs,ve);
					action->flag |= TYPE_INFO_JAVA_HANDLER;
					continue;
				}
#ifdef SUITE_ID_STRING
				if (match(ks, ke,L"x-suiteid") && !handler->suite_id) {handler->suite_id = substring_unquote(vs,ve);continue;}
#else
				if (match(ks, ke,L"x-suiteid") && !handler->suite_id) {handler->suite_id = get_integer(vs,ve);continue;}
#endif
				if (match(ks, ke,L"x-flag")) {handler->jflag = get_integer(vs,ve);continue;}
				if (match(ks, ke,L"x-classname") && !handler->classname) {handler->classname = substring_unquote(vs,ve);continue;}
				if (match(ks, ke,L"x-suffixes") && !type->suffixes) {type->suffixes = substringarray(vs,ve,0);continue;}
			}

			if (!action) {
				action = new_action_info(type, handler);
				action->actionname_const = DEFAULT_ACTION;
				action->flag |= TYPE_INFO_JAVA_HANDLER | TYPE_INFO_ACTION_DEFAULT;
			}

		}
		if (type_name) {CHAPI_FREE(type_name);type_name=0;}
		if (handler_name) {CHAPI_FREE(handler_name);handler_name=0;}
		if (params) {CHAPI_FREE(params);params=0;}
		handler = 0;
		action = 0;
		type = 0;
}
if (handler) pop_handler_info();
if (line) CHAPI_FREE(line);
close_db(f);
update_lastread(MAILCAP_INDEX);

if (g_handler_infos_used){
	read_access_list();
}

if (g_handler_infos_used){
	read_action_map();
}

return 0;
}


static int suffix_fits(const short* suffix, content_type_info* info){
	short buffer[MAX_BUFFER],*b=buffer;
	if (!suffix || !info) return 0;
	if (info->suffixes){
		short** p = info->suffixes-1;
		while (*++p) if (!javautil_str_wcsicmp(*p,suffix)) return -1;
	}
	if (info->nametemplate){
		*b++='%';*b++='s';
		while (*suffix) *b++=(short)*suffix++;
		*b=0;
		return (!javautil_str_wcsicmp(buffer,info->nametemplate))?-1:0;
	}
	return 0;
}

static int type_fits(const short* type, content_type_info* info){
	short *t;
	if (!type || !info) return 0;
	t = info->type_name;
	while (*type && *t) {
		if (chricmp(*type,*t)) return 0;
		if (*t == '/' && (!*(t+1) || *(t+1)=='*')) return 1; //check implicit and explicit wildcards
		++type;
		++t;
	}
	return (!*type) && (!*t);
}

static int file_exists(const char* fname){
	javautil_storage file;
	if (JAVACALL_OK == javautil_storage_open(fname,JUS_O_RDONLY,&file)){
		javautil_storage_close(file);
		return 1;
	}
	return 0;
}

int open_db(int db_index, javautil_storage* file, int flag){
	const char* db_path = 0;
	*file = 0;
	if (db_index == MAILCAP_INDEX)
		db_path = mailcaps_fname;	
	else if (db_index == ACTIONMAP_INDEX)
		db_path = actionmap_fname;	
	else if (db_index == ACCESSLIST_INDEX)
		db_path = accesslist_fname;	
	else return JAVACALL_CHAPI_ERROR_BAD_PARAMS;


	if (flag == CHAPI_READ) {
		javautil_storage_open(db_path,JUS_O_RDONLY, file);
	} else 	if (flag == CHAPI_WRITE) {
		javautil_storage_open(db_path,JUS_O_RDWR | JUS_O_CREATE, file);
	} else if (flag == CHAPI_APPEND){
		javautil_storage_open(db_path, JUS_O_APPEND | JUS_O_CREATE, file);
	} else 
		return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (*file==JAVAUTIL_INVALID_STORAGE_HANDLE) {
		return JAVACALL_CHAPI_ERROR_IO_FAILURE;
	}

	return 0;
}


void close_db(javautil_storage file){
	if (file) javautil_storage_close(file);
}

/**********************************************************************************************************************/
/**
/**	Functions implemented by platform
/**
/**********************************************************************************************************************/

/**
 * Check native database was modified since time pointed in lastread
 *
 */
int is_modified(int index){
	unsigned long mtime=-1L;

	switch (index) {
		case MAILCAP_INDEX: 
			javautil_storage_get_modified_time(mailcaps_fname,&mtime);
			return mtime>mailcap_lastread;
		

		case ACTIONMAP_INDEX:
			javautil_storage_get_modified_time(actionmap_fname,&mtime);
			return mtime>actionmap_lastread;
		

		case ACCESSLIST_INDEX: 
			javautil_storage_get_modified_time(accesslist_fname,&mtime);
			return mtime>accesslist_lastread;

		default:
			return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
	}
}

void update_lastread(int index){
	unsigned long mtime=0;

	switch (index) {
		case MAILCAP_INDEX: 
			javautil_storage_get_modified_time(mailcaps_fname,&mtime);
			mailcap_lastread = mtime;
			break;
		

		case ACTIONMAP_INDEX:
			javautil_storage_get_modified_time(actionmap_fname,&mtime);
			actionmap_lastread = mtime;
			break;
		

		case ACCESSLIST_INDEX: 
			javautil_storage_get_modified_time(accesslist_fname,&mtime);
			accesslist_lastread = mtime;
			break;
	}
}

void reset_lastread(){
	mailcap_lastread = 0;
	actionmap_lastread = 0;
	accesslist_lastread = 0;
}

/**********************************************************************************************************************/
/**
/**	PUBLIC API
/**
/**********************************************************************************************************************/


/* try to load
	query MAILCAPS environment variable, if empty try to load:
   1. `./.mailcap'
   3. `$HOME/.mailcap'
   4. `/etc/mailcap'
   5. `/usr/etc/mailcap'
   6. `/usr/local/etc/mailcap' 
*/

javacall_result javacall_chapi_init_registry(void){
	char buf[MAX_BUFFER];
	int fpos=0;
	int res;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_init_registry()\n");
#endif
	
	while (1){
		if (getenv("MAILCAPS")) {
			fpos = sprintf(buf,"%s",getenv("MAILCAPS"));
			break;
		}

		fpos = sprintf(buf,".");
		sprintf(&buf[fpos],"/.mailcap");
		if (file_exists(buf)) break;

		if (getenv("HOME")){
			fpos = sprintf(buf,"%s",getenv("HOME"));
			sprintf(&buf[fpos],"/.mailcap");
			if (file_exists(buf)) break;
		}

		fpos = sprintf(buf,"/mailcap");
		sprintf(&buf[fpos],"/.mailcap");
		if (file_exists(buf)) break;

		fpos = sprintf(buf,"/etc/mailcap");
		sprintf(&buf[fpos],"/.mailcap");
		if (file_exists(buf)) break;

		fpos = sprintf(buf,"/usr/etc/mailcap");
		sprintf(&buf[fpos],"/.mailcap");
		if (file_exists(buf)) break;
	
		fpos = sprintf(buf,"/usr/local/etc/mailcap");
		sprintf(&buf[fpos],"/.mailcap");
		if (file_exists(buf)) break;

		//default
		fpos = sprintf(buf,".");
		break;
	}

	buf[fpos] = '\0';
	mailcaps_path = strdup(buf);

	sprintf(&buf[fpos],"/.mailcap");
	mailcaps_fname = strdup(buf);

	sprintf(&buf[fpos],"/.actionmap");
	actionmap_fname = strdup(buf);

	sprintf(&buf[fpos],"/.accesslist");
	accesslist_fname = strdup(buf);

	res = read_caps();

	return res;
}


void javacall_chapi_finalize_registry(void){
#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_finalize_registry()\n");
#endif

	clean_registry();
	reset_lastread();

	if (mailcaps_path) CHAPI_FREE(mailcaps_path);
	mailcaps_path = 0;

	if (mailcaps_fname) CHAPI_FREE(mailcaps_fname);
	mailcaps_fname = 0;

	if (actionmap_fname) CHAPI_FREE(actionmap_fname);
	actionmap_fname = 0;

	if (accesslist_fname) CHAPI_FREE(accesslist_fname);
	accesslist_fname = 0;

}


//register handler
javacall_result javacall_chapi_register_handler(
        javacall_const_utf16_string content_handler_id,
        javacall_const_utf16_string content_handler_friendly_appname,
#ifdef SUITE_ID_STRING
        javacall_const_utf16_string suite_id,
#else
       int suite_id,
#endif
        javacall_const_utf16_string class_name,
        javacall_chapi_handler_registration_type flag,
        javacall_const_utf16_string* content_types,     int nTypes,
        javacall_const_utf16_string* suffixes,  int nSuffixes,
        javacall_const_utf16_string* actions,   int nActions,  
        javacall_const_utf16_string* locales,   int nLocales,
        javacall_const_utf16_string* action_names, int nActionNames,
        javacall_const_utf16_string* access_allowed_ids,  int nAccesses){

	int result;
	javautil_storage file=0;
	int len;
	int itype,iact, il, iacc;
	char buf[MAX_LINE],*b;
	int idQuoted;

#ifdef DEBUG_OUTPUT
#ifdef SUITE_ID_STRING
	wprintf(L"JAVACALL::javacall_chapi_register_handler(%s,%s,%s)\n",content_handler_id,suite_id,class_name);
#else
	wprintf(L"JAVACALL::javacall_chapi_register_handler(%s,%d,%s)\n",content_handler_id,suite_id,class_name);
#endif
#endif

	result = open_db(MAILCAP_INDEX,&file,CHAPI_APPEND);
	if (result != 0) return result;

	//idQuoted = javautil_str_wcschr(content_handler_id,' ') || javautil_str_wcschr(content_handler_id,'\t') || javautil_str_wcschr(content_handler_id,'\'') || javautil_str_wcschr(content_handler_id,'=');
	idQuoted = 1;

	for (itype=0;itype<nTypes || (!nTypes && !itype);itype++){
		const short* type = nTypes?content_types[itype]:java_type;
		b=buf;

		// content type 
		b += append_string(b,type);
		*b++ = ';';
	
		// default action
#ifdef SUITE_ID_STRING
		b += sprintf(b,"%s -suite \'",java_invoker);
		b += append_string(b,suite_id);
		b += sprintf(b,"\' -class \'");
#else
		b += sprintf(b,"%s -suite \'%d\' -class \'",java_invoker,suite_id);
#endif
		b += append_string(b,class_name);
		b += sprintf(b,"\' \'%%s\';");

		b += sprintf(b,"test=test -n \"${JVM_INVOKER}\";");

		if (content_handler_friendly_appname){
			b += sprintf(b,"description=");
			b += append_string(b,content_handler_friendly_appname);
			b += sprintf(b," Document;");
		}
		if (nSuffixes){
			b += sprintf(b,"nametemplate=%%s");
			b += append_string(b,nSuffixes>itype ? suffixes[itype] : suffixes[0]);
			*b++ = ';';
		}

		for (iact=0;iact<nActions;iact++){
			const short* action = actions[iact];
			const short* taction = 0;

			if (!javautil_str_wcsicmp(action,EDIT)) {
				taction=EDIT;
			} else if (!javautil_str_wcsicmp(action,COMPOSE) || !javautil_str_wcsicmp(action,NEW)){
					taction=COMPOSE;
			} else if (!javautil_str_wcsicmp(action,COMPOSETYPED)){
					taction=COMPOSETYPED;
			} else if (!javautil_str_wcsicmp(action,PRINT)){
					taction=PRINT;
			}

			//regular mailcap action
			if (taction) { 
#ifdef SUITE_ID_STRING
		b += sprintf(b,"%s=%s -suite \'",taction,java_invoker);
		b += append_string(b,suite_id);
		b += sprintf(b,"\' -class \'");
#else
		b += sprintf(b,"%s=%s -suite \'%d\' -class \'",taction,java_invoker,suite_id);
#endif
				b += append_string(b,class_name);
				b += sprintf(b,"\' -action '");
				b += append_string(b, action);
				b += sprintf(b,"\' \'%%s\';");
			} 
			//java action
				b += sprintf(b,"x-action='");
				b += append_string(b, action);
				b += sprintf(b,"\';");
		}
		
		b += sprintf(b,"x-java;x-handlerid=");
		if (idQuoted) *b++ = '\'';
		b += append_string(b, content_handler_id);
		if (idQuoted) *b++ = '\'';
		*b++ = ';';
		
#ifdef SUITE_ID_STRING
		b += sprintf(b,"x-suiteid=\'");
		b += append_string(b,suite_id);
		b += sprintf(b,"\';");
#else
		b += sprintf(b,"x-suiteid=%d;",suite_id);
#endif

		b += sprintf(b,"x-classname='");
		b += append_string(b, class_name);
		b += sprintf(b,"\';");

		if (nSuffixes){
			int k;
			b += sprintf(b,"x-suffixes=");
			for (k=0;k<nSuffixes;++k) {
				b += append_string(b, suffixes[k]);
				if (k<nSuffixes-1) *b++ = XJAVA_ARRAY_DIVIDER;
			}
			*b++ = ';';
		}
		
		*b++ = '\n';

		len = b - buf;
		if (javautil_storage_write(file,buf,len)!=len) {
			result = JAVACALL_CHAPI_ERROR_IO_FAILURE;
			break;
		}
		result=0;
	}
	close_db(file);
	reset_lastread();

	if (result)  return result;


//update action local names
	if (nLocales){
		result = open_db(ACTIONMAP_INDEX,&file,CHAPI_APPEND);
		if (result != 0) return result;

		javautil_storage_setpos(file,0,JUS_SEEK_END);

		for (iact=0;iact<nActions;iact++){
			const short* action = actions[iact];
			
			b=buf;
			//handler_id
			if (idQuoted) *b++ = '\'';
			b += append_string(b,content_handler_id);
			if (idQuoted) *b++ = '\'';
			*b++ = ';';

			//action
			*b++ = '\'';
			b += append_string(b,action);
			*b++ = '\'';
			*b++ = ';';

			//locales
			for (il=0;il<nLocales;++il){
				b += append_string(b,locales[il]);
				if (il<nLocales-1) *b++ = XJAVA_ARRAY_DIVIDER;
			}
			*b++ = ';';

			//actionnames
			for (il=0;il<nLocales;++il){
				*b++ = '\'';
				b += append_string(b,action_names[iact + nActions * il]);
				*b++ = '\'';
				if (il<nLocales-1) *b++ = XJAVA_ARRAY_DIVIDER;
			}

			*b++ = ';';
			*b++ = '\n';

			len=b-buf;
			if (javautil_storage_write(file,buf,len)!=len) {
				result = JAVACALL_CHAPI_ERROR_IO_FAILURE;
				break;
			}
			result = 0;
		}

		close_db(file);
		reset_lastread();

		if (result)  return result;
	}

//update access_list	
	if (nAccesses){
		result = open_db(ACCESSLIST_INDEX,&file,CHAPI_APPEND);
		if (result) return result;


		javautil_storage_setpos(file,0,SEEK_END);

		while (1){
			b=buf;
			//handler_id
			if (idQuoted) *b++ = '\'';
			b += append_string(b,content_handler_id);
			if (idQuoted) *b++ = '\'';
			*b++ = '=';

		for (iacc=0;iacc<nAccesses;++iacc){
				*b++ = '\'';
				b += append_string(b,access_allowed_ids[iacc]);
				*b++ = '\'';
				if (iacc<nAccesses-1) *b++ = XJAVA_ARRAY_DIVIDER;
			}

			*b++ = ';';
			*b++ = '\n';

			len=b-buf;
			if (javautil_storage_write(file,buf,len)!=len) {
				result = JAVACALL_CHAPI_ERROR_IO_FAILURE;
				break;
			}
			result = 0;
			break;
		}
		close_db(file);
		reset_lastread();
	}

	return result;
}


javacall_result javacall_chapi_enum_handlers(int* pos_id, /*OUT*/ javacall_utf16*  buffer, int* length){
	int result;
#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_handlers(%d)\n",*pos_id);
#endif

	if (!pos_id) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
	if (!g_handler_infos || g_handler_infos_used<=*pos_id) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	result=get_id(g_handler_infos[*pos_id],buffer,length);
	if (!result){
		*pos_id = *pos_id+1;
	}
	return result;
}

javacall_result javacall_chapi_enum_handlers_by_suffix(javacall_const_utf16_string suffix, int* pos_id, /*OUT*/ javacall_utf16*  buffer, int* length){
	int result;
	int index1=((*(unsigned int*)pos_id) >> 16) & 0xFFFFL, index2 = *pos_id & 0xFFFFL;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_handlers_by_suffix(%s,%d)\n",suffix,*pos_id);
#endif

	if (!*pos_id){
		result = update_registry();
		if (result) return result;
	}

	if (!pos_id) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;
	if (!g_handler_infos || !g_content_type_infos) return result;
	while (index1 < g_content_type_infos_used){
		if (suffix_fits(suffix,g_content_type_infos[index1])){
			while (index2 < g_action_infos_used){
				if (g_action_infos[index2]->content_type == g_content_type_infos[index1]) {
					result=get_id(g_action_infos[index2]->handler,buffer,length);
					if (!result){
						*pos_id = (unsigned int)((index1+1) << 16) + (index2+1);
					}
					return result;
				}
				index2++;
			}
		}
		index1++;
	}

	return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
}

javacall_result javacall_chapi_enum_handlers_by_type(javacall_const_utf16_string content_type, int* pos_id, /*OUT*/ javacall_utf16*  buffer, int* length){
	int result;
	int index=*pos_id;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_handlers_by_type(%s,%d)\n",content_type,*pos_id);
#endif
	if (!pos_id) return JAVACALL_CHAPI_ERROR_BAD_PARAMS;

	if (!*pos_id){
		result = update_registry();
		if (result) return result;
	}

	if (!g_handler_infos || !g_content_type_infos || !g_action_infos) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	while (index < g_action_infos_used){
		if (!javautil_str_wcsicmp(content_type,g_action_infos[index]->content_type->type_name)){
				result=get_id(g_action_infos[index]->handler,buffer,length);
				if (!result){
					*pos_id = index+1;
				}
				return result;
		}
		++index;
	}

	return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
}

javacall_result javacall_chapi_enum_handlers_by_action(javacall_const_utf16_string action, int* pos_id, /*OUT*/ javacall_utf16*  buffer, int* length){
	int result,index=*pos_id;
	int found = 0;
	int searched_action=0;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_handlers_by_action(%s,%d)\n",action,*pos_id);
#endif

	if (!index){
		result = update_registry();
		if (result) return result;
	}

	result = JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;

	if (!g_handler_infos) return result;
	
	if (!action || !*action) searched_action = TYPE_INFO_ACTION_DEFAULT;
	else
	if (!javautil_str_wcsicmp(action,VIEW) || !javautil_str_wcsicmp(action, OPEN)) searched_action = TYPE_INFO_ACTION_VIEW;
	else
	if (!javautil_str_wcsicmp(action,EDIT)) searched_action = TYPE_INFO_ACTION_EDIT;
	else
	if (!javautil_str_wcsicmp(action,PRINT)) searched_action = TYPE_INFO_ACTION_PRINT;
	else
	if (!javautil_str_wcsicmp(action,COMPOSE) || !javautil_str_wcsicmp(action, NEW)) searched_action = TYPE_INFO_ACTION_COMPOSE;

	while (index < g_action_infos_used){
		action_info* ai = g_action_infos[index];
		if (ai->flag & TYPE_INFO_ACTION_MASK){
			found = (ai->flag & searched_action);
		} else {
			found = !javautil_str_wcsicmp(action,ai->actionname);
			
		}
		if (found){
			result=get_id(g_action_infos[index]->handler,buffer,length);
			if (!result){
				*pos_id = index+1;
			}
			return result;
		}
		index++;
	}

	return result;
}

javacall_result javacall_chapi_enum_handlers_by_suite_id(
#ifdef SUITE_ID_STRING
        javacall_const_utf16_string suite_id,
#else
        int suite_id,
#endif
        int* pos_id, 
        /*OUT*/ javacall_utf16*  buffer,
        int* length){
	int result;
	int index=*pos_id;

#ifdef DEBUG_OUTPUT
#ifdef SUITE_ID_STRING
	wprintf(L"JAVACALL::javacall_chapi_enum_handlers_by_suite_id(%s,%d)\n",suite_id,*pos_id);
#else
	wprintf(L"JAVACALL::javacall_chapi_enum_handlers_by_suite_id(%d,%d)\n",suite_id,*pos_id);
#endif
#endif

	if (!index){
		result = update_registry();
		if (result) return result;
	}

	while (index < g_handler_infos_used){
#ifdef SUITE_ID_STRING
		if ((g_handler_infos[index]->flag & TYPE_INFO_JAVA_HANDLER) && !javautil_str_wcscmp(g_handler_infos[index]->suite_id,suite_id)){
#else
		if ((g_handler_infos[index]->flag & TYPE_INFO_JAVA_HANDLER) && g_handler_infos[index]->suite_id == suite_id){
#endif
			result=get_id(g_handler_infos[index],buffer,length);
			if (!result){
				*pos_id = index+1;
			}
			return result;
		}
		index++;
	}

	return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
}

javacall_result javacall_chapi_enum_handlers_by_prefix(javacall_const_utf16_string id, int* pos_id, 
                                                    		/*OUT*/ javacall_utf16* buffer, int* length)
{
	int result;
	int index=*pos_id;
	int id_len = javautil_str_wcslen( id );

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_handlers_by_prefix(%s,%d)\n",id,*pos_id);
#endif

	if (!index){
		result = update_registry();
		if (result) return result;
	}

	while (index < g_handler_infos_used){
		if ( javautil_str_wcslen( g_handler_infos[index]->handler_id ) > id_len &&
				0 == javautil_str_wcsncmp(id, g_handler_infos[index]->handler_id, id_len) ){
			result=get_id(g_handler_infos[index],buffer,length);
			if (!result){
				*pos_id = index+1;
			}
			return result;
		}
		index++;
	}

	return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
}

javacall_result javacall_chapi_enum_handlers_prefixes_of(javacall_const_utf16_string id, int* pos_id, 
                                                    		/*OUT*/ javacall_utf16* buffer, int* length)
{
	int result;
	int index=*pos_id;
	int id_len = javautil_str_wcslen( id );

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_handlers_prefixes_of(%s,%d)\n",id,*pos_id);
#endif

	if (!index){
		result = update_registry();
		if (result) return result;
	}

	while (index < g_handler_infos_used){
		int handler_len = javautil_str_wcslen( g_handler_infos[index]->handler_id );
		if ( handler_len <= id_len &&
				0 == javautil_str_wcsncmp(id, g_handler_infos[index]->handler_id, handler_len) ){
			result=get_id(g_handler_infos[index],buffer,length);
			if (!result){
				*pos_id = index+1;
			}
			return result;
		}
		index++;
	}

	return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
}

//NOTE: not unique
javacall_result javacall_chapi_enum_suffixes(javacall_const_utf16_string content_handler_id, int* pos_id, /*OUT*/ javacall_utf16*  buffer, int* length){
	handler_info* handler;
	int result;
	int index = *pos_id & 0xFFFFF;
	int index2 = ((unsigned int)*pos_id) >> 20;
	content_type_info* type = 0;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_suffixes(%s,%d)\n",content_handler_id,*pos_id);
#endif

	if (index == 0){
		result = update_registry();
		if (result) return result;
	}

	if (content_handler_id){
		handler = find_handler(content_handler_id);
		if (!handler) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
		while (index < g_action_infos_used){
			action_info* action = g_action_infos[index];
			if (action->handler == handler){
				type = action->content_type;
				if ((type->suffixes && type->suffixes[index2]) || (type->nametemplate && index2==0))
					break;
			}
			index2 = 0;
			index++;
		}
		if (index >= g_action_infos_used) {
			return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
		}
	} else {
		while (index < g_content_type_infos_used){
			type = g_content_type_infos[index];
			if ((type->suffixes && type->suffixes[index2]) || (type->nametemplate && index2==0))
				break;
			index2=0;
			index++;
		}
		return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	}

	if (type->suffixes){
		result = copy_string(type->suffixes[index2],buffer,length);
		if (!result){
			*pos_id = (int)(((unsigned int)(index2+1) << 20) + (unsigned int)index);
		}
	} else {
		const short* pos;
		pos = javautil_str_wcsrchr(type->nametemplate,'.');
		if (!pos) pos=0;
		result = copy_string(pos,buffer,length);
		if (!result){
			*pos_id = index+1;
		}
	}
	return result;
}

//NOTE: not unique
javacall_result javacall_chapi_enum_types(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16*  buffer, int* length){
	handler_info* handler;
	int index = *pos_id; 
	int res;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_types(%s,%d)\n",content_handler_id,*pos_id);
#endif

	if (index == 0){
		res = update_registry();
		if (res) return res;
	}

	if (content_handler_id) {
		handler = find_handler(content_handler_id);
		if (!handler) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
		while (index < g_action_infos_used){
			action_info* action = g_action_infos[index];
			if (action->handler == handler){
				content_type_info* type = action->content_type;
				if (!type->type_name[0]) {
					index++;
					continue;
				}
				res = copy_string(type->type_name,buffer,length);
				if (!res){
					*pos_id = index + 1;
				}
				return res;
			}
			index++;
		}
		return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	} else {
		while (1){
			content_type_info* type = g_content_type_infos[index];
			if (!type->type_name[0]) {
				index ++;
				continue;
			}
			res = copy_string(type->type_name,buffer,length);
			if (!res){
				*pos_id = index + 1;
			}
			break;
		}
	}
	return res;
}

// if content_handler_id is null, enum actions regardless of content_handler
// returned actions can be not unique
javacall_result javacall_chapi_enum_actions(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16*  buffer, int* length){
	int result;
	handler_info* handler=0;

	int index = *pos_id;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_actions(%s,%d)\n",content_handler_id,*pos_id);
#endif
	
	if (!index){
		result = update_registry();
		if (result) return result;
	}
	
	if (!g_action_infos) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;

	if (content_handler_id){
		handler = find_handler(content_handler_id);
		if (!handler) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
	}

	for (;index < g_action_infos_used;++index){
		if (handler && g_action_infos[index]->handler != handler) continue;
		if (!g_action_infos[index]->actionname) continue;
		result = copy_string(g_action_infos[index]->actionname,buffer,length);
		if (!result){
			*pos_id = index+1;
		}
		return result;
	}

	return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
}


// can be not more than 4.000.000 actions and not more than 256 locales 
javacall_result javacall_chapi_enum_action_locales(javacall_const_utf16_string content_handler_id, /*OUT*/ int* pos_id, javacall_utf16* buffer, int* length){
	int result;
	handler_info* handler=0;

	int index1 = *(unsigned int*)pos_id >> 8;
	int index2 = *pos_id & 0xFF;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_action_locales(%s,%d)\n",content_handler_id,*pos_id);
#endif

	if (!*pos_id){
		result = update_registry();
		if (result) return result;
	}
	
	if (!g_action_infos) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;

	if (!*pos_id && content_handler_id){
		handler = find_handler(content_handler_id);
		if (!handler) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
	}

	if (!*pos_id){
		for (;index1 < g_action_infos_used;++index1){
			if (handler && g_action_infos[index1]->handler != handler) continue;
			if (!g_action_infos[index1]->locales) continue;
			break;
		}
		if (index1 == g_action_infos_used) return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	}
		
	if (!g_action_infos[index1]->locales[index2]){
		return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	}

	result = copy_string(g_action_infos[index1]->locales[index2],buffer,length);
	if (!result){
		*pos_id = (index1<<8)+index2+1;
	}
	return result;
}

javacall_result javacall_chapi_get_local_action_name(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string action, javacall_const_utf16_string locale, javacall_utf16*  buffer, int* length){
	int result;
	handler_info* handler=0;
	int i,j;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_get_local_action_name(%s,%s,%s)\n",content_handler_id,action,locale);
#endif

	result = update_registry();
	if (result) return result;
	
	//bound to handler
	if (content_handler_id){
		handler = find_handler(content_handler_id);
		if (!handler) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
	}

	for (i=0;i < g_action_infos_used;++i){
		if (handler && g_action_infos[i]->handler != handler) continue;
		if (!javautil_str_wcsicmp(g_action_infos[i]->actionname,action)) break;
	}
	if (i == g_action_infos_used) return JAVACALL_CHAPI_ERROR_NOT_FOUND;

	for (j=0;g_action_infos[i]->locales[j];++j){
		if (!javautil_str_wcsicmp(g_action_infos[i]->locales[j],locale)) {
			return copy_string(g_action_infos[i]->localnames[j],buffer,length);
		}
	}
	return JAVACALL_CHAPI_ERROR_NOT_FOUND;
}

javacall_result javacall_chapi_enum_access_allowed_callers(javacall_const_utf16_string content_handler_id, int* pos_id, /*OUT*/ javacall_utf16*  buffer, int* length){
	handler_info* info;
	int index = *pos_id;
	int res;
	if (!index){
	 res = update_registry();
	 if (res) return res;
	}
#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_access_allowed_callers(%s,%d)\n",content_handler_id,*pos_id);
#endif

	info=find_handler(content_handler_id);
	if (!info) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
	if (!info->access_list || !(info->access_list[index])){
		return JAVACALL_CHAPI_ERROR_NO_MORE_ELEMENTS;
	}
	
	res = copy_string(info->access_list[index],buffer,length);
	if (!res) {
		*pos_id = ++index;
	}
	return res;
}

javacall_result javacall_chapi_get_content_handler_friendly_appname(javacall_const_utf16_string content_handler_id, /*OUT*/ javacall_utf16*  buffer, int* length){
	handler_info* info;
	int res;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_get_content_handler_friendly_appname(%s)\n",content_handler_id);
#endif

	res = update_registry();
	if (res) return res;
	info=find_handler(content_handler_id);
	if (!info) return JAVACALL_CHAPI_ERROR_NOT_FOUND;
	return copy_string(info->handler_friendly_name,buffer,length);
}


javacall_result javacall_chapi_get_handler_info(javacall_const_utf16_string content_handler_id,
				   /*OUT*/
#ifdef SUITE_ID_STRING
				   javacall_utf16*  suite_id_out, int* suite_id_len,
#else
				   int*  suite_id_out,
#endif

				   javacall_utf16*  classname_out, int* classname_len,
				   javacall_chapi_handler_registration_type *flag_out){
handler_info* info;
int i;
int res;

#ifdef DEBUG_OUTPUT
wprintf(L"JAVACALL::javacall_chapi_get_handler_info(%s)\n",content_handler_id);
#endif

res = update_registry();
if (res) return res;

	for (i=0;i<g_handler_infos_used;++i){
		if (!javautil_str_wcscmp(g_handler_infos[i]->handler_id,content_handler_id)){
			info = g_handler_infos[i];
			if (info->flag & TYPE_INFO_JAVA_HANDLER){
#ifdef SUITE_ID_STRING
				if (suite_id_out) {
					res = copy_string(info->suite_id,suite_id_out,suite_id_len);
					if (res) return res;
				}
#else
				if (suite_id_out) *suite_id_out = info->suite_id;
#endif
				if (flag_out) *flag_out = info->jflag;
				if (classname_out) {
					res = copy_string(info->classname,classname_out,classname_len);
				}
				return res;
			} else {
				if (suite_id_out) *suite_id_out = 0;
				if (flag_out) *flag_out = 0;
				if (classname_out) {
					res = copy_string(info->appname,classname_out,classname_len);
				}
				return res;
			}
		}
	}
	return JAVACALL_CHAPI_ERROR_NOT_FOUND;
}

static javacall_bool is_access_allowed( const handler_info* info, javacall_const_utf16_string caller_id ) {
	if (info->flag & TYPE_INFO_JAVA_HANDLER){
	    const unsigned short** s;
		if (!caller_id || !*caller_id) return JAVACALL_TRUE; //system caller
		if (!info->access_list || !*info->access_list) return JAVACALL_TRUE;
		s = info->access_list;
		while (*s) { 
			int len = javautil_str_wcslen(*s);
			if (!javautil_str_wcsncmp(*s,caller_id,len)) return JAVACALL_TRUE;
			++s;
		}
		return JAVACALL_FALSE;
	}
	return JAVACALL_TRUE;
}

javacall_bool javacall_chapi_is_access_allowed(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string caller_id){
	int res;
	handler_info* info;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_is_access_allowed(%s)\n",content_handler_id);
#endif

	res = update_registry();
	if (res) return res;
	
	info = find_handler(content_handler_id);
	if (!info) return JAVACALL_FALSE;

    return is_access_allowed( info, caller_id );
}

javacall_bool javacall_chapi_is_action_supported(javacall_const_utf16_string content_handler_id, javacall_const_utf16_string action){
	int searched_action=0,i,res;
	handler_info* info;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_is_action_supported(%s,%s)\n",content_handler_id, action);
#endif

	res = update_registry();
	if (res) return res;

	info = find_handler(content_handler_id);
	if (!info) return JAVACALL_CHAPI_ERROR_NOT_FOUND;

	if (!action || !*action || !javautil_str_wcsicmp(action,VIEW) || !javautil_str_wcsicmp(action,OPEN)) searched_action = TYPE_INFO_ACTION_VIEW;
	else
	if (!javautil_str_wcsicmp(action,EDIT)) searched_action = TYPE_INFO_ACTION_EDIT;
	else
	if (!javautil_str_wcsicmp(action,PRINT)) searched_action = TYPE_INFO_ACTION_PRINT;
	else
	if (!javautil_str_wcsicmp(action,COMPOSE) || !javautil_str_wcsicmp(action,NEW)) searched_action = TYPE_INFO_ACTION_COMPOSE;
	else
	if (!javautil_str_wcsicmp(action,COMPOSETYPED)) searched_action = TYPE_INFO_ACTION_COMPOSETYPED;

	for (i=0;i<g_action_infos_used;++i){
		if (g_action_infos[i]->handler == info) {
			if (info->flag & TYPE_INFO_JAVA_HANDLER) {
				if (!javautil_str_wcsicmp(action,g_action_infos[i]->actionname)) return JAVACALL_TRUE;
			} else {
				if (info->flag & searched_action) return JAVACALL_TRUE;
			}
		}
	}
	
	return JAVACALL_FALSE;
}

static int removehandler_from_file(const char* pattern,const char* fname){
	FILE *fin,*fout;
	char tmpfname[MAX_BUFFER];
	char backupfname[MAX_BUFFER];
	char line[MAX_LINE];
	int found = 0;

	sprintf(tmpfname,"%s/%s",mailcaps_path,tmpnam(NULL));
	sprintf(backupfname,"%s.backup",fname);

	while (1){
		fin = fopen(fname, "rb");
		fout = fopen(tmpfname,"wb");
		if (!fin || !fout) {
			if (fin) fclose(fin);
			if (fout) {
				fclose(fout);
				remove(tmpfname);
			}
			return  JAVACALL_CHAPI_ERROR_IO_FAILURE;
		}
		while (read_line(line,MAX_LINE,fin)>0){
			if (!line[0] ||  line[0]=='#' || !strstr(line,pattern)){
				write_line(line,fout);
			} else found = 1;
		}
		fclose(fin);fin=0;
		fclose(fout);fout=0;
		if (found){
			remove(backupfname);
			rename(fname,backupfname);
			rename(tmpfname,fname);
		} else {
			remove(tmpfname);
		}
		break;
	}
	return JAVACALL_OK;
}


javacall_result javacall_chapi_unregister_handler(javacall_const_utf16_string content_handler_id){
	handler_info* handler;
	int res;
	char pattern[MAX_BUFFER];
	char *p;

#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_unregister_handler(%s)\n",content_handler_id);
#endif

	res = update_registry();
	if (res) return res;

	handler = find_handler(content_handler_id);
	if (!handler) return JAVACALL_CHAPI_ERROR_NOT_FOUND;

	if (!handler->flag & TYPE_INFO_JAVA_HANDLER) return JAVACALL_CHAPI_ERROR_ACCESS_DENIED;

	p = pattern;
	p += sprintf(p,"x-handlerid='");
	p += append_string(p, content_handler_id);
	p += sprintf(p,"\';");

	res = removehandler_from_file(pattern,mailcaps_fname);
	reset_lastread();
	if (res) return res;

	p = pattern;
	*p++ = '\'';
	p += append_string(p, content_handler_id);
	*p++ = '\'';
	*p++ = ';';
	*p++ = '\0';
	removehandler_from_file(pattern,actionmap_fname);
	reset_lastread();

	p = pattern;
	*p++ = '\'';
	p += append_string(p, content_handler_id);
	*p++ = '\'';
	*p++ = '=';
	*p++ = '\0';

	removehandler_from_file(pattern,accesslist_fname);
	reset_lastread();
	
	return res;
}

/**
 * Finish enumeration call. Clean enumeration position handle
 * This method is called after caller finished to enumerate by some parameter
 * Can be used by implementation to cleanup object referenced by pos_id if required
 *
 * @param pos_id position handle used by some enumeration method call
 * @return nothing
 */
void javacall_chapi_enum_finish(int pos_id){
	(void)pos_id;
#ifdef DEBUG_OUTPUT
	wprintf(L"JAVACALL::javacall_chapi_enum_finish(%d)\n",pos_id);
#endif

}
