# php-cgi-spawner
[![Build status](https://ci.appveyor.com/api/projects/status/6f2rqvltmp9ax4nd?svg=true)](https://ci.appveyor.com/project/deemru/php-cgi-spawner)

[php-cgi-spawner](https://github.com/deemru/php-cgi-spawner) is the smallest and easiest application to spawn a multiple php-cgi processes in Windows for your web server with FastCGI.

- It spawns as many php-cgi on a signle port as you need.
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
cd php
../php-cgi-spawner.exe php-cgi.exe 9000 4
cd ..
```

It's important to be in `php-cgi.exe` directory else it won't pick up `php.ini`.

As an alternative you may specify config file explicitly:

```
php-cgi-spawner.exe "php/php-cgi.exe -c php/php.ini" 9000 4
```

`php-cgi` are spawned under the same user that runs `php-cgi-spawner.exe`.

## Download

Go to [release](https://github.com/deemru/php-cgi-spawner/releases/latest) to download pre-built binary.

## Build

Go to [src](src) directory and run [make.bat](src/make.bat) in a Visual Studio environment.

## Notice

Currently a maximum number of php-cgi processes is 64 because of `MAXIMUM_WAIT_OBJECTS` in `WaitForMultipleObjects`.
