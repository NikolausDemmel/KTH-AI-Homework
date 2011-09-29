/*
 * common.h
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "constants.h"
#include <random>
#include <functional>
#include <string>


#define DEBUG



namespace ducks {

// RANDOM NUMBERS
static std::uniform_int_distribution<unsigned int> unif;
static std::random_device rd;
static std::mt19937 engine(rd());
static std::function<unsigned int()> rnd = std::bind(unif, engine);

template<class real = double>
real random_delta(real number, real fraction = 0.1) {
	std::uniform_real_distribution<> dist(-number*fraction,number*fraction);
	return dist(engine);
}


// STRING CONVERSIONS
const char* speciesToString(ESpecies s);
const char* actionToString(EAction a);
const char* actionToShortString(EAction a);
std::string movementToString(int m);



};


#endif /* COMMON_H_ */
