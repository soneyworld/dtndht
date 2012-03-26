/*
 * evaluation.h
 *
 *  Created on: 12.03.2012
 *      Author: Till Lorentzen
 */

#ifndef EVALUATION_H_
#define EVALUATION_H_

/**
 * Prints the actual time to stdout without a line break
 */
void printf_time(void);

/**
 * Prints all the start of every evaluation message to stdout
 */
void printf_evaluation_start(void);

/**
 * prints the given hash as hex to stdout
 */
void printf_hash(const unsigned char *buf);

/**
 * prints a line break and flushes the stdout stream
 */
void fflush_evaluation(void);

#endif /* EVALUATION_H_ */
