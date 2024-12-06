#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include "ultis.h"

void toBytes(char *dest, int *totalSize, void *data, int dataSize){
    memcpy(dest + *totalSize, data, dataSize);
    *totalSize += dataSize;
}

typedef struct{
    char *name;
    int args;
} Command;

Command feedCommands[] = {{.name = "exit", .args = 0}, {.name = "topics", .args = 0}, {.name = "msg", .args = 3},
    {.name = "subscribe", .args = 1}, {.name = "unsubscribe", .args = 1}};

Command managerCommands[] = {{.name = "users", .args = 0}, {.name = "remove", .args = 1}, {.name = "topics", .args = 0},
    {.name = "show", .args = 1}, {.name = "lock", .args = 1}, {.name = "unlock", .args = 1}, {.name = "close", .args = 0}};

void printError(CommandValidation v){
    switch(v){
        case INVALID_COMMAND:{
            printf("INVALID COMMAND\n");
            break;
        }
        case MISSING_ARGUMENTS:{
            printf("MISSING ARGUMENTS\n");
            break;
        }
        case ERROR:{
            printf("AN ERROR HAD OCCURRED\n");
            break;
        }
        case INVALID_ARGUMENTS: {
            printf("INVALID ARGUMENTS\n");
            break;
        }
        case CHANGED_ARGUMENTS:{
            printf("Truncated arguments\n");
            break;
        }
        default:{
            break;
        }
    }
}

void getCommandName(char *command, char *name){
    char temp[BUFFER_SIZE];
    strcpy(temp, command);
    char *token = strtok(temp, " ");
    strcpy(name, token);
}

void getArg(char *command, char *arg){
    // int i;
    char temp[BUFFER_SIZE];
    strcpy(temp, command);
    strtok(temp, " ");
    char *token = strtok(NULL, " ");
    strcpy(arg, token);

    snprintf(arg, MAX_TOPIC_LEN + 1, "%s", token);
    // for(i = 0; i < strlen(token) && i < MAX_TOPIC_LEN && i < BUFFER_SIZE - 1; i++){
    //     arg[i] = token[i];
    // }

    // arg[i] = '\0';
}

void getLifeTime(char *command, int *lifetime){
    char temp[BUFFER_SIZE];
    strcpy(temp, command);
    strtok(temp, " ");
    char *token = strtok(NULL, " ");
    token = strtok(NULL, " ");

    *lifetime = atoi(token);
}

void getMessage(char *command, char *message){
    // int i;
    char temp[BUFFER_SIZE];
    strcpy(temp, command);
    strtok(temp, " ");
    char *token = strtok(NULL, " ");
    token = strtok(NULL, " ");
    token = strtok(NULL, "");

    snprintf(message, MAX_MSG_LEN + 1, "%s", token);
    // for(i = 0; i < strlen(token) && i < MAX_MSG_LEN; i++){
    //     message[i] = token[i];
    // }
    // message[i] = '\0';
}

CommandValidation validateCommand(char *command, int *nArgs, CommandType t){
    char temp[BUFFER_SIZE];
    int size = t;
    Command *commandList = t == FEED_COMMAND ? feedCommands : managerCommands;

    strcpy(temp, command);
    char *token = strtok(temp, " ");

    if(!token) return INVALID_COMMAND;
    
    int index = -1;
    for(int i = 0; i < size; i++){
        if(strcmp(token, commandList[i].name) == 0){
            index = i;
            break;
        }
    }
    
    if(index == -1)
        return INVALID_COMMAND;
    
    int counter = 0;
    while(strtok(NULL, " ") && counter < commandList[index].args){
        counter++;
    }

    if(counter < commandList[index].args) return MISSING_ARGUMENTS;
    *nArgs = counter;

    if(counter == 0) return VALID;

    bool modify = false;
    strcpy(temp, command);

    token = strtok(temp, " ");
    if(!token) return ERROR;
    token = strtok(NULL, " ");
    if(!token) return ERROR;

    if(strlen(token) > MAX_TOPIC_LEN) modify = true;
    if(counter == 1) return modify ? CHANGED_ARGUMENTS : VALID;

    token = strtok(NULL, " ");
    if(!token) return ERROR;

    for(int i = 0; i < strlen(token); i++){
        if(!isdigit(token[i])) return INVALID_ARGUMENTS;
    }

    token = strtok(NULL, "");
    if(!token) return ERROR; 
    if(strlen(token) > MAX_MSG_LEN) modify = true;
    
    return modify ? CHANGED_ARGUMENTS : VALID;
}