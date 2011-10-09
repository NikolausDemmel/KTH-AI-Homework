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


const int cRoundsUnknownDiscardPattern = 50;

const double cWhiteReward = 3;
const double cColorReward = 5;
const double cBlackReward = -300;
const double cMissReward = -1;

const double UnknownReward = 4; // FIXME


enum Answer {
	SayYes,
	SayNo,
	SayMaybe
};

enum Group {
	UnknownGroup,
	WhiteGroup,
	BlackGroup,
	ColorGroup
};

enum Pattern {
	Quacking = 0,
	Migrating = 1,
	Panicking = 2,
	FeigningDeath = 3,
	UnknownPattern = 4
};


std::string patternToString(Pattern pat);
std::string groupToString(Group group);

static const std::list<Pattern> gAllPatterns = { Quacking, Migrating, Panicking, FeigningDeath };
std::vector<std::string> patternToString(const std::vector<Pattern> &pats);


double speciesReward(ESpecies spec);

Pattern categorizeBrow(std::vector<prob> &BHori, std::vector<prob> &BVert);

class CPlayer;

class DuckInfo
{
public:
	typedef HMM<3,DuckObservation::numObservations, DuckObservation> hmm_t;
	typedef HMM<4,DuckObservation::numObservations, DuckObservation> hmm_fixed_t;


	DuckInfo():
		mDuck(0),
		mObs(),
		mModel(DuckObservation::getSplitNames()), // FIXME
		//mModel(),
		mLastRound(0),
		mNumUp(0),
		mNumDown(0),
		mNumVStopped(0),
		mNumWest(0),
		mNumEast(0),
		mNumHStopped(0),
		mRoundOfDeath(-1),
		mSpecies(SPECIES_UNKNOWN),
//		mPatterns({UnknownPattern, UnknownPattern, UnknownPattern}),
//		mPatternsLastKnown({-1,-1,-1}),
		mFixedModel(),
		mFixedModelMissingPattern(UnknownPattern),
		mFixedModelMissingPatternCertain(false),
		mFixedModelMissingPatternLastKnown(-1)
	{
	}

	void initialize(const CDuck *duck, int number, CPlayer *player, bool practiceMode) {
		mFixedModel.fixedBInitialization(getFullObservationMatrix(), 0, 0.00001);
		mModel.hardcodedInitialization();

		mModel.setObservations(&mObs);
		mFixedModel.setObservations(&mObs);

		mDuck = duck;
		mNumber = number;
		mPlayer = player;
		mPracticeMode = practiceMode;
	}

	void notifyBlackBirdFound();
	bool isBlackBirdFound();

	bool isDead() {
		return mRoundOfDeath >= 0;
	}

