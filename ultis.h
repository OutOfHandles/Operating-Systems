#ifndef UTILS_H
#define UTILS_H

#define MANAGER_FIFO "Pipes/MANAGER_FIFO"
#define FEED_FIFO "Pipes/FEED_FIFO_%d"

#include <pthread.h>

typedef enum { LOGIN, LOGOUT, COMMAND, MESSAGE, ZERO, ONE, THREE } Type;
typedef enum { MANAGER_FULL, SUCCESS, TRUNCATED, USERNAME_IN_USE } ManagerStatus;
typedef enum { KICK, MANAGER_ERROR } ClosingType;
typedef enum { INVALID_COMMAND, MISSING_ARGUMENTS, ERROR, CHANGED_ARGUMENTS, INVALID_ARGUMENTS, VALID } CommandValidation;
typedef enum { MANAGER_COMMAND  = 7, FEED_COMMAND = 5 } CommandType;

#define MAX_NAME_LEN 20
#define MAX_MSG_LEN 300
#define MAX_TOPIC_LEN 20
#define MAX_COMMAND_LEN 15

#define BUFFER_SIZE 400
#define MAX_BYTES sizeof(Headers) + sizeof(Three) + sizeof(ServerMessage)

typedef struct{
    Type type;
    pid_t pid;
    int size;
} Headers;

typedef struct{
    char message[MAX_NAME_LEN + MAX_TOPIC_LEN + MAX_MSG_LEN + 15];
} ServerMessage;

typedef struct{
    char author[MAX_NAME_LEN + 1];
    char topic[MAX_TOPIC_LEN + 1];
    char content[MAX_MSG_LEN + 1];
} Message;

typedef struct{
    char command[MAX_COMMAND_LEN + 1];
} Zero;

typedef struct{
    char command[MAX_COMMAND_LEN + 1];
    char arg[MAX_TOPIC_LEN + 1];
} One;

typedef struct{
    char topic[MAX_TOPIC_LEN + 1];
    int lifetime;
    char message[MAX_MSG_LEN + 1];
} Three;

void toBytes(char *dest, int *totalSize, void *data, int dataSize);
void getCommandName(char *command, char *name);
void getArg(char *command, char *arg);
void getLifeTime(char *command, int *lifetime);
void getMessage(char *command, char *message);
CommandValidation validateCommand(char *command, int *nArgs, CommandType t);
void printError(CommandValidation v);

#endif