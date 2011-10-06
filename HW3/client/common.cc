/*
 * common.cc
 *
 *  Created on: 29.09.2011
 *      Author: demmeln
 */

#include "common.h"
#include <limits>

using namespace std;


namespace ducks {

const char* speciesToString(ESpecies s) {
	switch(s) {
	case SPECIES_UNKNOWN:
		return "Unknown";
	case SPECIES_WHITE:
    	return "White";
	case SPECIES_BLACK:
		return "Black";
	case SPECIES_BLUE:
		return "Blue";
	case SPECIES_RED:
		return "Red";
	case SPECIES_GREEN:
		return "Green";
	case SPECIES_YELLOW:
		return "Yellow";
	}
}

const char* actionToString(EAction a) {
	switch(a) {
	case ACTION_ACCELERATE:
		return "Accelerate";
	case ACTION_KEEPSPEED:
		return "Keep Speed";
	case ACTION_STOP:
		return "Stop";
	}
}

const char* actionToShortString(EAction a) {
	switch(a) {
	case ACTION_ACCELERATE:
		return "A";
	case ACTION_KEEPSPEED:
		return "K";
	case ACTION_STOP:
		return "S";
	}
}

string movementToString(int m) {
	switch(m) {
	case BIRD_STOPPED:
		return "Stopped";
	case BIRD_DEAD:
		return "Dead";
	}
	string s;
	if ( m & MOVE_WEST ) {
		s += "WEST ";
	}

	if ( m & MOVE_EAST ) {
		s += "EAST ";
	}

	if ( m & MOVE_UP ) {
		s += "UP ";
	}

	if ( m & MOVE_DOWN ) {
		s += "DOWN ";
	}

	if (!s.empty()) {
		return s.substr(0, s.size()-1);
	}

	return "Invalid Movement";
}


bool gTimeout = false;


void reset_timeout_flag() {
	gTimeout = false;
}

void timeout_handler (int i) {
	gTimeout = true;
}

void check_timeout() {
	if(gTimeout) {
		throw timeout_exception();
	}
}

void ITimevalUntil(const CTime &pNow, const CTime &pUntil,struct itimerval &pDiff)
{
    int64_t lDiff=pUntil.Get()-pNow.Get();
    if(lDiff<=0)
    {
        pDiff.it_value.tv_sec=0;
        pDiff.it_value.tv_usec=0;
    }
    else
    {
        pDiff.it_value.tv_sec=lDiff/1000000;
        pDiff.it_value.tv_usec=lDiff%1000000;
    }
    pDiff.it_interval.tv_sec = 0;
    pDiff.it_interval.tv_usec = 0;
}


bool hStopped(int movement) {
	return !(movement & (MOVE_WEST | MOVE_EAST));
}
bool vStopped(int movement) {
	return !(movement & (MOVE_UP | MOVE_DOWN));
}



};

