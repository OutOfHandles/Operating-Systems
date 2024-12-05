#ifndef USER_H
#define USER_H

#include "../ultis.h"
#include "shared.h"
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

int openUserFifo(pid_t pid);
void removeUser(Users users[], pid_t pid);
int getUserIndex(Users *users, pid_t pid);
ManagerStatus addUser(Users *users, char *name, User *newUser);
void handleLogin(Users *users, char *name, pid_t pid);
void handleLogin(Users *users, char *name, pid_t pid);
void showUsers(Users *users);
void kickUser(Users *users, char *name);
void listTopicsUser(Topics *topics, pid_t pid);
int sendServerMessage(int fifo, char *message, pid_t pid);
void unsubscribeTopic(Users *users, char *topicName, pid_t pid);
bool userInTopic(User user, char *topic);
void broadCastMessage(Users *users, char *message, char *topic, pid_t pid);

#endif