#define FD_SETSIZE 1
#include <winsock.h>

#pragma comment( lib, "kernel32.lib")
#pragma comment( lib, "ws2_32.lib")

#define MAX_SPAWN_HANDLES MAXIMUM_WAIT_OBJECTS

typedef struct _PHPSPWCTX
{
    SOCKET s;
    CRITICAL_SECTION cs;
    char * cmd;
    unsigned port;
    unsigned fcgis;
    unsigned helpers;
    unsigned restart_delay;
    char PHP_FCGI_MAX_REQUESTS[16];
    char PHP_HELP_MAX_REQUESTS[16];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    HANDLE hFCGIs[MAX_SPAWN_HANDLES];
    unsigned helpers_delay;
    volatile LONG helpers_running;
} PHPSPWCTX;

static PHPSPWCTX ctx;

static void inline memsym( void * mem, size_t size, char sym )
{
    while( size-- )
        ( (volatile char *)mem )[size] = sym;
}

static char spawn_fcgi( HANDLE * hFCGI, BOOL is_perm )
{
    char isok = 1;

    EnterCriticalSection( &ctx.cs );

    for( ;; )
    {
        // set correct PHP_FCGI_MAX_REQUESTS
        {
            char * val;

            if( is_perm )
                val = ctx.PHP_FCGI_MAX_REQUESTS;
            else
                val = ctx.PHP_HELP_MAX_REQUESTS;

            if( val[0] == 0 )
                val = NULL;

            SetEnvironmentVariableA( "PHP_FCGI_MAX_REQUESTS", val );
        }

        if( !CreateProcessA( NULL, ctx.cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW,
                             NULL, NULL, &ctx.si, &ctx.pi ) )
        {
            isok = 0;
            break;
        }

        CloseHandle( ctx.pi.hThread );
        *hFCGI = ctx.pi.hProcess;
        break;
    }

    LeaveCriticalSection( &ctx.cs );

    return isok;
}

static DWORD WINAPI helper_holder( HANDLE hFCGI )
{
    WaitForSingleObject( hFCGI, INFINITE );
    CloseHandle( hFCGI );
    InterlockedDecrement( &ctx.helpers_running );
    return 0;
}

static DWORD WINAPI helpers_thread( void * unused )
{
    (void)unused;

    struct timeval tv = { 0, 0 };
    struct timeval * timeout;

    for( ;; )
    {
        DWORD dwTick = GetTickCount();
        timeout = &tv;

        for( ;; )
        {
            int err;

            fd_set fs;
            fs.fd_count = 1;
            fs.fd_array[0] = ctx.s;

            err = select( 0, &fs, NULL, NULL, timeout );

            if( err == SOCKET_ERROR )
                return 0;

            if( timeout )
            {
                if( err == 0 )
                {
                    timeout = NULL;
                    continue;
                }

                if( GetTickCount() - dwTick > ctx.helpers_delay )
                    timeout = NULL;
            }

            if( err == 1 && timeout == NULL )
                break;

            if( timeout )
                Sleep( 1 );
        }

        if( ctx.helpers_running >= (LONG)ctx.helpers )
            continue;

        InterlockedIncrement( &ctx.helpers_running );

        {
            HANDLE h;

            if( !spawn_fcgi( &h, FALSE ) )
                break;

            h = CreateThread( NULL, 0, &helper_holder, h, 0, NULL );

            if( h == NULL )
                break;

            CloseHandle( h );
        }
    }

    return 0;
}

static void perma_thread( BOOL helpers )
{
    for( ;; )
    {
        unsigned i;

        for( i = 0; i < ctx.fcgis; i++ )
        {
            if( ctx.hFCGIs[i] == INVALID_HANDLE_VALUE &&
                !spawn_fcgi( &ctx.hFCGIs[i], TRUE ) )
                return;
        }

        if( helpers )
        {
            HANDLE h;

            h = CreateThread( NULL, 0, &helpers_thread, NULL, 0, NULL );

            if( h == NULL )
                return;

            CloseHandle( h );
            helpers = FALSE;
        }

        if( ctx.fcgis == 0 )
            Sleep( INFINITE );

        WaitForMultipleObjects( ctx.fcgis, ctx.hFCGIs, FALSE, INFINITE );

        for( i = 0; i < ctx.fcgis; i++ )
        {
            if( ctx.hFCGIs[i] != INVALID_HANDLE_VALUE )
            {
                DWORD dwExitCode;
                if( !GetExitCodeProcess( ctx.hFCGIs[i], &dwExitCode ) )
                    return;

                if( dwExitCode != STILL_ACTIVE )
                {
                    CloseHandle( ctx.hFCGIs[i] );
                    ctx.hFCGIs[i] = INVALID_HANDLE_VALUE;
                }
            }
        }

        // optional restart delay
        // https://github.com/deemru/php-cgi-spawner/issues/3
        if( ctx.restart_delay )
            Sleep( ctx.restart_delay );
    }
}

