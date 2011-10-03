/*
 * common.h
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#ifndef COMMON_H_
#define COMMON_H_

#include "constants.h"
#include "ctime.h"
#include <random>
#include <functional>
#include <string>
#include <boost/foreach.hpp>
#include <cstdlib>
#include <signal.h>
#include <sys/time.h>
#include <exception>


#define DEBUG


// Make Eclipse parser happy
#ifdef __CDT_PARSER__
    #define foreach(a, b) for(a : b)
#else
    #define foreach BOOST_FOREACH
#endif




namespace ducks {

// RANDOM NUMBERS
static std::uniform_int_distribution<unsigned int> unif;
static std::random_device rd;
static std::mt19937 engine(rd());
static std::function<unsigned int()> rnd = std::bind(unif, engine);

template<class real = double>
real random_delta(real number, real fraction = 0.2) {
	std::uniform_real_distribution<> dist(-number*fraction,number*fraction);
	return dist(engine);
}

template<class real = double>
real random(real lower = 0, real upper = 1) {
	std::uniform_real_distribution<> dist(lower, upper);
	return dist(engine);
}


// STRING CONVERSIONS
const char* speciesToString(ESpecies s);
const char* actionToString(EAction a);
const char* actionToShortString(EAction a);
std::string movementToString(int m);



// TIMEOUT
class timeout_exception: public std::exception
{
public:
	virtual const char* what() const throw()
	{
		return "Timeout";
	}
};

static bool gTimeout = false;

void timeout_handler (int i);

void check_timeout();



void ITimevalUntil(const CTime &pNow, const CTime &pUntil,struct itimerval &pDiff);

};


#endif /* COMMON_H_ */
