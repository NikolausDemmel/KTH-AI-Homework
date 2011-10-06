/*
 * duckinfo.h
 *
 *  Created on: 02.10.2011
 *      Author: demmeln
 */

#ifndef DUCKINFO_H_
#define DUCKINFO_H_

#include "defines.h"
#include "cduck.h"
#include "duckobservation.h"
#include <iostream>
#include <vector>
#include <algorithm>

using std::cout;
using std::endl;


namespace ducks {


const double WhiteReward = 3;
const double ColorReward = 5;
const double BlackReward = -300;
const double MissReward = -1;

const double UnknownReward = 2; // FIXME



static double speciesReward(ESpecies spec) {
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


class DuckInfo
{
public:
	typedef HMM<3,DuckObservation::numObservations, DuckObservation> hmm_t;


	DuckInfo():
		mDuck(0),
		mObs(),
		mModel(DuckObservation::getSplitNames()),
		mLastRound(0),
		mNumUp(0),
		mNumDown(0),
		mNumVStopped(0),
		mNumWest(0),
		mNumEast(0),
		mNumHStopped(0),
		mRoundOfDeath(-1),
		mSpecies(SPECIES_UNKNOWN)
	{
	}

	void setDuck(const CDuck *duck) {
		mModel.setObservations(&mObs);
		mDuck = duck;
	}

	void setNumber(int i) {
		mNumber = i;
	}

	bool isDead() {
		return mRoundOfDeath >= 0;
	}

	bool couldBeBlack() {
		return false; // FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}

	void iteration(int round) {
//		mAllModels.resize(round);
//		mAllModels[round-1] = hmm_t::model_t(mModel.getModel());
		if (!isDead()) {
			for (int i = mLastRound; i < round; ++i) {
				int move = mDuck->GetAction(i).GetMovement();

				if (move & BIRD_DEAD) {
					mRoundOfDeath = i;
					break;
				}

				if (move & MOVE_UP) {
					++mNumUp;
				} else if (move & MOVE_DOWN) {
					++mNumDown;
				} else {
					++mNumVStopped;
				}
				if (move & MOVE_WEST) {
					++mNumWest;
				} else if (move & MOVE_EAST) {
					++mNumEast;
				} else {
					++mNumHStopped;
				}
			}
		}
		mLastRound = round;
	}

	void update(bool verbose = false)
	{
		if (isDead())
			return;

		int n = mObs.size();
		for (int i = n; i < mDuck->GetSeqLength(); ++i) {
			if (mDuck->GetAction(i).GetMovement() & BIRD_DEAD)
				break;
			mObs.push_back(mDuck->GetAction(i));
		}

		mModel.learnModel(10, 30, false, verbose);
	}

	hmm_t& getModel() {
		return mModel;
	}

	static void normalizeDist(std::vector<prob> &dist) {
		double sum = std::accumulate( dist.begin(), dist.end(), 0.0 );
		for (auto it = dist.begin(); it != dist.end(); ++it) {
			(*it) /= sum;
		}
	}

	CAction makeAction(int observation) {
		CAction action = DuckObservation(observation).toAction();
		int curr_move = mDuck->GetLastAction().GetMovement();
		EAction v = action.GetVAction();
		EAction h = action.GetHAction();
		int move = curr_move;
		if (h == ACTION_ACCELERATE) {
			if (hStopped(curr_move)) {
				if ( random() > 0.5) {
					move |= MOVE_WEST;
				} else {
					move |= MOVE_EAST;
				}
			}
		}
		if (v == ACTION_ACCELERATE) {
			if (vStopped(curr_move)) {
				if ( random() > 0.5) {
					move |= MOVE_UP;
				} else {
					move |= MOVE_DOWN;
				}
			}
		}
		if (h == ACTION_STOP) {
			move &= ~(MOVE_WEST | MOVE_EAST);
		}

		if (v == ACTION_STOP) {
			move &= ~(MOVE_UP | MOVE_DOWN);
		}

		return CAction(mNumber, h, v, move);
	}

	// bestObs will be set to the bestObservation that the returned prob refers to
	prob expectedReward(int *pBestObs = 0) {
		auto nextObsDist = mModel.distNextObs();
		auto splitDist = mModel.split_obs_dist(nextObsDist);
		auto hDist = splitDist[DuckObservation::splitHindex];
		auto vDist = splitDist[DuckObservation::splitVindex];
		auto move = mDuck->GetLastAction().GetMovement();
		hmm_t::obs_dist_t expected_rewards;

		if (hStopped(move)) {
			hDist[ACTION_KEEPSPEED] = 0;
			normalizeDist(hDist);
		}

		if (vStopped(move)) {
			vDist[ACTION_KEEPSPEED] = 0;
			normalizeDist(vDist);
		}

		double factor;
		for (int h = 0; h < 3; ++h) {
			if (hStopped(move) && (h == ACTION_ACCELERATE))
				factor = 0.5;
			else
				factor = 1;
			for (int v = 0; v < 3; ++v) {
				if (vStopped(move) && (v == ACTION_ACCELERATE))
					factor *= 0.5;
				int obs = DuckObservation::hvToObs(h, v);
				double probHit = factor * nextObsDist[obs];
				expected_rewards[obs] = probHit * speciesReward(mSpecies) + (1.0 - probHit) * MissReward;
			}
		}

		auto bestIter = std::max_element(expected_rewards.begin(), expected_rewards.end());
		if (pBestObs) {
			*pBestObs = bestIter - expected_rewards.begin();
		}

		return *bestIter;
	}

	void analyseDevelopment() {
//		for (int t = 0; t < mAllModels.size(); ++t) {
//			hmm_t::model_t m = mAllModels[t];
//			cout << "t " << t << ", dist " << std::setprecision(15) << mModel.simple_distance(m);
//			hmm_t hmm(mAllModels[t]);
//			cout << ", kl dist " << std::setprecision(15) << mModel.distance(hmm) << endl;
//		}
	}

	prob currDistance(DuckInfo &other) {
		//cout <<  "foo: " << std::setprecision(15) << mModel.simple_distance(other.mModel.getModel());
		return fabs(mModel.kullback_leibler_distance_sample(other.mModel));
	}

private:

	const CDuck *mDuck;
	int mNumber;
	hmm_t mModel;
	std::vector<DuckObservation> mObs;
	int mLastRound;

	int mNumUp;
	int mNumDown;
	int mNumVStopped;
	int mNumWest;
	int mNumEast;
	int mNumHStopped;
	int mRoundOfDeath;

	ESpecies mSpecies;

	// std::vector<hmm_t::model_t> mAllModels;
};


}


#endif /* DUCKINFO_H_ */
