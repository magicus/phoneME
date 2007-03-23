/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * @brief javacall registry access implementation for unix mailcap files (rfc 1524)
 */

#include "javacall_registry.h"

#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
//#include <wchar.h>
#include <memory.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#define DATABASE_INCREASE_SIZE 128
#define MAX_BUFFER 512

#define TYPE_INFO_COMPOSETYPED 0x1
#define TYPE_INFO_NEEDSTERMINAL 0x2
#define TYPE_INFO_COPIOUSOUTPUT 0x4
#define TYPE_INFO_TEXTUALNEWLINES 0x8
#define TYPE_INFO_JAVA_HANDLER 0x10

#define TYPE_INFO_USE_REFFERENCE 0x100

#define TYPE_INFO_ACTION_MASK 0x0F000000
#define TYPE_INFO_ACTION_VIEW 0x01000000
#define TYPE_INFO_ACTION_EDIT 0x02000000
#define TYPE_INFO_ACTION_PRINT 0x04000000
#define TYPE_INFO_ACTION_COMPOSE 0x08000000

typedef struct _mailcap_type_info{
	char* type;
	char* handler;
	char* params;
	char* description;
	char* test;
	char* nametemplate;
	int flag;
} mailcap_type_info;


static int get_vm_path(char* buffer){
	strcpy(buffer,"C:\\TI\\WORK\\jsr211\\Invoker\\Debug\\Invoker.exe");
	return strlen(buffer);
}

static int get_icon_path(char* buffer){
	strcpy(buffer,"C:\\TI\\WORK\\jsr211\\dukedoc.bmp");
	return strlen(buffer);
}


#define INFO_INC 128


char* mailcaps_fname = 0;
const char* java_invoker = "${VM_INVOKER}";
const char* java_type = "application/x-javame-content-handler";

mailcap_type_info** g_infos = 0;
int g_infos_used;
int g_infos_allocated;
const char* VIEW = "view";
const char* EDIT = "edit";
const char* PRINT = "print";
const char* COMPOSE = "compose";
const char* OPEN = "open";
const char* NEW = "new";

#define CHAPI_READ 1
#define CHAPI_WRITE 2
#define CHAPI_APPEND 3

#define chricmp(a,b) (((a>='A' && a<='Z') ? a+('a'-'A'): a) == ((b>='A' && b<='Z') ? b+('a'-'A'): b))

int open_db(int* file, int flag);
void close_db(int file);

static int wasiequal(const short* ws, const char* s){
	while (*ws && *s && chricmp(*ws,*s)){
		++ws;
		++s;
	}
	return (!*ws && !*s);
}

static mailcap_type_info* new_info(){
	mailcap_type_info* info;
	if (!g_infos){
		g_infos = (mailcap_type_info**)malloc(sizeof(mailcap_type_info*)*INFO_INC);
		if (!g_infos) return 0;
		g_infos_allocated = INFO_INC;
		g_infos_used = 0;
	}
	if (g_infos_used>=g_infos_allocated){
		mailcap_type_info** tmp = realloc(g_infos,sizeof(mailcap_type_info*)*(g_infos_allocated+INFO_INC));
		if (!tmp) return 0;
		g_infos = tmp;
		g_infos_allocated += INFO_INC;
	}
	info = (mailcap_type_info*)malloc(sizeof(mailcap_type_info));
	if (!info) return 0;
	memset(info,0,sizeof(mailcap_type_info));
	g_infos[g_infos_used++] = info;
	return info;
}

static void free_info(mailcap_type_info* info){
	if (!(info->flag & TYPE_INFO_USE_REFFERENCE)){
		if (info->type) free(info->type);
		if (info->description) free(info->description);
		if (info->test) free(info->test);
		if (info->nametemplate) free(info->nametemplate);
	}

	if (info->handler) free(info->handler);
	if (info->params) free(info->params);
	free(info);
}

static void pop_info(){
	if (g_infos_used) free_info(g_infos[--g_infos_used]);
}


