#ifndef UTILS_H
#define UTILS_H

#define MANAGER_FIFO "MANAGER_FIFO"
#define FEED_FIFO "FEED_FIFO_%d"

typedef enum {LOGIN, COMMAND} Type;

typedef struct{
    Type type;
    pid_t pid;
    size_t size;
} Request;

#endif