#include "manager.h"
#include "ultis.h"

int fd_manager_fifo, nUsers;
User users[MAX_USERS];

void handleLogin(int feed_fifo, char *username, char *message){
    if(nUsers >= MAX_USERS){
        strcpy(message, "MANAGER FULL\n");
        return;
    }
    strcpy(message, "SUCCESSFULLY LOGGED IN\n");
    
    User newUser;

    newUser.fd_client_fifo = feed_fifo;

    /*
        strncpy(newUser.name, username, MAX_NAME_LEN - 1);
        newUser.name[MAX_NAME_LEN - 1] = '\0';
    */

    snprintf(newUser.name, MAX_NAME_LEN, "%s", username); //acts like strcpy_s

    if(strlen(username) >= MAX_NAME_LEN){
        strcpy(message, "NAME WAS TRUNCATED TO: ");
        strcat(message, username);
    }

    users[nUsers++] = newUser;
}

void Abort(int code){
    close(fd_manager_fifo);
    unlink(MANAGER_FIFO);

    exit(code);
}

void handle_signal(int signum, siginfo_t *info, void* secret){
    if(signum == SIGINT){
        close(fd_manager_fifo);
    }
}

int manager_fifo;

int main(){
    setbuf(stdout, NULL);

    struct sigaction sa;
    sa.sa_sigaction = handle_signal;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

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

    /*TODO: Thread*/
    Request req;
    char buffer[500]; //malloc???
    while(1){
        int size = read(fd_manager_fifo, &req, sizeof(Request));
        
        if(size < 0){
            printf("Failed to read request from client\n");
            Abort(1);
        }

        size = read(fd_manager_fifo, buffer, req.size);

        if(size < 0){
            printf("Failed to read request from client\n");
            Abort(1);
        }


        switch(req.type){
            case LOGIN: {
                char message[50], fifo_name[50]; //modify sizes

                sprintf(fifo_name, FEED_FIFO, req.pid);
                int feed_fifo = open(fifo_name, O_WRONLY);
                
                if(feed_fifo == -1){
                    printf("Failed to open feed fifo\n");
                    kill(req.pid, SIGKILL);
                    break;
                }

                handleLogin(feed_fifo, buffer, message);
                
                int written = write(feed_fifo, message, strlen(message) + 1);

                if(written <= 0){
                    kill(req.pid, SIGKILL);
                    printf("Failed to send message to user\n");
                }

                break;
            }
            case COMMAND: {
                //handle command
                break;
            }
        }

    }
}