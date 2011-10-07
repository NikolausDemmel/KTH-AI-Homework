#include "duckinfo.h"
#include "cplayer.h"

namespace ducks {


void DuckInfo::notifyBlackBirdFound() {
	mPlayer->notifyBlackBirdFound();
}

bool DuckInfo::isBlackBirdFound() {
	return mPlayer->isBlackBirdFound();
}

double speciesReward(ESpecies spec) {
	switch(spec) {
	case SPECIES_WHITE:
		return WhiteReward;
	case SPECIES_BLACK:
		return BlackReward;
	case SPECIES_BLUE:
	case SPECIES_RED:
	case SPECIES_YELLOW:
	case SPECIES_GREEN:
		return ColorReward;
	case SPECIES_UNKNOWN:
		return UnknownReward;
	}
}

Pattern categorizeBrow(std::vector<prob> &BHori, std::vector<prob> &BVert) {
	if ( BVert[ACTION_ACCELERATE] > 0.7 &&
		 BVert[ACTION_STOP] < 0.1 &&
	     BHori[ACTION_STOP] > 0.5 &&
	     BHori[ACTION_KEEPSPEED] < 0.1) {
		return FeigningDeath;
	}
	if ( BVert[ACTION_ACCELERATE] > 0.6 &&
	     BVert[ACTION_STOP] < 0.1 &&
	     BHori[ACTION_ACCELERATE] > 0.6 &&
	     BHori[ACTION_STOP] < 0.1) {
		return Panicking;
	}
	if ( BVert[ACTION_STOP] > 0.7 &&
		 BVert[ACTION_KEEPSPEED] < 0.1 &&
		 BHori[ACTION_STOP] < 0.2 ) {
		return Migrating;
	}
	if ( BVert[ACTION_ACCELERATE] > 0.30 &&
		 BHori[ACTION_ACCELERATE] > 0.30 &&
		 BVert[ACTION_STOP] > 0.30 &&
		 BHori[ACTION_STOP] > 0.30 &&
		 BVert[ACTION_KEEPSPEED] < 0.15 &&
		 BHori[ACTION_KEEPSPEED] < 0.15) {
		return Quacking;
	}
	return UnknownPattern;
}

std::string patternToString(Pattern pat) {
	switch(pat) {
	case UnknownPattern:
		return "Unknown";
	case Quacking:
		return "Quacking";
	case Migrating:
		return "Migrating";
	case Panicking:
		return "Panicking";
	case FeigningDeath:
		return "FeignDeath";
	}
}


std::vector<std::string> patternToString(const std::vector<Pattern> &pats) {
	std::vector<std::string> res;
	foreach ( Pattern pat, pats ) {
		res.push_back(patternToString(pat));
	}
	return res;
}


}
