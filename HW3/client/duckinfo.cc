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
	     BHori[ACTION_STOP] > 0.65 &&
	     BHori[ACTION_KEEPSPEED] < 0.1) {
		return FeigningDeath;
	}
	if ( BVert[ACTION_ACCELERATE] > 0.65 &&
	     BVert[ACTION_STOP] < 0.1 &&
	     BHori[ACTION_ACCELERATE] > 0.65 &&
	     BHori[ACTION_STOP] < 0.1) {
		return Panicking;
	}
	if ( BVert[ACTION_STOP] > 0.7 &&
		 BVert[ACTION_KEEPSPEED] < 0.1 &&
		 BHori[ACTION_STOP] < 0.1 ) {
		return Migrating;
	}
	if ( BVert[ACTION_ACCELERATE] > 0.30 &&
		 BHori[ACTION_ACCELERATE] > 0.30 &&
		 BVert[ACTION_STOP] > 0.30 &&
		 BHori[ACTION_STOP] > 0.30 &&
		 BVert[ACTION_KEEPSPEED] < 0.10 &&
		 BHori[ACTION_KEEPSPEED] < 0.10) {
		return Quacking;
	}
//	if ( BVert[ACTION_ACCELERATE] > 0.7 &&
//		 BVert[ACTION_STOP] < 0.1 &&
//	     BHori[ACTION_STOP] > 0.5 &&
//	     BHori[ACTION_KEEPSPEED] < 0.1) {
//		return FeigningDeath;
//	}
//	if ( BVert[ACTION_ACCELERATE] > 0.6 &&
//	     BVert[ACTION_STOP] < 0.1 &&
//	     BHori[ACTION_ACCELERATE] > 0.6 &&
//	     BHori[ACTION_STOP] < 0.1) {
//		return Panicking;
//	}
//	if ( BVert[ACTION_STOP] > 0.7 &&
//		 BVert[ACTION_KEEPSPEED] < 0.1 &&
//		 BHori[ACTION_STOP] < 0.2 ) {
//		return Migrating;
//	}
//	if ( BVert[ACTION_ACCELERATE] > 0.30 &&
//		 BHori[ACTION_ACCELERATE] > 0.30 &&
//		 BVert[ACTION_STOP] > 0.30 &&
//		 BHori[ACTION_STOP] > 0.30 &&
//		 BVert[ACTION_KEEPSPEED] < 0.15 &&
//		 BHori[ACTION_KEEPSPEED] < 0.15) {
//		return Quacking;
//	}
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

// TODO: use learned full B matrix

// unsmoothed values;
const double init_real_B[4][9] = {
		0.24961,	0.00053,	0.24327,	0.00032,	0.00000,	0.00032,	0.25596,	0.00054,	0.24946,
		0.02743,	0.10264,	0.00090,	0.00007,	0.00004,	0.00003,	0.19820,	0.66903,	0.00167,
		0.63278,	0.13284,	0.00028,	0.19692,	0.03470,	0.00008,	0.00037,	0.00060,	0.00143,
		0.06889,	0.00002,	0.70775,	0.02080,	0.00000,	0.19957,	0.00177,	0.00003,	0.00116
};


#ifdef NOTDEFINED
// Measured values

const double init_hor[4][3] = {
		0.540, 0.000, 0.460, // Quacking = 0,
		0.210, 0.780, 0.010, // Migrating = 1,
		0.830, 0.160, 0.010, // Panicking = 2,
		0.190, 0.000, 0.810  // FeigningDeath = 3,
};
const double init_ver[4][3] = {
		0.530, 0.000, 0.470, // Quacking = 0,
		0.140, 0.000, 0.860, // Migrating = 1,
		0.830, 0.160, 0.010, // Panicking = 2,
		0.890, 0.090, 0.010  // FeigningDeath = 3,
};

#else
// Manually smoothed values
const double init_hor[4][3] = {
		0.500, 0.000, 0.500, // Quacking = 0,
		0.250, 0.750, 0.000, // Migrating = 1,
		0.850, 0.150, 0.000, // Panicking = 2,
		0.100, 0.000, 0.900  // FeigningDeath = 3,
};
const double init_ver[4][3] = {
		0.500, 0.000, 0.500, // Quacking = 0,
		0.150, 0.000, 0.850, // Migrating = 1,
		0.750, 0.250, 0.000, // Panicking = 2,
		0.750, 0.250, 0.000  // FeigningDeath = 3,
};

#endif

DuckInfo::hmm_fixed_t::state_obs_trans_t DuckInfo::getFullObservationMatrix() {
	hmm_fixed_t::state_obs_trans_t B;

	for (int k = 0; k < 4; ++k) {
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				B(k,i+3*j) = init_hor[k][i] * init_ver[k][j];
			}
		}
	}

//	B  = make_matrix_from_pointer(init_real_B);

	return B;
}

DuckInfo::hmm_t::state_obs_trans_t DuckInfo::getMissingPatternObservationMatrix(Pattern notPattern) {

	hmm_t::state_obs_trans_t B;

	for (int n = 0, k = 0; n < 4; ++n) {
		if (n == notPattern)
			continue;
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				B(k,i+3*j) = init_hor[n][i] * init_ver[n][j];
			}
		}
		++k;
	}

	return B;

}


}
