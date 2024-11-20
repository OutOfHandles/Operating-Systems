#ifndef MANAGER_H
#define MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

#define MAX_NAME_LEN 20
#define MAX_USERS 10
#define MAX_TOPICS 20

typedef struct{
    char name[MAX_NAME_LEN];
    int fd_client_fifo;
} User;

#endif