/**
*   reads new not empty not commented line from file
*	line and max_size should not be zero
**/
static int get_line(char* line, int max_size, int f){
	long pos = _tell(f);
	int i=0,count=0;
	while (_read(f,&i,1)==1){
		if ((char)i=='#'){
			while (_read(f,&i,1)==1 && (char)i!='\n');
		}
		if ((char)i!='\n' && (char)i!='\r') break;
	}
	while (i!=EOF) {
		if ((char)i=='\n') break;
		if (count>=max_size-1) {
			//buffer too small return
			_lseek(f,pos,SEEK_SET);
			return -1;
		}
		line[count++]=(char)i;
		if (_read(f,&i,1)!=1) break;
	};
	line[count]=0;
	return count;
}


//find next tooken that ends on ; and can contain key and value separated by =
static int next_key(const char* line,const char** kstart,const char** kend, const char** valstart,const char** valend, const char** tokenend){
	int p=0,quot=0,dquot=0,endword=0;
	while (line[p] && (line[p]==' ' || line[p]=='\t')) ++p; //trim left
	if(!line[p]) return 0;
	endword=p;
	if (kstart) *kstart = &line[p];
	if (kend) *kend = &line[p];
	if (valstart) *valstart = 0;
	if (valend) *valstart = 0;

	while (line[p]){
		if (line[p]=='=' && !quot && !dquot){ // value found
			if (kend) *kend = &line[endword];
			while (line[p] && (line[p]==' ' || line[p]=='\t')) ++p; //trim left
			if (valstart) *valstart = &line[p];
			if (valend) *valend = &line[p];
			endword=p;
			while (line[p]){
				if (line[p]==';' && !quot && !dquot){
					if (valend) *valend = &line[endword];
					p++;
					break;
				}
				if (line[p]=='\''&& !dquot){ quot = !quot;}
				if (line[p]=='\"'&& !quot){ dquot = !dquot;}
				if (line[p]!=' ' && line[p]!='\t') endword=p; //trim right
				++p;
			}
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


static int match(const char* p1, const char* pend, const char* p2){
	if (!p1 || !p2) return 0;
	while (*p1 && (p1<pend) &&  *p2 && chricmp(*p1,*p2))
	{
		++p1;
		++p2;
	}
	return (!*p2);
}

static int get_id(mailcap_type_info* info, /*OUT*/ short*  buffer, int* length){
	char* t = info->type;
	char* h = info->handler;
	int len = strlen(t)+strlen(h)+2;
	if (!length || (*length && !buffer)) return ERROR_BAD_PARAMS;
	if (!*length) {
		*length = len;
	} else {
		if (*length < len){
			*length = len;
			return ERROR_BUFFER_TOO_SMALL;
		}
		while (*t) *buffer++ = *t++;
		*buffer++ = ';';
		while (*h) *buffer++ = *h++;
		*buffer=0;
		*length = len;
	}
	return 0;
}

static int id_match(const mailcap_type_info* info, /*OUT*/ const unsigned short*  id){
	char* s;
	if (!id) return 0;
	s = info->type;
	while (*id && *s && chricmp(*id,*s)){id++;s++;}
	if (*s || *id!=';') return 0;
	s = info->handler;
	while (*id && *s && chricmp(*id,*s)){id++;s++;}
	return (!*s && !*id);
}


static int extract_handler(const char* command,char** handler, char** params){
	int quoted = 0;
	int dquoted = 0;
	const char* c = command;
	const char* start,*end;
	int len;

	while (*c && (*c==' ' || *c=='\t')) ++c; //trim left
	start = c;

	while (*c){
		if (!(quoted || dquoted) && (*c==' ' || *c=='\t')) break;
		if (quoted && *c=='\'') quoted=!quoted;
		if (dquoted && *c=='\"') dquoted=!dquoted;
		c++;
	}

	len = c-start;
	*handler = malloc(len+1);
	if (!*handler) return ERROR_NO_MEMORY;
	memcpy(*handler,start,len);
	(*handler)[len]=0;

	while (*c && (*c==' ' || *c=='\t')) ++c; //trim left
	start = end = c;

	while (*c){
		if ((*c!=' ' && *c!='\t')) end=c; //trim right
		c++;
	}

	len = end-start+1;
	*params = malloc(len+1);
	if (!*params) return ERROR_NO_MEMORY;
	memcpy(*params,start,len);
	(*params)[len]=0;

	return 0;
}

static int duplicate_info(mailcap_type_info* src_info, int action, const char* command){

	mailcap_type_info* info = new_info();
	if (!info) return ERROR_NO_MEMORY;


	extract_handler(command,&info->handler,&info->params);
	if (!info->handler || !info->params){
		pop_info();
		return ERROR_NO_MEMORY;
	}

	info->flag = (src_info->flag & (~TYPE_INFO_ACTION_MASK)) | action | TYPE_INFO_USE_REFFERENCE;

	//just refer other params
	info->type = src_info->type;
	info->description = src_info->description;
	info->test=src_info->test;
	info->nametemplate;src_info->nametemplate;

	return 0;
}

static char* substring(const char* str_begin, const char* str_end){
	int len;
	char* buf;
	if (!str_begin) return 0;
	len = str_end - str_begin + 1;
	buf = malloc(len+1);
	if (!buf) return 0;
	memcpy(buf,str_begin,len);
	buf[len]=0;
	return buf;
}


//read mime-type handlers information from mailcaps file according to rfc1343
static int read_caps(){
	char* p, *ks, *ke, *vs, *ve;
	char *edit=0, *print=0, *compose=0, *command;
	mailcap_type_info* info = 0;
	int f;
	char* line;
	int length = MAX_BUFFER;
	int res = ERROR_NO_MEMORY;
	
	line = malloc(length);
	if (!line) return ERROR_NO_MEMORY;

	open_db(&f,CHAPI_READ);
	if (!f) {
		free(line);
		return -1;
	}

	while ((res=get_line(line,length,f))){

		if (res < 0) { //buffer too small
			length*=2;
			free(line);
			line = malloc(length);
			if (!line) break;
			continue;
		}

		p = line;
		info = new_info();
		if (!info) break;

		if (!next_key(p,&ks,&ke,0,0,&p)) break;
		info->type=substring(ks,ke);
		if (!info->type) break;

		if (!next_key(p,&ks,&ke,0,0,&p)) break;
		command=substring(ks,ke);
		if (!command) break;

		if (extract_handler(command,&info->handler,&info->params)) break;

		info->flag=TYPE_INFO_ACTION_VIEW;

		while ((next_key(p,&ks,&ke,&vs,&ve,&p))){
			if (match(ks, ke,"edit")) {edit = substring(vs,ve);continue;}
			if (match(ks, ke,"print")) {print = substring(vs,ve);continue;}
			if (match(ks, ke,"compose")) {compose = substring(vs,ve);continue;}

			if (match(ks, ke,"description")) {info->description = substring(vs,ve);continue;}
			if (match(ks, ke,"test")) {info->test = substring(vs,ve);continue;}
			if (match(ks, ke,"nametemplate")) {info->nametemplate = substring(vs,ve);continue;}
			if (match(ks, ke,"composetyped")) {info->flag |= TYPE_INFO_COMPOSETYPED; continue;}
			if (match(ks, ke,"needsterminal")) {info->flag |= TYPE_INFO_NEEDSTERMINAL; continue;}
			if (match(ks, ke,"copiousoutput")) {info->flag |= TYPE_INFO_COPIOUSOUTPUT; continue;}
			if (match(ks, ke,"textualnewlines")) {info->flag |= TYPE_INFO_TEXTUALNEWLINES; continue;}
			if (match(ks, ke,"x-java")) {info->flag |= TYPE_INFO_JAVA_HANDLER;continue;}
		}
		if (command) free(command);
		if (edit) {duplicate_info(info,TYPE_INFO_ACTION_EDIT,edit);free(edit);}
		if (print) {duplicate_info(info,TYPE_INFO_ACTION_PRINT,print);free(print);}
		if (compose) {duplicate_info(info,TYPE_INFO_ACTION_COMPOSE,compose);free(compose);}
		info = 0;
}
if (info) pop_info();
if (line) free(line);
close_db(f);
return 0;
}



static int copy_string(const char* str, /*OUT*/ short*  buffer, int* length){
	int len = strlen(str)+1;
	if (!length || (*length && !buffer)) return ERROR_BAD_PARAMS;
	if (!*length) {
		*length = len;
	} else {
		if (*length < len){
			*length = len;
			return ERROR_BUFFER_TOO_SMALL;
		}
		while (*str) *buffer++ = *str++;
		*buffer=0;
		*length = len;
	}
	return 0;
}


static int suffix_fits(const short* suffix, mailcap_type_info* info){
	char buffer[MAX_BUFFER],*b=buffer;
	if (!suffix || !info || !info->nametemplate) return 0;
	*b++='%';*b++='s';
	while (*suffix) *b++=(char)*suffix++;
	return stricmp(b,info->nametemplate);
}

static int type_fits(const short* type, mailcap_type_info* info){
	char *t;
	if (!type || !info) return 0;
	t = info->type;
	while (*type && *t) {
		if (!chricmp(*type,*t)) return 0;
		if (*t == '/' && (!*(t+1) || *(t+1)=='*')) return 1; //check implicit and explicit wildcards
		++type;
		++t;
	}
	return (!*type) && (!*t);
}

static int has_action(const short* action, mailcap_type_info* info){
	if (!info) return 0;
	if (!action || !*action || wasiequal(action,VIEW) || wasiequal(action,"open")) return ((info->flag & TYPE_INFO_ACTION_VIEW) !=0);
	if (wasiequal(action,EDIT)) return ((info->flag & TYPE_INFO_ACTION_EDIT) !=0);
	if (wasiequal(action,COMPOSE) || wasiequal(action,"new")) return ((info->flag & TYPE_INFO_ACTION_COMPOSE) !=0);
	if (wasiequal(action,PRINT)) return ((info->flag & TYPE_INFO_ACTION_PRINT) !=0);
	return 0;
}

static int file_exists(const char* fname){
	int file = _open(fname,_O_RDONLY);
	if (file){
		_close(file);
		return 1;
	}
	return 0;
}

static int open_db(int* file, int flag){
	if (flag == CHAPI_READ) {
		handlerdb_global_lock(1);
		*file = _open(mailcaps_fname,_O_RDONLY);
	} else 	if (flag == CHAPI_WRITE) {
		handlerdb_global_lock(0);
		*file = _open(mailcaps_fname,_O_RDWR);
	} else if (flag == CHAPI_APPEND){
		handlerdb_global_lock(0);
		*file = _open(mailcaps_fname, _O_RDWR | _O_APPEND | _O_CREAT, _S_IWRITE);
	} else 
		return ERROR_BAD_PARAMS;

	if (*file<0) {
		handlerdb_global_unlock();
		return ERROR_IO_FAILURE;
	}

	return 0;
}


static void close_db(int file){
	if (file) _close(file);
	handlerdb_global_unlock();
}

/**********************************************************************************************************************/
/**
/**	Functions implemented by platform
/**
/**********************************************************************************************************************/


/**
 * Lock access to native handlers database
 *
 * @return JAVACALL_OK if lock aquired JAVACALL_FAIL otherwise
 */
int handlerdb_global_lock(int allow_read){
	//if (!allow_read) lock_read_locker();
	//lock_write_locker();
	return 0;
}

/**
 * Unlock access to native handlers database
 *
 */
void handlerdb_global_unlock(void){
	//unlock_read_locker();
	//unlock_write_locker();
}

/**
 * Check native database was modified since time pointed in lastread
 *
 */
int handlerdb_is_modified(long lastread){
	struct _stat st;
	_stat(mailcaps_fname,&st);
	return (st.st_mtime > (time_t)lastread);
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

int init_registry(){
	char buf[MAX_BUFFER];
	
	while (1){
		char* path = getenv("MAILCAPS");
		if (path) {
			mailcaps_fname = path;
			break;
		}

		mailcaps_fname = "./mailcap";
		if (file_exists(mailcaps_fname)) break;

		sprintf(buf,"%s/.mailcap",getenv("HOME"));
		if (file_exists(buf)) {
			mailcaps_fname = buf;
			break;
		}

		mailcaps_fname = "/mailcap";
		if (file_exists(mailcaps_fname)) break;

		mailcaps_fname = "/etc/mailcap";
		if (file_exists(mailcaps_fname)) break;

		mailcaps_fname = "/usr/etc/mailcap";
		if (file_exists(mailcaps_fname)) break;

		mailcaps_fname = "/usr/local/etc/mailcap";
		if (file_exists(mailcaps_fname)) break;

		mailcaps_fname = buf;
	}

	mailcaps_fname = strdup(mailcaps_fname);

	read_caps();
	return 0;
}

int finalize_registry(){
	if (mailcaps_fname) free(mailcaps_fname);
	mailcaps_fname = 0;
	return 0;
}

//register handler
int register_handler(
        const unsigned short* content_handler_id,
		const unsigned short* content_handler_friendly_name,
		const unsigned short* suite_id,
        const unsigned short* class_name,
		int flag,
        const unsigned short** types,     int nTypes,
        const unsigned short** suffixes,  int nSuffixes,
        const unsigned short** actions,   int nActions,  
        const unsigned short** locales,   int nLocales,
        const unsigned short** action_names, int nActionNames,
        const unsigned short** accesses,  int nAccesses,
		const registry_value_pair* additional_keys, int nKeys,
		const unsigned short* default_icon_path){

	int result;
	int file=0,len;
	int itype,iact;
	char buf[MAX_BUFFER],*b;

	result = open_db(&file,CHAPI_APPEND);
	if (result != 0) return result;

	for (itype=0;itype<nTypes || !nTypes;itype++){
		b=buf;

		if (!nTypes) 
			b += sprintf(b,"%s;",java_type);
		else 
			b += sprintf(b,"%S;",types[itype]);
		
		b += sprintf(b,"%s -suite \'%S\' -class \'%S\' \'%%s\';",java_invoker,suite_id,class_name);
		b += sprintf(b,"test=test -n \"$DISPLAY\";");
		if (content_handler_friendly_name){
			b += sprintf(b,"description=%S Document;",content_handler_friendly_name);
		}
		if (nSuffixes && nSuffixes>itype){
			b += sprintf(b,"nametemplate=%%s%S;",suffixes[itype]);
		}
		for (iact=0;iact<nActionNames;iact++){
			const short* action = action_names[iact];
			const char* taction;

			if (wasiequal(action,EDIT)) {
				taction=EDIT;
			} else if (wasiequal(action,COMPOSE)){
					taction=COMPOSE;
			} else if (wasiequal(action,PRINT)){
					taction=PRINT;
			} else {
				continue;
			}

			b += sprintf(b,"%s=%s -suite \'%S\' -class \'%S\' -action %S \'%%s\';",
							taction,java_invoker,suite_id,class_name,action);
			
		}
		
	

		b += sprintf(b,"x-java;\n");

		len=b-buf;
		if (_write(file,buf,len)!=len) {
			result = ERROR_IO_FAILURE;
			break;
		}
		if (!nTypes) break;
	}

	close_db(file);
	return result;
}


//NOTE:handlers are not unique
int enum_handlers(int* pos_id, /*OUT*/ short*  buffer, int* length){
	int result;
	if (!pos_id) return ERROR_BAD_PARAMS;
	if (!g_infos || g_infos_used<*pos_id) return ERROR_NO_MORE_ELEMENTS;
	result=get_id(g_infos[*pos_id],buffer,length);
	if (!result){
		*pos_id++;
	}
	return result;
}

int enum_handlers_by_suffix(const unsigned short* suffix, int* pos_id, /*OUT*/ short*  buffer, int* length){
	int result = ERROR_NO_MORE_ELEMENTS,index=*pos_id,found=0;
	if (!pos_id) return ERROR_BAD_PARAMS;
	if (!g_infos) return result;
	while (index < g_infos_used){
		if (suffix_fits(suffix,g_infos[index])){
			found = 1;
			break;
		}
		index++;
	}

	if (found){
	    result=get_id(g_infos[*pos_id],buffer,length);
		if (!result){
			*pos_id = index+1;
		}
	}

	return result;
}

int enum_handlers_by_type(const unsigned short* mimetype, int* pos_id, /*OUT*/ short*  buffer, int* length){
	int result = ERROR_NO_MORE_ELEMENTS,index=*pos_id,found=0;
	if (!pos_id) return ERROR_BAD_PARAMS;
	if (!g_infos) return result;
	while (index < g_infos_used){
		if (type_fits(mimetype,g_infos[index])){
			found = 1;
			break;
		}
		index++;
	}

	if (found){
		result=get_id(g_infos[*pos_id],buffer,length);
		if (!result){
			*pos_id = index+1;
		}
	}

	return result;
}

int enum_handlers_by_action(const unsigned short* action, int* pos_id, /*OUT*/ short*  buffer, int* length){
	int result = ERROR_NO_MORE_ELEMENTS,index=*pos_id;
	int searched_action=0;

	if (!g_infos) return result;
		
	if (!action || !*action || wasiequal(action,VIEW) || wasiequal(action,"open")) searched_action = TYPE_INFO_ACTION_VIEW;
	else
	if (wasiequal(action,EDIT)) searched_action = TYPE_INFO_ACTION_EDIT;
	else
	if (wasiequal(action,PRINT)) searched_action = TYPE_INFO_ACTION_PRINT;
	else
	if (wasiequal(action,COMPOSE) || wasiequal(action,"new")) searched_action = TYPE_INFO_ACTION_COMPOSE;

	if (!searched_action) return ERROR_NO_MORE_ELEMENTS;

	while (index < g_infos_used){
		if (g_infos[index]->flag & searched_action){
			result=get_id(g_infos[*pos_id],buffer,length);
			if (!result){
				*pos_id = index+1;
			}
			break;
		}
		index++;
	}

	return result;
}

int enum_handlers_by_suit_id(const unsigned short* suit_id, int* pos_handle, /*OUT*/ short*  buffer, int* length){
	return ERROR_NO_MORE_ELEMENTS;
}

int enum_actions(const unsigned short* content_handler_id, /*OUT*/ int* pos_id, short*  buffer, int* length){
	int result = ERROR_NO_MORE_ELEMENTS;
	int actions_found = (*pos_id) & TYPE_INFO_ACTION_MASK;
	int index = (*pos_id) & 0xFFFFFF;

	if (!pos_id) return ERROR_BAD_PARAMS;
	if (!g_infos) return result;

	//only 4 actions suported
	if (actions_found==TYPE_INFO_ACTION_MASK) return ERROR_NO_MORE_ELEMENTS;

	for (;index < g_infos_used;++index){
		if (id_match(g_infos[index],content_handler_id)){
			if (!(actions_found & g_infos[index]->flag & TYPE_INFO_ACTION_MASK)){
				const char* action;
				if (g_infos[index]->flag & TYPE_INFO_ACTION_VIEW) action = VIEW; else
					if (g_infos[index]->flag & TYPE_INFO_ACTION_EDIT) action = EDIT; else
						if (g_infos[index]->flag & TYPE_INFO_ACTION_PRINT) action = PRINT; else
							if (g_infos[index]->flag & TYPE_INFO_ACTION_COMPOSE) action = COMPOSE; else 
								continue;
				result = copy_string(action,buffer,length);
				if (!result){
					actions_found = actions_found | (g_infos[index]->flag & TYPE_INFO_ACTION_MASK);
					*pos_id = actions_found + index+1;
				}
				break;
			}
		}
	}

	return result;
}

int enum_action_locales(const unsigned short* content_handler_id, /*OUT*/ int* pos_id, short*  buffer, int* length){
	return ERROR_NO_MORE_ELEMENTS;
}

int get_local_action_name(const unsigned short* content_handler_id, const unsigned short* action, const unsigned short* locale, short*  buffer, int* length){
	return ERROR_NOT_FOUND;
}

//NOTE: not unique
int enum_suffixes(const unsigned short* content_handler_id, int* pos_id, /*OUT*/ short*  buffer, int* length){
int index = *pos_id, result = ERROR_NO_MORE_ELEMENTS;
	while (index < g_infos_used){
		if (id_match(g_infos[index],content_handler_id) && g_infos[index]->nametemplate && g_infos[index]->nametemplate[0]=='%' && g_infos[index]->nametemplate[1]=='s'){
			result = copy_string(&g_infos[index]->nametemplate[2],buffer,length);
			if (!result){
				*pos_id = index+1;
			}
			break;
		}
		index++;
	}

	return result;
}

//NOTE: not unique
int enum_types(const unsigned short* content_handler_id, /*OUT*/ int* pos_id, short*  buffer, int* length){
	int result =ERROR_NO_MORE_ELEMENTS;
	int index = *pos_id;

	while (index < g_infos_used){
			result = copy_string(g_infos[index]->type,buffer,length);
			if (!result){
				*pos_id = index+1;
			}
			break;
		index++;
	}

	return result;
}

int enum_trusted_callers(const unsigned short* content_handler_id, int* pos_id, /*OUT*/ short*  buffer, int* length){
	return ERROR_NOT_FOUND;
}

int get_class_name(const unsigned short* content_handler_id, /*OUT*/ short*  buf, int* length){
	return ERROR_NOT_FOUND;
}

int get_content_handler_friendly_name(const unsigned short* content_handler_id, /*OUT*/ short*  buffer, int* length){
	int i;
	mailcap_type_info* info;
	for (i=0;i<g_infos_used;++i){
		if (id_match(g_infos[i],content_handler_id)){
			info = g_infos[i];
			return copy_string(info->handler,buffer,length);
		}
	}
	return ERROR_NOT_FOUND;
}

int get_suite_id(const unsigned short* content_handler_id, /*OUT*/ short*  buffer, int* length){
	return ERROR_NOT_FOUND;
}

int get_flag(const unsigned short* content_handler_id,/*OUT*/  int* val){
	return ERROR_NOT_FOUND;
}

int get_handler_info(const unsigned short* content_handler_id,
				   /*OUT*/  short*  suit_id, int* suit_id_len,
				   short*  classname, int* classname_len,
				   int *flag){
	return ERROR_NOT_FOUND;
}

int is_trusted(const unsigned short* content_handler_id, const unsigned short* caller_id){
	return 1;
}

int is_action_supported(const unsigned short* content_handler_id, const unsigned short* action){
	int searched_action,i;
	mailcap_type_info* info;

	if (!action || !*action || wasiequal(action,VIEW) || wasiequal(action,OPEN)) searched_action = TYPE_INFO_ACTION_VIEW;
	else
	if (wasiequal(action,EDIT)) searched_action = TYPE_INFO_ACTION_EDIT;
	else
	if (wasiequal(action,PRINT)) searched_action = TYPE_INFO_ACTION_PRINT;
	else
	if (wasiequal(action,COMPOSE) || wasiequal(action,NEW)) searched_action = TYPE_INFO_ACTION_COMPOSE;

	for (i=0;i<g_infos_used;++i){
		if (id_match(g_infos[i],content_handler_id)){
			info = g_infos[i];
			if (info->flag & searched_action) return 1;
		}
	}
	
	return 0;
}

int unregister_handler(const unsigned short* content_handler_id){
	//find by x-java and x-java-content-handler-id
	return ERROR_ACCESS_DENIED;
}
