## php-cgi-spawner

Introducing the smallest and easiest way to spawn a multiple php-cgi in Windows for your nginx with fastcgi — php-cgi-spawner:

1) It spawns as many php-cgi on a chosen port as you need (change MAX_SPAWN_HANDLES if you need more than 256).
2) It automatically restarts them if they crashed by any reason.

## Build

Run [make.bat](src/make.bat) in a Visual Studio environment.

## Example

The [example](example) directory contains compiled in Visual Studio 2008 [php-cgi-spawner.exe](example/php-cgi-spawner.exe) and scripts to run and stop your web server.
