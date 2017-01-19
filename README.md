# php-cgi-spawner
[![Build status](https://ci.appveyor.com/api/projects/status/6f2rqvltmp9ax4nd?svg=true)](https://ci.appveyor.com/project/deemru/php-cgi-spawner)

[php-cgi-spawner](https://github.com/deemru/php-cgi-spawner) is the smallest and easiest application to spawn a multiple `php-cgi` processes in Windows for your web server with FastCGI.

- It spawns as many `php-cgi` (x86 or x64) on a single port as you need
- It automatically restarts them if they crashed or reached `PHP_FCGI_MAX_REQUESTS`

## Basic Usage

If you have the following directory structure:

```
php-cgi-spawner.exe
php
  php-cgi.exe
  php.ini
```

In order to spawn 4 `php-cgi` on tcp/9000:

```
php-cgi-spawner.exe php/php-cgi.exe 9000 4
```

You may specify a config file explicitly:

```
php-cgi-spawner.exe "php/php-cgi.exe -c php/php.ini" 9000 4
```

`php-cgi.exe` processes are spawned under the same user that runs `php-cgi-spawner.exe`.

## Version 1.1

Now you can additionally specify a number of `php-cgi` helpers for permanently running `php-cgi` processes:

```
php-cgi-spawner.exe "php/php-cgi.exe -c php/php.ini" 9000 4+16
```

It means that up to 16 `php-cgi` helpers can start automatically when 4 `php-cgi` permanents do not handle a temporary server high load.

`PHP_FCGI_MAX_REQUESTS` for `php-cgi` helpers can be set in `PHP_HELP_MAX_REQUESTS`:

```
set PHP_HELP_MAX_REQUESTS 100
php-cgi-spawner.exe "php/php-cgi.exe -c php/php.ini" 9000 4+16
```

You can even do not use permanently running `php-cgi` and use just helpers, even with 1 request per helper:

```
set PHP_HELP_MAX_REQUESTS 1
php-cgi-spawner.exe "php/php-cgi.exe -c php/php.ini" 9000 0+16
```

But it is recommended to run at least 1 permanent `php-cgi` to handle opcache.

## Download

Go to [release](https://github.com/deemru/php-cgi-spawner/releases/latest) to download pre-built binary.

## Build

Go to [src](src) directory and run [make.bat](src/make.bat) in a Visual Studio environment.

## Notes

- Currently a maximum number of permanently running `php-cgi` processes is 64 because of `MAXIMUM_WAIT_OBJECTS` in `WaitForMultipleObjects`.
- The number of `php-cgi` helpers does not have such limit.
