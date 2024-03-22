#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>

enum Action
{
    SORT,
    MERGE
};

typedef struct
{
    int sortType;
} DistributorArgs;

typedef struct
{
    int id;
    int start;
    int size;
    int sortType;
    int dependent_1;
    int dependent_2;
    int threadId;
    bool completed;
    enum Action action;
} SubArray;

#endif