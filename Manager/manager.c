#include "manager.h"

int fd_manager_fifo;

/*int openUserFifo(pid_t pid){
    char fifoName[25];
    sprintf(fifoName, FEED_FIFO, pid);
    int fifo = open(fifoName, O_WRONLY);

    return fifo;
}*/
/*
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
*/
/*ManagerStatus addUser(Users *users, char *name, User *newUser){
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
}*/

/*void handleLogin(Users *users, char *name, pid_t pid){
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
}*/

void Abort(int code){
    close(fd_manager_fifo);
    unlink(MANAGER_FIFO);

    exit(code);
}
/*
bool alreadyExists(Topics *topics, char *topic){
    for(int i = 0; i < topics->size; i++){
        if(strcmp(topics->topicsList[i].name, topic) == 0) return true;
    }

    return false;
}*/
/*
int loadFile(PersistentMessage pm[], int *size, Topics *topics){
    FILE *f = fopen("messages.txt", "r"); //change to read from env

    if(!f) return -1;
    int i = 0;
    while(fscanf(f, "%s %s %d %[^\n]s", pm[i].topic, pm[i].name, &pm[i].lifetime, pm[i].content) != EOF){
        if(!alreadyExists(topics, pm[i].topic)){
            strcpy(topics->topicsList[topics->size].name, pm[i].topic);
            topics->topicsList[topics->size++].locked = false;
        }
        i++;
    }
    *size = i;

    fclose(f);
    return 0;
}*/
/*
void saveToFile(PersistentMessage pm[], int size){
    FILE *f = fopen("messages.txt", "w"); //change to read from env

    if(!f){
        printf("Failed to open file\n");
        return;
    }
    
    for(int i = 0; i < size; i++){
        fprintf(f, "%s %s %d %s\n", pm[i].topic, pm[i].name, pm[i].lifetime, pm[i].content);
    }

    fclose(f);
}*/

void handleSignal(int sig){}

void *persistentThread(void *data){
    Topics *topics = (Topics*)data;
    signal(SIGUSR1, handleSignal);

    pthread_mutex_lock(topics->mutex);
    pthread_mutex_unlock(topics->mutex);

    while(1){
        pthread_mutex_lock(topics->mutex);
        if(!*topics->running){
            pthread_mutex_unlock(topics->mutex);
            break;
        }
        bool flag = false;
        for(int i = 0; i < topics->nTopics; i++){
            // pList->messageList[i].lifetime--;

            // if(pList->messageList[i].lifetime <= 0){
            //     flag = 1;
            // }
            flag = false;
            for(int j = 0; j < topics->topicsList[i].nPersistent; j++){
                if(--topics->topicsList[i].persistentList[j].lifetime <= 0){
                    flag = true;
                }
            }

            if(flag){
                int newSize = 0; 
                // for(int i = 0; i < pList->size; i++){
                //     if(pList->messageList[i].lifetime > 0)
                //         pList->messageList[newSize++] = pList->messageList[i]; 
                // }
                for(int j = 0; j < topics->topicsList[i].nPersistent; j++){
                    if(topics->topicsList[i].persistentList[j].lifetime > 0)
                        topics->topicsList[i].persistentList[newSize++] = topics->topicsList[i].persistentList[j];
                }
                topics->topicsList[i].nPersistent = newSize;
            }
        }
        
        pthread_mutex_unlock(topics->mutex);
        sleep(1);
    }

    printf("<MANAGER>: Closing persistent thread\n");

    pthread_exit(NULL);
}

void *pipeThread(void *data){
    ThreadData *td = (ThreadData*)data;
    signal(SIGUSR1, handleSignal);
    
    pthread_mutex_lock(td->topics->mutex);
    pthread_mutex_unlock(td->topics->mutex);

    while(1){
        pthread_mutex_lock(td->topics->mutex);
        if(!*td->topics->running){
            pthread_mutex_unlock(td->topics->mutex);
            break;
        }
        pthread_mutex_unlock(td->topics->mutex);

        Headers req;
        int size = read(fd_manager_fifo, &req, sizeof(Headers));

        if(size == -1) break; //handle == 0???

        /*switch(req.type){
            case LOGIN: {
                char buffer[req.size];
                size = read(fd_manager_fifo, buffer, req.size);

                if(size < 0){
                    printf("Failed to read request from feed\n");
                    kill(req.pid, SIGUSR1);
                    break;
                }
                
                if(size == -1) break;

                handleLogin(td->users, buffer, req.pid);
                break;
            }
            case LOGOUT: {
                removeUser(td->users, req.pid);
            }
            case COMMAND: {
                //handle command
                break;
            }
            default: {
                break;
            }
        }*/
    }

    printf("<MANAGER>: Closing pipe thread\n");

    pthread_exit(NULL); 
}

/*void showUsers(User users[], int size){
    if(size == 0){
        printf("<MANAGER>: No users at the moment\n");
    }
    for(int i = 0; i < size; i++){
        printf("<MANAGER>: [%d] -> %s\n", users[i].pid, users[i].name);
    }
}*/
/*void showPersistent(PersistentMessage messages[], int size, char *topic){
    bool shown = false;
    for(int i = 0; i < size; i++){
        if(strcmp(messages[i].topic, topic) == 0 || strcmp(topic, "*") == 0){ //|| strcmp(topic, "*") == 0
            printf("[%s] : <%s> : %d : %s\n", messages[i].name, messages[i].topic, messages[i].lifetime, messages[i].content);
            shown = true;
        }
    }

    if(!shown) printf("<MANAGER>: No messsages available at the moment\n");
}*/

