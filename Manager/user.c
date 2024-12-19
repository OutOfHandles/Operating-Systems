#include "user.h"

int openUserFifo(pid_t pid){
    char fifoName[50];
    sprintf(fifoName, FEED_FIFO, pid);
    int fifo = open(fifoName, O_WRONLY);

    return fifo;
}

void removeUser(Users users[], pid_t pid){
    int newSize = 0;

    for(int i = 0; i < users->size; i++){
        if(users->userList[i].pid != pid)
            users->userList[newSize++] = users->userList[i];
    }
    users->size = newSize;
}

void sendSignal(pid_t pid, ClosingType t){
    union sigval val = {.sival_int = t};
    sigqueue(pid, SIGUSR2, val);
}

int getUserIndex(Users *users, pid_t pid){
    for(int i = 0; i < users->size; i++){
        if(users->userList[i].pid == pid) return i;
    }
    return -1;
}

ManagerStatus addUser(Users *users, char *name, User *newUser){
    if(users->size >= MAX_USERS){
        return MANAGER_FULL;
    }
    for(int i = 0; i < users->size; i++){
        if(strcmp(name, users->userList[i].name) == 0) return USERNAME_IN_USE;
    }
    ManagerStatus ret = SUCCESS;

    snprintf(newUser->name, MAX_NAME_LEN + 1, "%s", name);

    if(strlen(name) >= MAX_NAME_LEN)
        ret = TRUNCATED;

    users->userList[users->size++] = *newUser;
    return ret;
}

void handleLogin(Users *users, char *name, pid_t pid){
    int fifo = openUserFifo(pid);
    
    pthread_mutex_lock(users->mutex);
    if(fifo == -1){
        removeUser(users, pid);
        pthread_mutex_unlock(users->mutex);
        sendSignal(pid, MANAGER_ERROR);
        printf("Failed to open feed fifo\n");
        return;
    }

    User newUser = {.pid = pid, .nTopics = 0};

    ManagerStatus status = addUser(users , name, &newUser);
    pthread_mutex_unlock(users->mutex);

    int written = write(fifo, &status, sizeof(ManagerStatus));
    
    if(written <= 0){
        pthread_mutex_lock(users->mutex);
        removeUser(users, pid);
        pthread_mutex_unlock(users->mutex);
        sendSignal(pid, MANAGER_ERROR);
        printf("Failed to send response to user\n");
    }

    close(fifo);
}

void showUsers(Users *users){
    pthread_mutex_lock(users->mutex);
    Users copy = *users;
    pthread_mutex_unlock(users->mutex);

    if(copy.size == 0){
        printf("<MANAGER>: No users at the moment\n");
    }
    for(int i = 0; i < copy.size; i++){
        printf("<MANAGER>: [%d] -> %s\n", copy.userList[i].pid, copy.userList[i].name);
    }
}

int sendServerMessage(int fifo, char *message, pid_t pid){
    Headers h = {.pid = pid, .size = sizeof(ServerMessage), .type = MESSAGE};
    ServerMessage msg;
    strcpy(msg.message, message);
    int size = 0;
    char bytes[MAX_BYTES];
    toBytes(bytes, &size, &h, sizeof(Headers));
    toBytes(bytes, &size, &msg, sizeof(ServerMessage));
    
    int written = write(fifo, bytes, size);

    return written;
}

void kickUser(Users *users, char *name){
    pthread_mutex_lock(users->mutex);
    User *user = NULL;
    for(int i = 0; i < users->size; i++){
        if(strcmp(name, users->userList[i].name) == 0){
            user = &users->userList[i];
            sendSignal(user->pid, KICK);
            break;
        }
    }
    if(!user){
        pthread_mutex_unlock(users->mutex);
        printf("<MANAGER>: Unable to find target user\n");
        return;
    }

    char buffer[40];
    sprintf(buffer, "%s %s", user->name, " has been kicked\n");
    
    pthread_mutex_unlock(users->mutex);
    removeUser(users, user->pid);
    pthread_mutex_lock(users->mutex);

    for(int i = 0; i < users->size; i++){
        int fifo = openUserFifo(users->userList[i].pid);
        if(fifo == -1){
            removeUser(users, users->userList[i].pid);
            sendSignal(users->userList[i].pid, MANAGER_ERROR);
            continue;
        }
        
        if(sendServerMessage(fifo, buffer, users->userList[i].pid) <= 0){
            printf("Failed to send kick message to user\n");
        }

        close(fifo);
    }
    pthread_mutex_unlock(users->mutex);
}

void listTopicsUser(Topics *topics, pid_t pid){
    pthread_mutex_lock(topics->mutex);
    int fifo = openUserFifo(pid);

    if(fifo == -1){
        pthread_mutex_unlock(topics->mutex);
        printf("Failed to open fifo\n");
        return;
    }

    if(topics->nTopics == 0){
        if(sendServerMessage(fifo, "<MANAGER> No topics available at the moment", pid) <= 0){
            printf("<MANAGER> Failed to send message to user\n");
        }
    }
    for(int i = 0; i < topics->nTopics; i++){
        ServerMessage msg;

        sprintf(msg.message, "<MANAGER> %s\t[%s]\tPersistent messages: %d", topics->topicsList[i].name, 
                topics->topicsList[i].locked ? "LOCKED" : "UNLOCKED", topics->topicsList[i].nPersistent);

        if(sendServerMessage(fifo, msg.message, pid) <= 0){
            printf("<MANAGER> Failed to send message to user\n");
        }
    }
    close(fifo);
    pthread_mutex_unlock(topics->mutex);
}


void unsubscribeTopic(Users *users, char *topicName, pid_t pid){
    pthread_mutex_lock(users->mutex);
    int newSize = 0;
    int index = getUserIndex(users, pid);

    if(index == -1){
        removeUser(users, pid);
        pthread_mutex_unlock(users->mutex);
        sendSignal(pid, MANAGER_ERROR);
        printf("Failed to find user\n");
        return;
    }

    for(int i = 0; i < users->userList[index].nTopics; i++){
        if(strcmp(users->userList[index].topics[i].name, topicName) != 0)
            users->userList[index].topics[newSize++] = users->userList[index].topics[i];
        
    }

    users->userList[index].nTopics--;
    
    int fifo = openUserFifo(pid);
    if(fifo == -1){
        printf("<MANAGER> Failed to open user fifo\n");
        pthread_mutex_unlock(users->mutex);

        return;
    }

    sendServerMessage(fifo, "<MANAGER> Success", pid);
    close(fifo);

    pthread_mutex_unlock(users->mutex);
}

bool userInTopic(User user, char *topic){
    for(int i = 0; i < user.nTopics; i++){
        if(strcmp(user.topics[i].name, topic) == 0) return true;
    }

    return false;
}

void broadCastMessage(Users *users, char *message, char *topic, pid_t pid){
    int index = getUserIndex(users, pid);
    
    if(index == -1){
        removeUser(users, pid);
        sendSignal(pid, MANAGER_ERROR);
        return;
    }

    ServerMessage sm;
    sprintf(sm.message, "[%s] <%s>: %s", users->userList[index].name, topic, message);
    for(int i = 0; i < users->size; i++){
        if(userInTopic(users->userList[i], topic)){
            int fifo = openUserFifo(users->userList[i].pid);

            if(fifo == -1){
                printf("<MANAGER> Failed to open user fifo\n");
                continue;
            }
            if(sendServerMessage(fifo, sm.message, pid) <= 0){
                printf("<MANAGER> Failed to broacast message to a user\n");
            }
            close(fifo);
        }
    }
}