cl /c /Ox /Os /GL /GF /GS- /Wall php-cgi-spawner.c
rc -r php-cgi-spawner.rc
link /LTCG php-cgi-spawner.obj php-cgi-spawner.res /subsystem:windows /MERGE:.rdata=.text