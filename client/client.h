#ifndef FTCLIENT_H
#define FTCLIENT_H

#define MAXBUFSIZE 2048

#include <netdb.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

struct message{
    char request_line[MAXBUFSIZE];
    char request_header[MAXBUFSIZE];
};

/**
 * Get login details from user and
 * send to server for authentication
 */
void ftclient_login();

/**
 * 将message结构体构中的内容构造为http格式发送给客户端
 * return true on success, false on fail
 */
bool send_message(struct message *request_message);

/**
 * Parse command in cstruct
 * return true on success, false on fail
 */ 
bool ftclient_read_command(char* buf, int size, struct message *request_message);

/**
 * read reply from server
 * only recv() once 
 */
bool read_result(char *buf,int size);

/**
 * Parse command in cstruct
 * return true on success, false on fail
 */ 
bool ftclient_read_command(char* buf, int size, struct message *request_message);

/**
 * Get login details from user and
 * send to server for authentication
 */
void ftclient_login();

/** 
 * Read input from command line
 */
void read_input(char* buffer, int size);

/**
 * Receive a response from server
 * Returns -1 on error, return code on success
 */
int read_reply();

#endif