/*
void showTopics(Topic topics[], int size){
    if(size == 0){
        printf("<MANAGER>: No topics at the moment\n");
    }
    for(int i = 0; i < size; i++){
        printf("<MANAGER>: %s [%s]\n", topics[i].name, topics[i].locked ? "LOCKED" : "UNLOCKED");
    }
}*/

void closeManager(Users *users){
    for(int i = 0; i < users->size; i++){
        kill(SIGTERM, users->userList[i].pid);
    }

    close(fd_manager_fifo);
    unlink(MANAGER_FIFO);
}
/*
void changeTopicState(Topic topics[], int size, char *name, bool state){
    bool changed = false;
    for(int i = 0; i < size; i++){
        if(strcmp(topics[i].name, name) == 0){
            if(topics[i].locked == state){ 
                printf("<MANAGER>: Topic already %s\n", state ? "locked" : "unlocked");
                return;
            }
            topics[i].locked = state;
            changed = true;
        }
    }
    if(!changed) 
        printf("<MANAGER>: Invalid topic name\n");
    else
        printf("<MANAGER>: Topic successfully %s\n", state ? "locked" : "unlocked");
}
*/

/*void kickUser(Users *users, int size, char *name){
    pid_t pid = -1;
    for(int i = 0; i < size; i++){
        if(strcmp(name, users->userList[i].name) == 0){
            pid = users->userList[i].pid;
            break;
        }
    }
    if(pid == -1){
        printf("<MANAGER>: Unable to find target user\n");
        return;
    }

    Headers h = {.pid = pid, .size = 0, .type = KICK};
    for(int i = 0; i < size; i++){
        int fifo = openUserFifo(users->userList[i].pid);
        if(fifo == -1){
            kill(users->userList[i].pid, SIGUSR1);
            continue;
        }
        int written = write(fifo, &h, sizeof(Headers));
        if(written < 0){
            kill(users->userList[i].pid, SIGUSR1);
        }
    }

    removeUser(users, pid);
}*/

int main(){
    setbuf(stdout, NULL);
    signal(SIGINT, handleSignal);
    bool running = true;
    
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
    pthread_mutex_t uMutex, tMutex;
    
    if(pthread_mutex_init(&uMutex, NULL) != 0|| pthread_mutex_init(&tMutex, NULL) != 0){
        printf("Failed to initialize mutex");
        Abort(1);
    }

    Topics topics = {.nTopics = 0, .mutex = &tMutex, .running = &running};
    Users users = {.size = 0, .mutex = &uMutex};
    
    if(loadFile(&topics) != 0){
        printf("Failed to open file\n");
        pthread_mutex_destroy(&uMutex);
        pthread_mutex_destroy(&tMutex);
        Abort(1);
    }

    ThreadData pipeThreadData = {.users = &users, .topics = &topics};

    pthread_mutex_lock(&tMutex);

    if(pthread_create(&threads[0], NULL, &pipeThread, &pipeThreadData) != 0 || pthread_create(&threads[1], NULL, &persistentThread, &topics) != 0){
        printf("Failed to create thread\n");
        running = false;
    }

    pthread_mutex_unlock(&tMutex);

    char command[BUFFER_SIZE];

    while(1){
        if(fgets(command, BUFFER_SIZE, stdin) == NULL){
            break;
        }
        if(command[strlen(command) - 1] == '\n') command[strlen(command) - 1] = '\0';
        int nArgs;
        CommandValidation v = validateCommand(command, &nArgs, MANAGER_COMMAND);
        printError(v);
        if(v != VALID && v != CHANGED_ARGUMENTS) continue;
        
        char commandName[MAX_COMMAND_LEN + 1];
        getCommandName(command, commandName);

        if(strcmp(commandName, "close") == 0){
            break;
        }
        if(strcmp(commandName, "users") == 0){
            showUsers(&users);
        }
        if(strcmp(commandName, "show") == 0){
            char topic[MAX_TOPIC_LEN + 1];
            getArg(command, topic);

            showPersistent(&topics, topic);
        }
        if(strcmp(commandName, "lock") == 0){
            char topic[MAX_TOPIC_LEN + 1];
            getArg(command, topic);

            changeTopicState(&topics, topic, true);
        }
        if(strcmp(commandName, "unlock") == 0){
            char topic[MAX_TOPIC_LEN + 1];
            getArg(command, topic);

            changeTopicState(&topics, topic, false);
        }
        if(strcmp(commandName, "remove") == 0){
            char name[MAX_NAME_LEN];
            getArg(command, name);
            
            kickUser(&users, name);
        }
        if(strcmp(commandName, "topics") == 0){
            showTopics(&topics);
        }
    }
    pthread_mutex_lock(&tMutex);
    running = false;
    pthread_mutex_unlock(&tMutex);

    printf("\n");
    for(int i = 0; i < 2; i++){
        pthread_kill(threads[i], SIGUSR1);
    }
 

    for(int i = 0; i < 2; i++){
        pthread_join(threads[i], NULL);
    }
    saveToFile(&topics);
    pthread_mutex_destroy(&uMutex);
    pthread_mutex_destroy(&tMutex);

    closeManager(&users);

    return 0;
}