#ifndef JAVACALL_INVOKE_H
#define JAVACALL_INVOKE_H

#define PIPENAME "\\\\.\\pipe\\EmulJavaInvokerPipe"
#define INV_MSG_BUFF_SIZE 256

#ifdef _DEBUG
#define assert( a ) if ( !(a) ) __asm int 3
#else
#define assert( a )
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