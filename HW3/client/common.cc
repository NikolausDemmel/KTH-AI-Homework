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


};

