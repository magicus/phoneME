#ifndef JAVACALL_INVOKE_H
#define JAVACALL_INVOKE_H

#include <windows.h>

#define PIPENAME "\\\\.\\pipe\\EmulJavaInvokerPipe"
#define INV_MSG_BUFF_SIZE 256

#if defined( _DEBUG ) && defined( _X86_ ) 
#define assert( a ) if ( !(a) ) __asm int 3
#else
#include <assert.h>
#endif

// NOTE! shouldn't contain virtual functions
class InvocationMsg{
public:
    InvocationMsg();
    
    /// returns non zero if incoming string too long
    int setMsg( wchar_t* );
    const wchar_t *getMsgPtr();

private:
    wchar_t mMsg[INV_MSG_BUFF_SIZE];
};


inline InvocationMsg::InvocationMsg(){
    *mMsg = 0;
}

inline int InvocationMsg::setMsg( wchar_t *s ){
    if ( wcslen( s ) >= ( sizeof( mMsg ) / sizeof( *mMsg ) ) )
        return -1;
    wcscpy( mMsg, s );
    return 0;
}

inline const wchar_t * InvocationMsg::getMsgPtr(){
    return mMsg;
}

#endif