#ifndef TOPIC_H
#define TOPIC_H

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include "shared.h"
#include <string.h>

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

void showPersistent(Topics *topics, char *topic);
void changeTopicState(Topics *topics, char *name, bool state);
void showTopics(Topics *topics);
int getTopicIndex(Topics *topics, char *topic);
int loadFile(Topics *topics);
void saveToFile(Topics *topics);

#endif