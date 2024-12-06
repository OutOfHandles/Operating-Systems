#ifndef TOPIC_H
#define TOPIC_H

#include <pthread.h>
#include "shared.h"
#include <string.h>
#include "user.h"

void showPersistent(Topics *topics, char *topic);
void changeTopicState(Topics *topics, char *name, bool state);
void showTopics(Topics *topics);
int getTopicIndex(Topics *topics, char *topic);
int loadFile(Topics *topics);
void saveToFile(Topics *topics);
int subscribeTopic(Topics *topics, Users *users, char *topicName, pid_t pid);
void addPersistent(Topics *topics, Users *users, Three data, pid_t pid);

#endif