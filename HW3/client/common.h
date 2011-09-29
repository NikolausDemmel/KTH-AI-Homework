/*
 * common.h
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "constants.h"
#include <boost/foreach.hpp>
#include <random>
#include <functional>
#include <string>

using namespace std;


// DEFINITIONS:
#define foreach BOOST_FOREACH


namespace ducks {


// RANDOM NUMBERS
uniform_int_distribution<unsigned int> unif;
random_device rd;
mt19937 engine(rd());
function<unsigned int()> rnd = bind(unif, engine);


// STRING CONVERSIONS
const char* speciesToString(ESpecies s);
const char* actionToString(EAction a);
string movementToString(int m);



};


#endif /* COMMON_H_ */
