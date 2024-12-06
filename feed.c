#include "feed.h"
#include "ultis.h"

char feedFifo[50];
int fd_manager_fifo, fd_feed_fifo;

int sendToManager(Type t, void *data, int size){
    char bytes[MAX_BYTES];
    int total = 0;
    Headers req = {.pid = getpid(), .size = size, .type = t};
    
    toBytes(bytes, &total, &req, sizeof(Headers));
    toBytes(bytes, &total, data, size);

    return write(fd_manager_fifo, bytes, total);
}

void sendLogout(){
    Zero data;
    strcpy(data.command, "exit");

    sendToManager(ZERO, &data, sizeof(Zero));
}

void handle_signal(int sig){}

void handleSignal(int sig, siginfo_t *info, void *secret){
    switch(sig){
        case SIGUSR2:{
            if(info->si_value.sival_int == MANAGER_ERROR)
                printf("An error has occurred!\n");
            else
                printf("You have been kicked!\n");
            break;
        }
        case SIGTERM:{
            printf("Manager closed!\n");
            break;
        }
        case SIGINT:{
            sendLogout();
            break;
        }
        default: {
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

    struct sigaction sa;
    sa.sa_sigaction = handleSignal;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

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

    if(sendToManager(LOGIN, argv[1], strlen(argv[1]) + 1) <= 0){
        printf("Failed to send to manager\n");
    }

    ManagerStatus response;
    read(fd_feed_fifo, &response, sizeof(ManagerStatus));

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

        if(n_args == 0){
            Zero data;
            getCommandName(command, data.command);
            if(strcmp(data.command, "exit") == 0){
                sendLogout();
                break;
            }

            sendToManager(ZERO, &data, sizeof(Zero));
        }
        else if(n_args == 1){
            One data;
            getCommandName(command, data.command);
            getArg(command, data.arg);
            
            sendToManager(ONE, &data, sizeof(One));
        }
        else if(n_args == 3){
            Three data;
            getArg(command, data.topic);
            getLifeTime(command, &data.lifetime);
            getMessage(command, data.message);
            sendToManager(THREE, &data, sizeof(Three));
        }
    }

    pthread_kill(thread, SIGUSR1);
    pthread_join(thread, NULL);
    pthread_mutex_destroy(&mutex);
    Abort(0);
}