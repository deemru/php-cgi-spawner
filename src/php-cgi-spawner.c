#include <ws2tcpip.h>

#pragma comment( lib, "kernel32.lib")
#pragma comment( lib, "ws2_32.lib")
#pragma warning( disable: 4701 )

__forceinline void memzero( void * mem, size_t size ) // anti memset
{
    do
    {
        ( (volatile char *)mem )[--size] = 0;
    }
    while( size );
}

#define MAX_SPAWN_HANDLES 256

__forceinline void spawner( char * app, unsigned port, unsigned cgis )
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

        if( -1 == bind( s, (struct sockaddr *)&fcgi_addr_in,
                        sizeof( fcgi_addr_in ) ) )
            return;
    }

    if( -1 == listen( s, SOMAXCONN ) )
        return;

    {
        HANDLE hProcesses[MAX_SPAWN_HANDLES];
        PROCESS_INFORMATION pi;
        STARTUPINFOA si;
        unsigned i;

        memzero( hProcesses, sizeof( hProcesses ) );
        memzero( &si, sizeof( si ) );

        si.cb = sizeof( STARTUPINFO );
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = INVALID_HANDLE_VALUE;
        si.hStdError = INVALID_HANDLE_VALUE;
        si.hStdInput = (HANDLE)s;

        for( ;; )
        {
            BOOL isSpawnNeed = FALSE;

            for( i = 0; i < cgis; i++ )
            {
                if( hProcesses[i] != NULL )
                {
                    DWORD dwExitCode;
                    if( !GetExitCodeProcess( hProcesses[i], &dwExitCode ) )
                        return;

                    if( dwExitCode != STILL_ACTIVE )
                    {
                        CloseHandle( hProcesses[i] );
                        hProcesses[i] = NULL;
                        isSpawnNeed = TRUE;
                    }
                }
                else
                {
                    isSpawnNeed = TRUE;
                }
            }

            if( isSpawnNeed )
            {
                for( i = 0; i < cgis; i++ )
                {
                    if( hProcesses[i] == NULL )
                    {
                        if( !CreateProcessA( NULL, app, NULL, NULL, TRUE,
                                             CREATE_NO_WINDOW, NULL, NULL,
                                             &si, &pi ) )
                            return;

                        CloseHandle( pi.hThread );
                        hProcesses[i] = pi.hProcess;
                    }
                }
            }

            WaitForMultipleObjects( cgis, &hProcesses[0], FALSE, INFINITE );
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

#define ARGS_COUNT 4

void __cdecl WinMainCRTStartup()
{
    char * argv[ARGS_COUNT];

    if( ARGS_COUNT == getargs( GetCommandLineA(), (char **)&argv, ARGS_COUNT ) )
        spawner( argv[1], char2num( argv[2] ), char2num( argv[3] ) );

    ExitProcess( 0 );
}
