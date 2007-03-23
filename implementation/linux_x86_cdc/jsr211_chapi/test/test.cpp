#include "javacall_registry.h"
#include "wchar.h"
#include "stdio.h"


int main(int argc, char* argv[])
{

	const unsigned short* content_handler_id=L"Moya.Classnaya.Programma";
	const unsigned short* content_handler_friendly_name = L"Moya Classnaya Programma";
	const unsigned short* suite_id=L"1";
	const unsigned short* class_name=L"com.sun.blablabla";
	const unsigned short* types[] = {L"blam/blam-blam",L"bluem/bluem-bluem",L"audio/wav"};
	int nTypes = sizeof(types)/sizeof(types[0]);

	const unsigned short* suffixes[] = {L".bla",L".blu"};
	int nSuffixes = sizeof(suffixes)/sizeof(suffixes[0]);

	const unsigned short* actions[] = {L"open",L"close",L"view",L"edit",L"grock"};
	int nActions = sizeof(actions)/sizeof(actions[0]);
	
	const unsigned short* locales[] = {L"en-US",L"ru-RU"};
	int nLocales = sizeof(locales)/sizeof(locales[0]);
	
	const unsigned short* action_names[]  = {
		L"open",L"close",L"view",L"edit",L"grock",
		L"otkr",L"zakr",L"smotr",L"redact",L"grokaty"
	};
	int nActionNames = sizeof(action_names)/sizeof(action_names[0]);

	const unsigned short* accesses[]  = {L"*.*"};
	int nAccesses = sizeof(accesses)/sizeof(accesses[0]);

	const registry_value_pair additional_keys[] = {
		{L"key1",L"value1"},
		{L"key2",L"value2"}
	};
	int nKeys = sizeof(additional_keys)/sizeof(additional_keys[0]);

	init_registry();
	
	register_handler(
        content_handler_id,
		content_handler_friendly_name,
		suite_id,
        class_name,
		0,
        types,nTypes,
        suffixes, nSuffixes,
        actions, nActions, 
        locales, nLocales,
        action_names, nActionNames,
        accesses, nAccesses,
		additional_keys, nKeys,
		0);

	
	int pos;
	short buf[255];
	int len;

	pos=0;
	len = 255;
	content_handler_id = L"text/html";
	while (!enum_actions(content_handler_id,&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}

	pos=0;
	len = 255;
	while (!enum_suffixes(content_handler_id,&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}

	len = 255;
	pos=0;
	while (!enum_types(content_handler_id,&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}

	len = 255;
	get_class_name(content_handler_id,  buf, &len);
	wprintf(L"%s\n",buf);

	pos=0;
	len = 255;
	wprintf(L"by type bluem/bluem-bluem:\n");
	while (!enum_handlers_by_type(L"bluem/bluem-bluem",&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}


	pos=0;
	len = 255;
	wprintf(L"by type audio/wav:\n");
	while (!enum_handlers_by_type(L"audio/wav",&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}



	int result = unregister_handler(content_handler_id);
	wprintf(L"uregistering result is %d\n",result);

	pos=0;
	len = 255;
	while (!enum_actions(content_handler_id,&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}

	pos=0;
	len = 255;
	while (!enum_suffixes(content_handler_id,&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}

	len = 255;
	pos=0;
	while (!enum_types(content_handler_id,&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}

	len = 255;
	get_class_name(content_handler_id,  buf, &len);
	wprintf(L"%s\n",buf);


	

	pos=0;
	len = 255;
	wprintf(L"by suffix .html:\n");
	while (!enum_handlers_by_suffix(L".html",&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}

	

	pos=0;
	len = 255;
	wprintf(L"by type bluem/bluem-bluem:\n");
	while (!enum_handlers_by_type(L"bluem/bluem-bluem",&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}


	pos=0;
	len = 255;
	wprintf(L"by type audio/wav:\n");
	while (!enum_handlers_by_type(L"audio/wav",&pos,buf,&len)){
		wprintf(L"%s\n",buf);
		len = 255;
	}


	return 0;
}
