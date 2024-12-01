#include "topic.h"

void showPersistent(Topics *topics, char *topic){
    bool shown = false;
    pthread_mutex_lock(topics->mutex);
    for(int i = 0; i < topics->nTopics; i++){
        if(strcmp(topic, topics->topicsList[i].name) == 0){ //|| strcmp(topic, "*") == 0
            PersistentMessage *persistentList = topics->topicsList[i].persistentList;

            for(int j = 0; j < topics->topicsList[i].nPersistent; j++){
                printf("[%s] : <%s> : %d : %s\n", persistentList[j].name, persistentList[j].topic, persistentList[j].lifetime, persistentList[j].content); //persistentList[j], persistentList->topic, messages[i].lifetime, messages[i].content
                shown = true;
            }
        }
    }
    pthread_mutex_unlock(topics->mutex);

    if(!shown) printf("<MANAGER>: No messsages available at the moment\n");
}

void changeTopicState(Topics *topics, char *name, bool state){
    bool changed = false;
    pthread_mutex_lock(topics->mutex);
    for(int i = 0; i < topics->nTopics; i++){
        if(strcmp(topics->topicsList[i].name, name) == 0){
            if(topics->topicsList[i].locked == state){ 
                printf("<MANAGER>: Topic already %s\n", state ? "locked" : "unlocked");
                pthread_mutex_unlock(topics->mutex);            
                return;
            }
            topics->topicsList[i].locked = state;
            changed = true;
        }
    }
    pthread_mutex_unlock(topics->mutex);
    if(!changed) 
        printf("<MANAGER>: Invalid topic name\n");
    else
        printf("<MANAGER>: Topic successfully %s\n", state ? "locked" : "unlocked");
}

void showTopics(Topics *topics){
    pthread_mutex_lock(topics->mutex);
    if(topics->nTopics == 0){
        printf("<MANAGER>: No topics at the moment\n");
    }
    for(int i = 0; i < topics->nTopics; i++){
        printf("<MANAGER>: %s [%s]\n", topics->topicsList[i].name, topics->topicsList[i].locked ? "LOCKED" : "UNLOCKED");
    }
    pthread_mutex_unlock(topics->mutex);
}

int getTopicIndex(Topics *topics, char *topic){
    for(int i = 0; i < topics->nTopics; i++){
        if(strcmp(topics->topicsList[i].name, topic) == 0) return i;
    }

    return -1;
}

int loadFile(Topics *topics){
    FILE *f = fopen("messages.txt", "r"); //change to read from env

    if(!f) return -1;
    PersistentMessage pm;

    while(fscanf(f, "%s %s %d %[^\n]s", pm.topic, pm.name, &pm.lifetime, pm.content) != EOF){
        int index = getTopicIndex(topics, pm.topic);

        if(index != -1){
            Topic *topic = &topics->topicsList[index];
            if(topic->nPersistent < MAX_PERSISTENT)
                topic->persistentList[topic->nPersistent++] = pm;
        }
        else if(topics->nTopics < MAX_TOPICS){
            Topic *topic = &topics->topicsList[topics->nTopics++];
            *topic = (Topic){.nPersistent = 0, .locked = false};
            strcpy(topic->name, pm.topic);
            topic->persistentList[topic->nPersistent++] = pm;
        }
    }

    fclose(f);
    return 0;
}


void saveToFile(Topics *topics){
    FILE *f = fopen("messages.txt", "w"); //change to read from env

    if(!f){
        printf("Failed to open file\n");
        return;
    }
    
    for(int i = 0; i < topics->nTopics; i++){
        PersistentMessage *persistentList = topics->topicsList[i].persistentList;
        for(int j = 0; j < topics->topicsList[i].nPersistent; j++){
            fprintf(f, "%s %s %d %s\n", persistentList[j].topic, persistentList[j].name, persistentList[j].lifetime, persistentList[j].content);
        }
    }

    fclose(f);
}