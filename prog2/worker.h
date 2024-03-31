/**
 * \file worker.h
 * 
 * \brief Worker module.
 * 
 * This module provides the worker function.
 * 
 * \author Guilherme Antunes - 103600
 * \author Pedro Rasinhas - 103541
*/

#ifndef WORKER_H
#define WORKER_H

/**
 *  \brief Function worker.
 *
 *  Its role is to simulate the life cycle of a worker.
 *
 *  \param par pointer to application defined worker identification
 */
void *worker(void *par);

#endif