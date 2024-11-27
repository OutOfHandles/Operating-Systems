#include "manager.h"

int fd_manager_fifo, running = 1;


int openUserFifo(pid_t pid){
    char fifoName[20];
    sprintf(fifoName, FEED_FIFO, pid);
    int fifo = open(fifoName, O_WRONLY);

    return fifo;
}

void removeUser(Users *users, pid_t pid){
    int newSize = 0;
    for(int i = 0; i < *users->size; i++){
        if(users->users[i].pid != pid)
            users->users[newSize++] = users->users[i];
    }
    *users->size = newSize;
}

ManagerStatus addUser(Users *users, char *name, User *newUser){
    if(*users->size >= MAX_USERS){
        return MANAGER_FULL;
    }
    for(int i = 0; i < *users->size; i++){
        if(strcmp(name, users->users[i].name) == 0) return USERNAME_IN_USE;
    }
    ManagerStatus ret = SUCCESS;

    snprintf(newUser->name, MAX_NAME_LEN, "%s", name); //acts like strcpy_s

    if(strlen(name) >= MAX_NAME_LEN)
        ret = TRUNCATED;

    users->users[(*users->size)++] = *newUser;
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
    ManagerStatus status = addUser(users , name, &newUser);
    
    int written = write(fifo, &status, sizeof(ManagerStatus));
    
    if(written <= 0){
        kill(pid, SIGUSR1);
        printf("Failed to send response to user\n");
    }

    close(fifo);
}

void Abort(int code){
    close(fd_manager_fifo);
    unlink(MANAGER_FIFO);

    exit(code);
}

int loadFile(PersistentMessage pm[], int *size){
    FILE *f = fopen("messages.txt", "r"); //change to read from env

    if(!f) return -1;

    while(fscanf(f, "%s %s %d %[^\n]s", pm[(*size)++].name, pm[(*size)++].topic, &pm[(*size)++].lifetime, pm[(*size)++].content) != EOF);

    return 0;
}

void saveToFile(PersistentMessage pm[], int size){
    FILE *f = fopen("messages.txt", "w"); //change to read from env

    if(!f){
        printf("Failed to open file\n");
        return;
    }
    
    for(int i = 0; i < size; i++){
        fprintf(f, "%s %s %d %s", pm[size++].name, pm[size++].topic, &pm[size++].lifetime, pm[size++].content);
    }
}

void handle_signal(int signum, siginfo_t *info, void* secret){}

void *persistentThread(void *data){
    PersistentList *pList = (PersistentList*)data;

    pthread_mutex_lock(pList->mutex);
    pthread_mutex_unlock(pList->mutex);

    while(running){
        pthread_mutex_lock(pList->mutex);

        int flag = 0;
        for(int i = 0; i < *pList->size; i++){
            pList->messages[i].lifetime--;

            if(pList->messages[i].lifetime <= 0){
                flag = 1;
            }
        }

        if(flag != 0){
            int newSize = 0; 
            for(int i = 0; i < *pList->size; i++){
                if(pList->messages[i].lifetime > 0)
                    pList->messages[newSize] = pList->messages[i]; 
            }
            *pList->size = newSize;
        }
        
        pthread_mutex_unlock(pList->mutex);
        sleep(1);
    }

    pthread_exit(NULL);
}

void *pipeThread(void *data){
    ThreadData *td = (ThreadData*)data;
    
    pthread_mutex_lock(td->persistent.mutex);
    pthread_mutex_unlock(td->persistent.mutex);
    
    while(running){
        Headers req;
        int size = read(fd_manager_fifo, &req, sizeof(Headers));

        if(size < 0){
            printf("Failed to read request from feed\n");
        }

        switch(req.type){
            case LOGIN: {
                char buffer[req.size];
                size = read(fd_manager_fifo, buffer, req.size);

                if(size < 0){
                    printf("Failed to read request from feed\n");
                    kill(req.pid, SIGUSR1);
                    break;
                }

                handleLogin(&td->users, buffer, req.pid);
                break;
            }
            case LOGOUT: {
                removeUser(&td->users, req.pid);
            }
            case COMMAND: {
                //handle command
                break;
            }
            default: {
                break;
            }
        }
    }

    pthread_exit(NULL); 
}

int main(){
    setbuf(stdout, NULL);
    struct sigaction sa;
    sa.sa_sigaction = handle_signal;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    
    User users[MAX_USERS];
    PersistentMessage persistent[MAX_PERSISTENT];
    int nUsers = 0, nPersis = 0;
    
    if(loadFile(persistent, &nPersis) != 0){
        printf("Failed to open file\n");
        exit(1);
    }

    if(mkfifo(MANAGER_FIFO, 0666) == -1){
        if(errno == EEXIST)
            printf("Fifo already created\n");
        else
            printf("Error creating fifo\n");
        
        exit(0);
    }

    fd_manager_fifo = open(MANAGER_FIFO, O_RDWR);
    
    if(fd_manager_fifo == -1){
        printf("Error opening fifo\n");
        Abort(1);
    }

    pthread_t threads[2];
    pthread_mutex_t pMutex;
    
    if(pthread_mutex_init(&pMutex, NULL) != 0){
        printf("Failed to initialize mutex");
        Abort(1);
    }

    PersistentList perThreadData = {.messages = persistent, .size = &nUsers, .mutex = &pMutex};
    ThreadData pipeThreadData = {.users = {.users = users, .size = &nUsers}, 
        .persistent = {.messages = persistent, .size = &nUsers, .mutex = &pMutex}};

    pthread_mutex_lock(&pMutex);

    if(pthread_create(&threads[0], NULL, &pipeThread, &pipeThreadData) != 0 || pthread_create(&threads[1], NULL, &persistentThread, &perThreadData) != 0){
        printf("Failed to create thread\n");
        running = 0;
    }

    pthread_mutex_unlock(&pMutex);

    while(running){
        //read server commands
    }

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    saveToFile(persistent, nPersis);
    pthread_mutex_destroy(&pMutex);

    close(fd_manager_fifo);
    unlink(MANAGER_FIFO);

    return 0;
}