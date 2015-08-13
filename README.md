## php-cgi-spawner

Introducing the smallest and easiest way to spawn a multiple php-cgi in Windows for your nginx with fastcgi.

- It spawns as many php-cgi on a chosen port as you need.
- It automatically restarts them if they crashed by any reason.

## Build

Run [make.bat](src/make.bat) in a Visual Studio environment.

## Example

The [example](example) directory contains [php-cgi-spawner.exe](example/php-cgi-spawner.exe) (compiled in Visual Studio 2008 for successfully run on Windows XP) and scripts to [run](example/_php-cgi-nginx-restart.bat) and [stop](example/_php-cgi-nginx-stop.bat) your web server.
