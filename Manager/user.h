#ifndef USER_H
#define USER_H

#include "../ultis.h"
#include "shared.h"
#include <fcntl.h>
#include <signal.h>
#include "topic.h"
#include <string.h>
#include <unistd.h>

#define MAX_USERS 10

typedef struct{
    pid_t pid;
    char name[MAX_NAME_LEN + 1];
    Topic topics[MAX_TOPICS];
    int nTopics;
} User;

typedef struct{
    User userList[MAX_USERS];
    int size;
    pthread_mutex_t *mutex;
} Users;

int openUserFifo(pid_t pid);
void removeUser(Users users[], pid_t pid);
ManagerStatus addUser(Users *users, char *name, User *newUser);
void handleLogin(Users *users, char *name, pid_t pid);
void handleLogin(Users *users, char *name, pid_t pid);
void showUsers(Users *users);
void kickUser(Users *users, char *name);

#endif