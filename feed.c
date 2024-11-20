#include "feed.h"
#include "ultis.h"

char feedFifo[50];
int fd_manager_fifo, fd_feed_fifo;

void Abort(int i){
    close(fd_manager_fifo);
    close(fd_feed_fifo);
    unlink(feedFifo);
    exit(i);
}

void handle_signal(int signum, siginfo_t *info, void* secret){
    if(signum == SIGINT){
        Abort(0);
    }
}

char *toBytes(Request req, char *message, int *totalSize){
    *totalSize = sizeof(Request) + strlen(message) + 1;
    char *result = malloc(sizeof(char) * (*totalSize));

    if(!result){
        printf("Failed to allocate memory\n");
        Abort(1);
    }

    memcpy(result, &req, sizeof(req));
    memcpy(result + sizeof(Request), message,  strlen(message) + 1);

    return result;
}

int main(int argc, char *argv[]){
    setbuf(stdout, NULL);
    
    if(argc != 2){
        printf("Invalid arguments\n");
        return 1;
    }
    
    struct sigaction sa;
    sa.sa_sigaction = handle_signal;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    fd_manager_fifo = open(MANAGER_FIFO, O_WRONLY);

    if(fd_manager_fifo == -1){
        printf("Error opening manager\n");
        exit(1);
    }

    sprintf(feedFifo, FEED_FIFO, getpid());

    if(mkfifo(feedFifo, 0666) == -1){
        if(errno == EEXIST)
            printf("Fifo already created\n");
        printf("Error creating fifo\n");
        close(fd_manager_fifo);
        exit(1);
    }
    
    fd_feed_fifo = open(feedFifo, O_RDONLY);

    if(fd_feed_fifo == -1){
        printf("Error creating fifo\n");
        Abort(1);
    }

    char *msg;
    int totalSize;
    
    Request req = {.pid = getpid(), .type = LOGIN, .size = strlen(argv[1]) + 1};
    msg = toBytes(req, argv[1], &totalSize);
    write(fd_manager_fifo, msg, totalSize);
    free(msg);

    //TODO: wait for request to be validated with read() when login is fully implemented on the server side
    char command[500];
    while(1){
        req.type = COMMAND;
        printf("Write Here: ");

        fgets(command, 500, stdin);
        if(command[strlen(command) - 1] == '\n') command[strlen(command) - 1] = '\0';

        req.size = strlen(command) + 1;
        msg = toBytes(req, command, &totalSize);
        write(fd_manager_fifo, msg, totalSize);
        free(msg);
    }
    Abort(0);
}