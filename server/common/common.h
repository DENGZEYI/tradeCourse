#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>		// getaddrinfo()
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <mysql/mysql.h>

//containing some common used functions.

/**
 * Create listening socket on remote host
 * Returns -1 on error, socket fd on success
 */
int socket_create(int port);

/**
 * Create new socket for incoming client connection request
 * Returns -1 on error, or fd of newly created socket
 */
int socket_accept(int sock_listen);

/**
 * Trim whiteshpace and line ending
 * characters from a string
 */
void trimstr(char *str, int n);

void addsig( int sig, void( handler )(int), bool restart = true );

// add fd to epoll
void addfd( int epollfd, int fd, bool one_shot );

//set fd to nonblocking
int setnonblocking( int fd );

//从epollfd中移除fd
void removefd( int epollfd, int fd );

#endif