static unsigned char2num( char * str )
{
    unsigned u = 0;
    char c;

    while( 0 != ( c = *str++ ) )
        u = u * 10 + c - '0';

    return u;
}

static char * getshift( char * str, char sym )
{
    char c;

    for( ;; )
    {
        c = *str;

        if( c == 0 )
            break;

        if( c == sym )
            return str;

        str++;
    }

    return NULL;
}

#define IS_QUOTE( c ) ( c == '"' )
#define IS_SPACE( c ) ( c == ' ' || c == '\t' )

static unsigned getargs( char * cmd, char ** argv, unsigned max )
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

void __cdecl WinMainCRTStartup( void )
{
    char * argv[ARGS_MAX];
    argv[4] = NULL;

    for( ;; )
    {
        BOOL is_helpers = FALSE;

        if( ARGS_MIN > getargs( GetCommandLineA(), (char **)&argv, ARGS_MAX ) )
            break;

        ctx.cmd = argv[1];
        ctx.port = char2num( argv[2] );

        // permanent fcgis + helpers count
        {
            char * helpers_shift = getshift( argv[3], '+' );

            if( helpers_shift )
            {
                *helpers_shift = 0;

                ctx.helpers = char2num( helpers_shift + 1 );
                if( ctx.helpers )
                    is_helpers = TRUE;
            }
        }

        ctx.fcgis = char2num( argv[3] );
        ctx.restart_delay = argv[4] ? char2num( argv[4] ) : 0;
        ctx.helpers_delay = ctx.restart_delay ? ctx.restart_delay : 100;

        if( ( ctx.fcgis < 1 && !is_helpers ) || ctx.fcgis > MAX_SPAWN_HANDLES )
            break;

        // SOCKET
        {
            WSADATA wsaData;
            struct sockaddr_in fcgi_addr_in;
            int opt = 1;

            if( WSAStartup( MAKEWORD( 2, 0 ), &wsaData ) )
                break;

            if( -1 == ( ctx.s = socket( AF_INET, SOCK_STREAM, 0 ) ) )
                break;

            if( setsockopt( ctx.s, SOL_SOCKET, SO_REUSEADDR, 
                            (const char *)&opt, sizeof( opt ) ) < 0 )
                break;

            fcgi_addr_in.sin_family = AF_INET;
            fcgi_addr_in.sin_addr.s_addr = 0x0100007f; // 127.0.0.1
            fcgi_addr_in.sin_port = htons( (unsigned short)ctx.port );

            if( -1 == bind( ctx.s, ( struct sockaddr * )&fcgi_addr_in,
                sizeof( fcgi_addr_in ) ) )
                break;

            if( -1 == listen( ctx.s, SOMAXCONN ) )
                break;
        }

        // close before cgis (msdn: All processes start at shutdown level 0x280)
        if( !SetProcessShutdownParameters( 0x380, SHUTDOWN_NORETRY ) )
            break;

        // php-cgi crash silently if restart delay is >= 1000 ms
        // https://github.com/deemru/php-cgi-spawner/issues/3
        if( ctx.restart_delay >= 1000 )
            SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX );

        InitializeCriticalSection( &ctx.cs );

        ctx.PHP_FCGI_MAX_REQUESTS[0] = 0;
        GetEnvironmentVariableA( "PHP_FCGI_MAX_REQUESTS",
                                 ctx.PHP_FCGI_MAX_REQUESTS,
                                 sizeof( ctx.PHP_FCGI_MAX_REQUESTS ) );

        ctx.PHP_HELP_MAX_REQUESTS[0] = 0;
        GetEnvironmentVariableA( "PHP_HELP_MAX_REQUESTS",
                                 ctx.PHP_HELP_MAX_REQUESTS,
                                 sizeof( ctx.PHP_HELP_MAX_REQUESTS ) );

        memsym( &ctx.si, sizeof( ctx.si ), 0 );
        ctx.si.cb = sizeof( STARTUPINFO );
        ctx.si.dwFlags = STARTF_USESTDHANDLES;
        ctx.si.hStdOutput = INVALID_HANDLE_VALUE;
        ctx.si.hStdError = INVALID_HANDLE_VALUE;
        ctx.si.hStdInput = (HANDLE)ctx.s;

        memsym( ctx.hFCGIs, sizeof( ctx.hFCGIs ), -1 );

        perma_thread( is_helpers );
        break;
    }

    ExitProcess( 0 );
}
