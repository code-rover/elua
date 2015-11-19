#!/bin/bash

server : ae.c ae_epoll.c anet.c main.c
	gcc -o $@ $^ -std=gnu99 -llua -lm -ldl



clean :
	rm server $(objects)
