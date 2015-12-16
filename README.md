# php-cgi-spawner
[![Build status](https://ci.appveyor.com/api/projects/status/6f2rqvltmp9ax4nd?svg=true)](https://ci.appveyor.com/project/deemru/php-cgi-spawner)

[php-cgi-spawner](https://github.com/deemru/php-cgi-spawner) is the smallest and easiest application to spawn a multiple php-cgi processes in Windows for your web server with FastCGI.

- It spawns as many php-cgi (x86 or x64) on a single port as you need.
- It automatically restarts them if they crashed.

## Usage

If you have the following directory structure:

```
php-cgi-spawner.exe
php
  php-cgi.exe
  php.ini
```

In order to spawn 4 php-cgi on tcp/9000:

```
php-cgi-spawner.exe php/php-cgi.exe 9000 4
```

You may specify a config file explicitly:

```
php-cgi-spawner.exe "php/php-cgi.exe -c php/php.ini" 9000 4
```

`php-cgi.exe` are spawned under the same user that runs `php-cgi-spawner.exe`.

## Download

Go to [release](https://github.com/deemru/php-cgi-spawner/releases/latest) to download pre-built binary.

## Build

Go to [src](src) directory and run [make.bat](src/make.bat) in a Visual Studio environment.

## Notes

- Currently a maximum number of php-cgi processes is 64 because of `MAXIMUM_WAIT_OBJECTS` in `WaitForMultipleObjects`.
