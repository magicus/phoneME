#include "javacall_invoke.h"


int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
){
    DWORD retCode = 0;
    HANDLE pipe = INVALID_HANDLE_VALUE;
    try {
        pipe = CreateFile(
            TEXT(PIPENAME),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
        if ( pipe == 0 || pipe == INVALID_HANDLE_VALUE )
            throw (TCHAR *) TEXT("Seems like CLDC emulator wasn't started. Can't continue.");

        int argc;
        TCHAR **argv = CommandLineToArgvW( GetCommandLine(), &argc );
        if ( argv == NULL )
            throw (TCHAR *) 0; 

        if ( argc < 2 )
            throw (TCHAR *) TEXT("Required parameter missing"); 

        InvocationMsg msg;
        if ( msg.setMsg( argv[1] ) )
            throw (TCHAR *) TEXT("Parameter too long"); 
        
        DWORD numberOfBytesWritten;
        BOOL ret = WriteFile( pipe, &msg, sizeof( msg ), &numberOfBytesWritten, NULL );
        if ( ret == 0 )
            throw (TCHAR *) 0;
    }
    catch ( TCHAR *msg ){
        LPVOID lpMsgBuf;
        TCHAR *sCaption = TEXT("Java handlers invoker error");
        DWORD dw = GetLastError(); 
        if ( msg != 0 )
            MessageBox(NULL, msg, sCaption, MB_ICONEXCLAMATION); 
        if ( dw != 0 ){
            DWORD ret = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM,
                NULL,
                dw,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0, NULL );
            if ( ret ){
                MessageBox(NULL, (TCHAR*) lpMsgBuf, sCaption, MB_ICONEXCLAMATION);
                LocalFree(lpMsgBuf);
            }
        }
        retCode = dw;
    }
    CloseHandle( pipe );
    return retCode;
}