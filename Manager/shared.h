#ifndef SHARED_H
#define SHARED_H

#include <stdbool.h>
#include <stdio.h>

#define MAX_NAME_LEN 20
#define MAX_TOPICS 20
#define MAX_USERS 10

#define MAX_TOPIC_LEN 20
#define MAX_PERSISTENT 5

#define MAX_MSG_LEN 300

typedef struct{
    int lifetime;
    char topic[MAX_TOPIC_LEN + 1];
    char name[MAX_NAME_LEN + 1];
    char content[MAX_MSG_LEN + 1];
} PersistentMessage;

typedef struct{
    char name[MAX_TOPIC_LEN + 1];
    PersistentMessage persistentList[MAX_PERSISTENT];
    int nPersistent;
    bool locked;
} Topic;

typedef struct{
    Topic topicsList[MAX_TOPICS];
    int nTopics;
    bool *running;
    pthread_mutex_t *mutex;
} Topics;

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


#endif