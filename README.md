## php-cgi-spawner

php-cgi-spawner is the smallest and easiest way to spawn a multiple php-cgi processes in Windows for your web server with fastcgi.

- It spawns as many php-cgi on a port as you need.
- It automatically restarts them if they crashed.

## Build

Run [make.bat](src/make.bat) in a Visual Studio environment.

## Notice

Currently a maximum number of php-cgi processes is 64 because of MAXIMUM_WAIT_OBJECTS in WaitForMultipleObjects.

## Example

The [example](example) directory contains [php-cgi-spawner.exe](example/php-cgi-spawner.exe) (precompiled in "Visual Studio 2008 Command Prompt" x86 application to successfully run on Windows XP or higher) and scripts to [run](example/_php-cgi-nginx-restart.bat) and [stop](example/_php-cgi-nginx-stop.bat) your web server.

- VirusTotal: [analysis](https://www.virustotal.com/file/3605ada4fe718484086689a0ef957aa8245d41eecb1999c2f1274610496a98c9/analysis/)
- Graph view: [image](https://raw.githubusercontent.com/deemru/php-cgi-spawner/master/example/php-cgi-spawner.png)
