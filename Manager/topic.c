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

void subscribeTopic(Topics *topics, Users *users, char *topicName, pid_t pid){
    pthread_mutex_lock(topics->mutex);
    pthread_mutex_lock(users->mutex);
    int index = getTopicIndex(topics, topicName);
    int fifo = openUserFifo(pid);
    
    if(fifo == -1){
        pthread_mutex_unlock(topics->mutex);    
        return;
    }

    if(index == -1){
        if(topics->nTopics < MAX_TOPICS){
            int uIndex = getUserIndex(users, pid);
            if(uIndex == -1){
                pthread_mutex_unlock(users->mutex);   
                pthread_mutex_unlock(topics->mutex);
                printf("<MANAGER> Failed to find user\n");
                kill(pid, SIGUSR1);

                return;
            }
            strcpy(users->userList[uIndex].topics[users->userList[uIndex].nTopics++].name, topicName);
            strcpy(topics->topicsList[topics->nTopics++].name, topicName);
            topics->topicsList[topics->nTopics].nPersistent = 0;
            
            if(sendServerMessage(fifo, "<MANAGER> Success", pid) <= 0)
                printf("<MANAGER> Failed to send message to user\n");
        }
        else{
            if(sendServerMessage(fifo, "<MANAGER> Maximum amount of topics reached", pid) <= 0)
                printf("<MANAGER> Failed to send message to user\n");    
        }
    }
    else{
        int uIndex = getUserIndex(users, pid);
        if(uIndex == -1){
            pthread_mutex_unlock(users->mutex);   
            pthread_mutex_unlock(topics->mutex);
            printf("<MANAGER> Failed to find user\n");
            kill(pid, SIGUSR1);

            return;
        }

        strcpy(users->userList[uIndex].topics[users->userList[uIndex].nTopics++].name, topicName);
        for(int i = 0; i < topics->topicsList[index].nPersistent; i++){
            ServerMessage msg;
            sprintf(msg.message, "<%s> %s: %s", topics->topicsList[index].persistentList[i].name, 
                    topics->topicsList[index].name, topics->topicsList[index].persistentList[i].content);

            if(sendServerMessage(fifo, msg.message, pid) <= 0){
                printf("<MANAGER> Failed to send message to user\n");
            }
        }
    }
    pthread_mutex_unlock(users->mutex);
    pthread_mutex_unlock(topics->mutex);
}

void addPersistent(Topics *topics, Users *users, Three data, pid_t pid){
    pthread_mutex_lock(users->mutex);

    int index = getTopicIndex(topics, data.topic);
    int uIndex = getUserIndex(users, pid);

    if(uIndex == -1){
        kill(pid, SIGUSR1);
        printf("<MANAGER> Failed to find a user\n");
        return;
    }
    int fifo = openUserFifo(pid);

    if(fifo == -1){
        pthread_mutex_unlock(users->mutex);   
        pthread_mutex_unlock(topics->mutex); 
        kill(pid, SIGUSR1);
        printf("Failed to open user fifo\n");

        return;
    }
    if(topics->topicsList[index].nPersistent >= MAX_PERSISTENT){
        pthread_mutex_unlock(users->mutex);   
        pthread_mutex_unlock(topics->mutex);    
        sendServerMessage(fifo, "<MANAGER> Max amount of persistent messages on current topic", pid);
        return;    
    }
    PersistentMessage pm = {.lifetime = data.lifetime};
    strcpy(pm.name, users->userList[uIndex].name);
    strcpy(pm.content, data.message);
    strcpy(pm.topic, data.topic);

    topics->topicsList[index].persistentList[topics->topicsList[index].nPersistent++] = pm;

    pthread_mutex_unlock(users->mutex);   
}