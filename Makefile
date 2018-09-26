#
# Makefile fitxategia
# RadioCloud daemon
#

CC = gcc 
INCLUDES = $(shell mysql_config --libs --cflags) $(shell xml2-config --cflags) $(shell xml2-config --libs) $(shell pkg-config --cflags --libs gstreamer-1.0) -lcurl -lm -I.

default: rc-daemon

rc-daemon: src/main.c src/daemon.c src/config.c src/database.c src/downloader.c src/encoder.c src/uploader.c src/utils.c
	$(CC) -o rc-daemon src/main.c src/daemon.c src/config.c src/database.c src/downloader.c src/encoder.c src/uploader.c src/utils.c $(INCLUDES)


