start _php-cgi-nginx-stop.bat
ping -n 2 127.0.0.1
start php-cgi-spawner php\php-cgi 9000 8
cd nginx
start nginx
exit
