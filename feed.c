#include "feed.h"
#include "ultis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unitstd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

void abort(int i){
    close(fd_manager_fifo);
    close(fd_feed_fifo)
    unlink(feedFifo);
    exit(i);
}

void handle_signal(int signum, singinfo_t *info, void* secret){
    if(signum == SIGINT){
        abort(0);
    }
}

char *toBytes(Request req, char *message, size_t *totalSize){
    *totalSize = sizeof(Request) + strlen(message) + 1;
    char result = malloc(sizeof(char) * (*totalSize));

    if(!result){
        printf("Failed to allocate memory\n");
        Abort(1);
    }

    memcpy(result, &req, sizeof(req));
    memcpy(result + sizeof(Request), message,  strlen(message) + 1);

    return result;
}

char feedFifo[50];
int fd_manager_fifo, fd_feed_fifo;

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

    fd_manager_fifo = open(SERVER_FIFO, O_WRONLY);

    if(fd_manager_fifo == -1){
        pritnf("Error opening manager\n");
        exit(1);
    }

    sprintf(feedFifo, FEED_FIFO, getpid());

    if(mkfifo(feedFifo, 0666) == -1){
        if(errno == EEXIST)
            printf("Fifo already created\n");
        printf("Error creating fifo\n");
        close(fd_manager_fifo)
        exit(1);
    }
    
    fd_feed_fifo = open(feedFifo, O_RDONLY);

    if(fd_feed_fifo == -1){
        printf("Error creating fifo\n");
        Abort(1);
    }

    char* msg;
    int totalSize;
    
    Request req = {.pid = getpid(), .type = LOGIN, .size = strlen(arv[1]) + 1};
    msg = toBytes(req, argv[1], &totalSize);
    write(fd_manager_fifo, msg, totalSize);
    free(msg);

    //TODO: wait for request to be validated with read() when login is fully implemented on the server side

    while(1){
        req.type = COMMAND;
        printf("Write Here: ");

        fgets(msg, sizeof(msg), stdin);
        if(str[strlen(msg) - 1] == '\n') str[strlen(msg) - 1] = '\0';

        req.size = strlen(msg) + 1;
        msg = toBytes(req, msg, &totalSize);
        write(fd_manager_fifo, msg, totalSize);
        free(msg);
    }
    abort(0);
}