/**
 * \file structs.h
 * 
 * \brief Structs module.
 * 
 * This module provides the necessary structs.
 * 
 * \author Guilherme Antunes - 103600
 * \author Pedro Rasinhas - 103541
*/

#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdbool.h>

/** \brief enum Action */
enum Action
{
    SORT,
    MERGE
};

/** \brief struct DistributorArgs */
typedef struct
{
    int sortType;
} DistributorArgs;

/** \brief struct SubArray */
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