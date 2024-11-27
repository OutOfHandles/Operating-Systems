#ifndef UTILS_H
#define UTILS_H

#define MANAGER_FIFO "MANAGER_FIFO"
#define FEED_FIFO "FEED_FIFO_%d"

#include <pthread.h>

typedef enum { LOGIN, LOGOUT, KICK, COMMAND, CLOSING, RESPONSE } Type;
typedef enum { MANAGER_FULL, SUCCESS, TRUNCATED, USERNAME_IN_USE } ManagerStatus;

typedef struct{
    Type type;
    pid_t pid;
    int size;
} Headers;

#endif