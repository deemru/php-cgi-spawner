#include <winsock.h>
#include <TlHelp32.h>

#pragma comment( lib, "kernel32.lib")
#pragma comment( lib, "ws2_32.lib")

__forceinline void memzero( void * mem, size_t size ) // anti memset
{
    while( size-- )
        ( (volatile char *)mem )[size] = 0;
}

#define MAX_SPAWN_HANDLES MAXIMUM_WAIT_OBJECTS

// based on spawn-fcgi-win32.c
// found here: http://redmine.lighttpd.net/boards/2/topics/686#message-689
__forceinline void spawner( char * app, unsigned port, unsigned cgis,
                            unsigned restart_delay )
{
    SOCKET s;

    if( cgis < 1 || cgis > MAX_SPAWN_HANDLES )
        return;

    {
        WSADATA wsaData;

        if( WSAStartup( MAKEWORD( 2, 0 ), &wsaData ) )
            return;
    }

    if( -1 == ( s = socket( AF_INET, SOCK_STREAM, 0 ) ) )
        return;

    {
        struct sockaddr_in fcgi_addr_in;

        fcgi_addr_in.sin_family = AF_INET;
        fcgi_addr_in.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
        fcgi_addr_in.sin_port = htons( (unsigned short)port );

        if( -1 == ( s = socket( AF_INET, SOCK_STREAM, 0 ) ) )
            return;

        {
            int opt = 1;

            if( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt,
                sizeof( opt ) ) < 0 )
                return;
        }

        if( -1 == bind( s, ( struct sockaddr * )&fcgi_addr_in,
                        sizeof( fcgi_addr_in ) ) )
            return;
    }

    if( -1 == listen( s, SOMAXCONN ) )
        return;

    // close prior to cgis (msdn: All processes start at shutdown level 0x280)
    if( !SetProcessShutdownParameters( 0x380, SHUTDOWN_NORETRY ) )
        return;

    // php-cgis crash silently if restart delay is 1 second or more
    // (https://github.com/deemru/php-cgi-spawner/issues/3)
    if( restart_delay >= 1000 )
        SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

    {
        HANDLE hProcesses[MAX_SPAWN_HANDLES];
        PROCESS_INFORMATION pi;
        STARTUPINFOA si;
        unsigned i;

        memzero( &si, sizeof( si ) );

        si.cb = sizeof( STARTUPINFO );
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = INVALID_HANDLE_VALUE;
        si.hStdError = INVALID_HANDLE_VALUE;
        si.hStdInput = (HANDLE)s;

        for( i = 0; i < cgis; i++ )
            hProcesses[i] = INVALID_HANDLE_VALUE;

        for( ;; )
        {
            for( i = 0; i < cgis; i++ )
            {
                if( hProcesses[i] == INVALID_HANDLE_VALUE )
                {
                    if( !CreateProcessA( NULL, app, NULL, NULL, TRUE,
                        CREATE_NO_WINDOW, NULL, NULL,
                        &si, &pi ) )
                        return;

                    // close unnecessary conhost.exe
                    // (https://github.com/deemru/php-cgi-spawner/issues/4)
                    if( ( restart_delay >= 100 && restart_delay <= 200 ) ||
                        ( restart_delay >= 1100 && restart_delay <= 1200 ) )
                    {
                        PROCESSENTRY32 pe32;
                        HANDLE hSnap;

                        Sleep( restart_delay );
                        hSnap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS,
                                                          pi.dwProcessId );
                        pe32.dwSize = sizeof( pe32 );

                        if( hSnap != INVALID_HANDLE_VALUE )
                        {
                            if( Process32First( hSnap, &pe32 ) )
                            do
                            {
                                if( pe32.th32ParentProcessID == pi.dwProcessId )
                                {
                                    HANDLE hCon;

                                    hCon = OpenProcess( PROCESS_TERMINATE, 0,
                                                        pe32.th32ProcessID );

                                    if( hCon != INVALID_HANDLE_VALUE )
                                    {
                                        TerminateProcess( hCon,
                                                          CONTROL_C_EXIT );
                                        CloseHandle( hCon );
                                    }

                                    break;
                                }
                            }
                            while( Process32Next( hSnap, &pe32 ) );

                            CloseHandle( hSnap );
                        }
                    }

                    CloseHandle( pi.hThread );
                    hProcesses[i] = pi.hProcess;
                }
            }

            WaitForMultipleObjects( cgis, &hProcesses[0], FALSE, INFINITE );

            for( i = 0; i < cgis; i++ )
            {
                if( hProcesses[i] != INVALID_HANDLE_VALUE )
                {
                    DWORD dwExitCode;
                    if( !GetExitCodeProcess( hProcesses[i], &dwExitCode ) )
                        return;

                    if( dwExitCode != STILL_ACTIVE )
                    {
                        CloseHandle( hProcesses[i] );
                        hProcesses[i] = INVALID_HANDLE_VALUE;
                    }
                }
            }

            // optional restart delay
            // (https://github.com/deemru/php-cgi-spawner/issues/3)
            if( restart_delay )
                Sleep( restart_delay );
        }
    }
}

__forceinline unsigned char2num( char * str ) // no checks at all
{
    unsigned u = 0;
    char c;

    while( 0 != ( c = *str++ ) )
        u = u * 10 + c - '0';

    return u;
}

#define IS_QUOTE( c ) ( c == '"' )
#define IS_SPACE( c ) ( c == ' ' || c == '\t' )

__forceinline unsigned getargs( char * cmd, char ** argv, unsigned max )
{
    char c;
    char is_begin = 0;
    char is_quote = 0;
    unsigned argc = 0;

    while( 0 != ( c = *cmd ) )
    {
        if( !is_begin )
        {
            if( IS_SPACE( c ) )
            {
                cmd++;
                continue;
            }

            else
            {
                if( argc >= max )
                    return 0;

                is_begin = 1;
                if( IS_QUOTE( c ) )
                {
                    is_quote = 1;
                    cmd++;
                }
                argv[argc] = cmd;
                continue;
            }
        }

        if( IS_QUOTE( c ) )
        {
            if( !is_quote )
                return 0;

            is_begin = 0;
            is_quote = 0;
            *cmd = 0;
            cmd++;
            argc++;
            continue;
        }

        if( is_quote )
        {
            cmd++;
            continue;
        }

        if( IS_SPACE( c ) )
        {
            is_begin = 0;
            *cmd = 0;
            cmd++;
            argc++;
            continue;
        }

        cmd++;
    }

    if( is_begin )
        argc++;

    return argc;
}

#define ARGS_MAX 5
#define ARGS_MIN 4

void __cdecl WinMainCRTStartup()
{
    char * argv[ARGS_MAX];
    argv[4] = NULL;

    if( ARGS_MIN <= getargs( GetCommandLineA(), (char **)&argv, ARGS_MAX ) )
    {
        spawner( argv[1], char2num( argv[2] ), char2num( argv[3] ),
                 argv[4] ? char2num( argv[4] ) : 0 );
    }

    ExitProcess( 0 );
}
