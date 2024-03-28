# LEXISERVER
A Simple Webserver written in C with simple SSL support for Educational Purposes.

## INSTALL
run (as sudo/root) install.sh (e.g: sudo bash install.sh) this will create /opt/lexiserver/ and move
all the files there.

### Raspberri Pi Users

run lexiserver-arm64 instead


## UNINSTALL
remove the /opt/lexiserver directory (e.g: sudo rm -rf /opt/lexiserver/)

## STARTING AND SERVING DOCUMENTS:

The important files are:
- lexiserver (the server binary)
- lexiserver.conf (the config file for the server)

Before starting the server, edit the config to change the WEB_ROOT to a directory that you like.
By default, it looks for html documents inside /tmp/www/. You may need to create this directory
if you don't have it (you likely don't have it since most webservers use /var/www/html/)

To start the server, run ./lexiserver

If the client doesn't request any page, ej you hit localhost:port
it will serve index.html in /tmp/www or whatever directory you decided to use for your experiment.

Stopping the server:

You can kill the PID (Process ID) to end serving documents
```ps aux | grep lexiserver``` ```kill <pid number>```

## WARNING
This is a webserver for educational purposes. Do not expose this to the internet unless you know
exactly what you're doing. 

## TO DO:
- More Error Pages (only handles 404 at the moment)
- start, stop, restart arguments

## Compile your own version

Inside the src directory you will find the current version of lexiserver.c
compile with gcc: ```gcc lexiserver.c -o lexiserver```
move the compiled version to /opt/lexiserver

The binary has to be alongside lexiserver.conf in order to work properly. If the config
is not present, it will assume defaults anyway.

## OTHER NOTES
This is a recreational project. Just for fun. Not intended to be serious or to be ran in production.
Bugs will be there, things will not make much sense. Feel free to fork the project, fix whatever
you think you can fix, and send me a merge request.

To fork the project:

Fork https://github.com/alexiarstein/simple-webserver

## Documentation and info

[alexia.lat/docs/lexiserver](https://alexia.lat/docs/lexiserver)

I hope you enjoy this little project and if you have any doubts you can write me on discord at
https://lexi.lat > mis links > discord

Goodbye!
