#include "user.h"

int openUserFifo(pid_t pid){
    char fifoName[25];
    sprintf(fifoName, FEED_FIFO, pid);
    int fifo = open(fifoName, O_WRONLY);

    return fifo;
}

void removeUser(Users users[], pid_t pid){
    int newSize = 0;

    pthread_mutex_lock(users->mutex);
    for(int i = 0; i < users->size; i++){
        if(users->userList[i].pid != pid)
            users->userList[newSize++] = users->userList[i];
    }
    users->size = newSize;
    pthread_mutex_unlock(users->mutex);
}

ManagerStatus addUser(Users *users, char *name, User *newUser){
    if(users->size >= MAX_USERS){
        return MANAGER_FULL;
    }
    for(int i = 0; i < users->size; i++){
        if(strcmp(name, users->userList[i].name) == 0) return USERNAME_IN_USE;
    }
    ManagerStatus ret = SUCCESS;

    snprintf(newUser->name, MAX_NAME_LEN, "%s", name); //acts like strcpy_s

    if(strlen(name) >= MAX_NAME_LEN)
        ret = TRUNCATED;

    users->userList[users->size++] = *newUser;
    return ret;
}

void handleLogin(Users *users, char *name, pid_t pid){
    int fifo = openUserFifo(pid);
    
    if(fifo == -1){
        printf("Failed to open feed fifo\n");
        kill(pid, SIGUSR1); //USR1 -> internal server error
        return;
    }

    User newUser = {.pid = pid, .nTopics = 0};

    pthread_mutex_lock(users->mutex);
    ManagerStatus status = addUser(users , name, &newUser);
    pthread_mutex_unlock(users->mutex);

    int written = write(fifo, &status, sizeof(ManagerStatus));
    
    if(written <= 0){
        kill(pid, SIGUSR1);
        printf("Failed to send response to user\n");
    }

    close(fifo);
}

void showUsers(Users *users){
    pthread_mutex_lock(users->mutex);
    if(users->size == 0){
        printf("<MANAGER>: No users at the moment\n");
    }
    for(int i = 0; i < users->size; i++){
        printf("<MANAGER>: [%d] -> %s\n", users->userList[i].pid, users->userList[i].name);
    }
    pthread_mutex_unlock(users->mutex);
}

void kickUser(Users *users, char *name){
    pthread_mutex_lock(users->mutex);
    User *user = NULL;
    for(int i = 0; i < users->size; i++){
        if(strcmp(name, users->userList[i].name) == 0){
            user = &users->userList[i];
            kill(SIGUSR2, user->pid);
            break;
        }
    }
    if(!user){
        printf("<MANAGER>: Unable to find target user\n");
        pthread_mutex_unlock(users->mutex);
        return;
    }

    Headers h = {.pid = user->pid, .size = 0, .type = MESSAGE};
    char buffer[40];
    sprintf(buffer, "%s %s", user->name, " has been kicked\n");
    h.size = strlen(buffer) + 1;
    
    pthread_mutex_unlock(users->mutex);
    removeUser(users, user->pid);
    pthread_mutex_lock(users->mutex);

    char message[MAX_BYTES];
    int size = 0;
    toBytes(message, &size, &h, sizeof(Headers));
    toBytes(message, &size, buffer, strlen(buffer) + 1);

    for(int i = 0; i < users->size; i++){
        int fifo = openUserFifo(users->userList[i].pid);
        if(fifo == -1){
            kill(users->userList[i].pid, SIGUSR1);
            continue;
        }
        int written = write(fifo, message, strlen(message));
        if(written < 0){
            kill(users->userList[i].pid, SIGUSR1);
        }
    }
    pthread_mutex_unlock(users->mutex);
}