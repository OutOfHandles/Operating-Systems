#ifndef MANAGER_H
#define MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "../ultis.h"
#include "user.h"
#include "topic.h"

typedef struct{
    Users *users;
    Topics *topics;
} ThreadData;

#endif