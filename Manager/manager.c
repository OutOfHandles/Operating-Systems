#include "manager.h"

int fd_manager_fifo;

void Abort(int code){
    close(fd_manager_fifo);
    unlink(MANAGER_FIFO);

    exit(code);
}

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
            flag = false;
            for(int j = 0; j < topics->topicsList[i].nPersistent; j++){
                if(--topics->topicsList[i].persistentList[j].lifetime <= 0){
                    flag = true;
                }
            }

            if(flag){
                int newSize = 0; 
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

        if(size == -1) break;

        switch(req.type){
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
            case ZERO: {
                Zero data;
                size = read(fd_manager_fifo, &data, req.size);                
                
                if(size < 0){
                    printf("Failed to read request from feed\n");
                    kill(req.pid, SIGUSR1);
                    break;
                }

                if(strcmp("exit", data.command) == 0){
                    removeUser(td->users, req.pid);
                }
                else if(strcmp("topics", data.command) == 0){
                    listTopicsUser(td->topics, req.pid);
                }

                break;
            }
            case ONE: {
                One data;
                size = read(fd_manager_fifo, &data, req.size);                
                
                if(size < 0){
                    printf("Failed to read request from feed\n");
                    kill(req.pid, SIGUSR1);
                    break;
                }

                if(strcmp("subscribe", data.command) == 0){
                    subscribeTopic(td->topics, td->users, data.arg, req.pid);
                }
                else if(strcmp("unsubscribe", data.command) == 0){
                    unsubscribeTopic(td->users, data.arg, req.pid);
                }
                
                break;
            }
            case THREE: {
                Three data;
                size = read(fd_manager_fifo, &data, req.size);                

                if(size < 0){
                    printf("Failed to read request from feed\n");
                    kill(req.pid, SIGUSR1);
                    break;
                }

                pthread_mutex_lock(td->topics->mutex);
                int index = getTopicIndex(td->topics, data.topic);
                if(index == -1){
                    pthread_mutex_unlock(td->topics->mutex);

                    int fifo = openUserFifo(req.pid);
                    if(sendServerMessage(fifo, "<MANAGER> Topic does not exist", req.pid) <= 0){
                        printf("<MANAGER> Failed to send message to user\n");
                        kill(req.pid, SIGUSR1);
                    }
                    continue;
                }
                if(td->topics->topicsList[index].locked){
                    pthread_mutex_unlock(td->topics->mutex);

                    int fifo = openUserFifo(req.pid);
                    if(sendServerMessage(fifo, "<MANAGER> Topic is currently locked", req.pid) <= 0){
                        printf("<MANAGER> Failed to send message to user\n");
                        kill(req.pid, SIGUSR1);
                    }
                    continue;
                }

                if(data.lifetime > 0) addPersistent(td->topics, td->users, data, req.pid);
                broadCastMessage(td->users, data.message, data.topic, req.pid);

                pthread_mutex_unlock(td->topics->mutex);

                break;
            }
            default: {
                break;
            }
        }
    }

    printf("<MANAGER>: Closing pipe thread\n");

    pthread_exit(NULL); 
}

void closeManager(Users *users){
    for(int i = 0; i < users->size; i++){
        kill(users->userList[i].pid, SIGTERM);
    }

    close(fd_manager_fifo);
    unlink(MANAGER_FIFO);
}

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
        else if(strcmp(commandName, "users") == 0){
            showUsers(&users);
        }
        else if(strcmp(commandName, "show") == 0){
            char topic[MAX_TOPIC_LEN + 1];
            getArg(command, topic);

            showPersistent(&topics, topic);
        }
        else if(strcmp(commandName, "lock") == 0){
            char topic[MAX_TOPIC_LEN + 1];
            getArg(command, topic);

            changeTopicState(&topics, topic, true);
        }
        else if(strcmp(commandName, "unlock") == 0){
            char topic[MAX_TOPIC_LEN + 1];
            getArg(command, topic);

            changeTopicState(&topics, topic, false);
        }
        else if(strcmp(commandName, "remove") == 0){
            char name[MAX_NAME_LEN];
            getArg(command, name);
            
            kickUser(&users, name);
        }
        else if(strcmp(commandName, "topics") == 0){
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