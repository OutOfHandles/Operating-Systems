#include "feed.h"
#include "ultis.h"

char feedFifo[50];
int fd_manager_fifo, fd_feed_fifo;

void handle_signal(int sig){}

void handleSignal(int sig){
    switch(sig){
        case SIGUSR1:{
            printf("An error has occurred in the manager, closing...\n");
            break;
        }
        case SIGUSR2:{
            printf("You have been kicked!\n");
            break;
        }
        case SIGTERM:{
            printf("Manager closed!\n");
            break;
        }
        default:{
            break;
        }
    }
}

void* readManager(){
    signal(SIGUSR1, handle_signal);
    Headers req;
    while(1){
        int size = read(fd_feed_fifo, &req, sizeof(Headers));
        if(size == -1) break;
        
        if(req.type == MESSAGE){
            ServerMessage msg;
            size = read(fd_feed_fifo, &msg, sizeof(ServerMessage));
            if(size == -1) break;
            printf("%s\n", msg.message);
        }
    }

    pthread_exit(NULL);
}

void sendLogout(){
    Headers req = {.pid = getpid(), .size = 0, .type = LOGOUT};
    write(fd_manager_fifo, &req, sizeof(Headers));
}

void Abort(int i){
    close(fd_manager_fifo);
    close(fd_feed_fifo);
    unlink(feedFifo);
    exit(i);
}


int main(int argc, char *argv[]){
    setbuf(stdout, NULL);
    
    if(argc != 2){
        printf("\nInvalid arguments\n");
        return 1;
    }
    signal(SIGINT, handleSignal);
    signal(SIGUSR1, handleSignal);
    signal(SIGUSR2, handleSignal);
    signal(SIGTERM, handleSignal);

    fd_manager_fifo = open(MANAGER_FIFO, O_WRONLY);

    if(fd_manager_fifo == -1){
        printf("\nError opening manager\n");
        exit(1);
    }

    sprintf(feedFifo, FEED_FIFO, getpid());

    if(mkfifo(feedFifo, 0666) == -1){
        if(errno == EEXIST)
            printf("\nFifo already created\n");
        printf("\nError creating fifo\n");

        close(fd_manager_fifo);
        exit(1);
    }
    
    fd_feed_fifo = open(feedFifo, O_RDWR);

    if(fd_feed_fifo == -1){
        printf("Error creating fifo\n");
        Abort(1);
    }

    char bytes[MAX_BYTES];
    int totalSize = 0;
    
    Headers req = {.pid = getpid(), .type = LOGIN, .size = strlen(argv[1]) + 1};
    toBytes(bytes, &totalSize, &req, sizeof(req));
    toBytes(bytes, &totalSize, argv[1], strlen(argv[1]) + 1);

    int size = write(fd_manager_fifo, bytes, totalSize); //error handling later

    if(size <= 0) printf("Failed to send to manager\n");

    //TODO: wait for request to be validated with read() when login is fully implemented on the server side
    ManagerStatus response;
    size = read(fd_feed_fifo, &response, sizeof(ManagerStatus)); //error handling later
    memset(bytes, '\0', sizeof(char)*MAX_BYTES);
    totalSize = 0;

    switch(response){
        case SUCCESS: {
            printf("\nSuccessfully logged in!\n");
            break;
        }
        case MANAGER_FULL: {
            printf("\nManager is full\n");
            Abort(1);
        }
        case TRUNCATED: {
            printf("\nYour username has been truncated\n");
            break;
        }
        case USERNAME_IN_USE: {
            printf("\nUsername already in use\n");
            Abort(1);
        }
        default: {
            printf("%d\n", response);
            break;
        }
    }

    pthread_t thread;
    pthread_mutex_t mutex;
    if(pthread_mutex_init(&mutex, NULL) != 0){
        printf("Mutex init has failed\n");
        Abort(1);
    }

    if(pthread_create(&thread, NULL, &readManager, NULL)!= 0){
        printf("Thread can't be created\n");
        pthread_mutex_destroy(&mutex);
        Abort(1);
    }

    char command[500];
    while(1){
        req.type = COMMAND;
        int n_args = 0;
        if(fgets(command, BUFFER_SIZE, stdin) == NULL) break;
        if(command[strlen(command) - 1] == '\n') command[strlen(command) - 1] = '\0';

        CommandValidation v = validateCommand(command, &n_args, FEED_COMMAND);

        if(v == CHANGED_ARGUMENTS){
            printError(v);
        }

        if(v != VALID && v != CHANGED_ARGUMENTS){
            printError(v);  
            continue;
        }

        switch(n_args){
            case 0:{
                Zero data; // Mudar o nome do struct
                getCommandName(command, data.command);
                req.type = ZERO;
                req.size = sizeof(data);
                toBytes(bytes, &totalSize, &req, sizeof(Headers));
                toBytes(bytes, &totalSize, &data, sizeof(Zero));

                write(fd_manager_fifo, bytes, totalSize);
                totalSize = 0;
                break;
            }
            case 1:{
                One data;
                getCommandName(command, data.command);
                getArg(command, data.arg);
                req.type = ONE;
                req.size = sizeof(data);
                toBytes(bytes, &totalSize, &req, sizeof(Headers));
                toBytes(bytes, &totalSize, &data, sizeof(One));

                write(fd_manager_fifo, bytes, totalSize);
                totalSize = 0;
                break;
            }
            case 3:{
                Three data;
                getArg(command, data.topic);
                getLifeTime(command, &data.lifetime);
                getMessage(command, data.message);
                req.type = THREE;
                req.size = sizeof(Three);
                toBytes(bytes, &totalSize, &req, sizeof(Headers));
                toBytes(bytes, &totalSize, &data, sizeof(Three));
                
                write(fd_manager_fifo, bytes, totalSize);
                totalSize = 0;
                break;
            }
            default:{
                break;
            }
        }
    }
    pthread_kill(thread, SIGUSR1);
    pthread_join(thread, NULL);
    pthread_mutex_destroy(&mutex);
    sendLogout();
    printf("Logged out\n");
    Abort(0);
}