	bool couldBeBlack() {
		if (isBlackBirdFound())
			return false;
		return false; // FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	}

	void iteration(int round) {
		if (!mPracticeMode && !isDead()) {
			for (int i = mLastRound; i < round; ++i) {
				int move = mDuck->GetAction(i).GetMovement();

				if (move & BIRD_DEAD) {
					// Bird became dead. See if we know its species.
					mRoundOfDeath = i;
					if (mDuck->GetSpecies() != SPECIES_UNKNOWN) {
						// this means we have hit the duck
						mSpecies = mDuck->GetSpecies();
						if (mSpecies == SPECIES_BLACK) {
							notifyBlackBirdFound();
						}
					}
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
		if (!mPracticeMode && isDead())
			return;

		int n = mObs.size();
		for (int i = n; i < mDuck->GetSeqLength(); ++i) {
			if (!mPracticeMode && (mDuck->GetAction(i).GetMovement() & BIRD_DEAD))
				break;
			mObs.push_back(mDuck->GetAction(i));
		}

		if (mPracticeMode) {

			mModel.learnModel(300, 300, true, verbose);
			mFixedModel.learnModel(300, 300, true, verbose);

		} else {

			mModel.learnModel(10, 30, false, verbose);
			mFixedModel.learnModel(10, 30, false, verbose);

			categorizeDuck();

		}

	}

//	Answer hasPattern(Pattern p) {
//		bool hasUnknown = false;
//		for (int i = 0; i < 3; ++i) {
//			if (mPatterns[i] == p)
//				return SayYes;
//			if (mPatterns[i] == UnknownPattern)
//				hasUnknown = true;
//		}
//
//		if (hasUnknown)
//			return SayMaybe;
//		else
//			return SayNo;
//
//	}

	void categorizeDuck() {
//		if (!mFixedModelMissingPatternCertain)
			fixedModelUpdateMissingPattern();


//		mModel.calculate_B_split();
//		auto split = mModel.getBSplit();
//		auto Bh = (*split)[DuckObservation::splitHindex];
//		auto Bv = (*split)[DuckObservation::splitVindex];
//
//		std::vector<prob> vrow(3), hrow(3);
//		std::list<Pattern> allPats(gAllPatterns);
//
//		for (int i = 0; i < 3; ++i) {
//			for (int j = 0; j < 3; ++j) {
//				hrow[j] = Bh(i,j);
//				vrow[j] = Bv(i,j);
//			}
//			Pattern pat = categorizeBrow(hrow, vrow);
//			mPatterns[i] = filterPatternChange(i, mPatterns[i], pat);
//		}
//		removeDoublePatterns();
	}

//	void removeDoublePatterns() {
//		std::list<Pattern> to_remove;
//		for (int i = 0; i < 4; ++i) {
//			Pattern p = static_cast<Pattern>(i);
//			if (std::count(mPatterns.begin(), mPatterns.end(), p) > 1) {
//				to_remove.push_front(p);
//			}
//		}
//		for(auto it = to_remove.begin(); it != to_remove.end(); ++it) {
//			for (int i = 0; i < 3; ++i) {
//				if (mPatterns[i] == (*it)) {
//					mPatterns[i] = UnknownPattern;
//				}
//			}
// 		}
//	}

//	Pattern filterPatternChange(int index, Pattern old, Pattern curr) {
//		if (curr != UnknownPattern) {
//			mPatternsLastKnown[index] = mLastRound;
//		}
//
//
//		if (old == UnknownPattern)
//			return curr;
//		if (curr == UnknownPattern) {
//			cout << "Previously known pattern now unknown. Duck: " << mNumber << endl;
//			if (mLastRound - mPatternsLastKnown[index] > cRoundsUnknownDiscardPattern) {
//				cout << "Discarding old value, since its too long not known." << endl;
//				return UnknownPattern;
//			}
//			return old;
//		}
//		if (old != curr) {
//			cout << "changing pattern!                                     !" << endl;
//		}
//		return curr;
//	}

//	std::list<Pattern> knownPatterns() {
//		std::list<Pattern> pats;
//		for (int i = 0; i < 3; ++i) {
//			if(mPatterns[i] != UnknownPattern) {
//				pats.push_back(mPatterns[3]);
//			}
//		}
//		return pats;
//	}

//	Pattern missingPattern() {
//		std::list<Pattern> patterns(gAllPatterns);
//		for (int i = 0; i < 3; ++i) {
//			patterns.remove(mPatterns[i]);
//		}
//		if(patterns.size() == 1) {
//			return patterns.front();
//		} else {
//			return UnknownPattern;
//		}
//	}

	hmm_t& getModel() {
		return mModel;
	}

	hmm_fixed_t& getFixedModel() {
		return mFixedModel;
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
				expected_rewards[obs] = probHit * getGroupReward(getGroup()) + (1.0 - probHit) * cMissReward;
			}
		}

		auto bestIter = std::max_element(expected_rewards.begin(), expected_rewards.end());
		if (pBestObs) {
			*pBestObs = bestIter - expected_rewards.begin();
		}

		return *bestIter;
	}

//	prob currDistance(DuckInfo &other) {
//		//cout <<  "foo: " << std::setprecision(15) << mModel.simple_distance(other.mModel.getModel());
//		return fabs(mModel.kullback_leibler_distance_sample(other.mModel));
//	}

//	const std::vector<Pattern>& getPatterns() {
//		return mPatterns;
//	}

	void setSpecies(ESpecies spec) {
		mSpecies = spec;
	}

	ESpecies getSpecies() {
		return mSpecies;
	}

	void maybeUpdateSpecies(ESpecies spec) {
		if (getSpecies() == SPECIES_UNKNOWN) {
			setSpecies(spec);
		}
	}

	void printWarnings() {
		mModel.print_warnings();
		mFixedModel.print_warnings();
	}

	static hmm_t::state_obs_trans_t getMissingPatternObservationMatrix(Pattern notPattern);
	static hmm_fixed_t::state_obs_trans_t getFullObservationMatrix();

	Pattern filterNewFixedModelMissingPattern(Pattern newPattern);
	void fixedModelUpdateMissingPattern();

	Pattern getFixedModelMissingPattern() {
		return mFixedModelMissingPattern;
	}

	bool getFixedModelMissingPatternCertain() {
		return mFixedModelMissingPatternCertain;
	}

	int getEastWestBalance() {
		return mNumEast - mNumWest;
	}

	Group getGroup();

	double getGroupReward(Group group) {
		switch(group) {
		case UnknownGroup:
			return 4; // FIXME
		case WhiteGroup:
			return cWhiteReward;
		case BlackGroup:
			return cBlackReward;
		case ColorGroup:
			return cColorReward;
		}
	}

private:

	CPlayer *mPlayer;
	const CDuck *mDuck;
	int mNumber;
	hmm_t mModel;
	hmm_fixed_t mFixedModel;
	Pattern mFixedModelMissingPattern;
	int mFixedModelMissingPatternLastKnown;
	bool mFixedModelMissingPatternCertain;


	std::vector<DuckObservation> mObs;
	int mLastRound;

//	std::vector<Pattern> mPatterns;
//	std::vector<int> mPatternsLastKnown;

	bool mPracticeMode;

	int mNumUp;
	int mNumDown;
	int mNumVStopped;
	int mNumWest;
	int mNumEast;
	int mNumHStopped;
	int mRoundOfDeath;


	ESpecies mSpecies;
};


}


#endif /* DUCKINFO_H_ */
