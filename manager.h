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
#include "ultis.h"

#define MAX_USERS 10
#define MAX_TOPICS 20
#define MAX_PERSISTENT 5

#define MAX_NAME_LEN 20
#define MAX_TOPIC_LEN 20
#define MAX_MSG_LEN 300

typedef struct{
    pid_t pid;
    char name[MAX_NAME_LEN + 1];
    char topics[MAX_TOPICS][MAX_TOPIC_LEN + 1];
    int nTopics;
} User;

typedef struct{
    int lifetime;
    char topic[MAX_TOPIC_LEN + 1];
    char name[MAX_NAME_LEN + 1];
    char content[MAX_MSG_LEN + 1];
} PersistentMessage;

typedef struct{
    User *users;
    int *size;
    pthread_mutex_t *mutex;
} Users;

typedef struct{
    PersistentMessage *messages;
    int *size;
    pthread_mutex_t *mutex;
} PersistentList;

typedef struct{
    Users users;
    PersistentList persistent;
} ThreadData;

#endif