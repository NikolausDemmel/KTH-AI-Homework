#include "cplayer.h"
#include "duckobservation.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <fstream>

namespace ducks
{



CPlayer::CPlayer()
{
	mFirstTime = true;
	cout.precision(5);
	cout << std::fixed;
	signal(SIGALRM,timeout_handler);
}

void CPlayer::Initialize(const CState &pState)
{
#ifdef DEBUG
	cout << "INITIALIZATION" << endl;
#endif

	mBlackFound = false;
	mRound = 0;
	mFirstTime = false;
	mState = &pState;
	mNumDucks = mState->GetNumDucks();

	mGroups.resize(5);

	mDuckInfo.resize(mNumDucks);
	for (int i = 0; i < mNumDucks; ++i) {
		mDuckInfo[i].setDuck(&(mState->GetDuck(i)));
		mDuckInfo[i].setNumber(i);
		mDuckInfo[i].setPlayer(this);
	}
}

void CPlayer::notifyBlackBirdFound() {
	mBlackFound = true;
}

bool CPlayer::isBlackBirdFound() {
	return mBlackFound;
}

bool CPlayer::isSingleplayer() {
	return (!isPractice()) && mState->GetNumPlayers() == 1;
}

bool CPlayer::isPractice(const CState *state) {
	return (state?state:mState)->GetNumDucks() == 1;
}


void CPlayer::DisableTimer() {
	setitimer(ITIMER_REAL, 0, 0);
}

void CPlayer::EnableTimer(const CTime &pDue) {
	struct itimerval diff;

	ITimevalUntil(CTime::GetCurrent(), pDue, diff);
	reset_timeout_flag();

	setitimer(ITIMER_REAL, &diff, 0);
}

CAction CPlayer::Shoot(const CState &pState,const CTime &pDue)
{
	if(isPractice(&pState))
		return PracticeModeShoot(pState, pDue);

	if (mFirstTime == true)
		Initialize(pState);


	int startShooting = 50; // FIXME !!!
	if(isSingleplayer()) {
		startShooting = 50; // FIXME !!!!
	}


	int newTurns = mState->GetNumNewTurns();
	mRound += newTurns;
#ifdef DEBUG
	cout << "ROUND: " << mRound << endl;
#endif

	for (int i = 0; i < mState->GetNumDucks(); ++i) {
		mDuckInfo[i].iteration(mRound);
	}

	if (mRound < startShooting) {
		cout << "Too little info. Don't shoot" << endl;
		return cDontShoot;
	}

	CAction action = cDontShoot;
	double bestReward = 0;
	int bestDuck = -1;
	int bestObs = 0;

	try
	{
		EnableTimer(pDue);

		for (int i = 0; i < mState->GetNumDucks(); ++i) {
			mDuckInfo[i].update();
		}

		for (int i = 0; i < mState->GetNumDucks(); ++i) {

			if ( mDuckInfo[i].isDead() || mDuckInfo[i].couldBeBlack() ||
				 mDuckInfo[i].getSpecies() == SPECIES_BLACK )
				continue;

			int obs;
			double expRew = mDuckInfo[i].expectedReward(&obs);
			if (expRew > bestReward) {
				bestReward = expRew;
				bestDuck = i;
				bestObs = obs;
			}
		}

		if (bestDuck >= 0) {
			action = mDuckInfo[bestDuck].makeAction(bestObs);
		}

		DisableTimer();
	}
	catch(std::exception &e) {
		cout << "Exception: " << e.what() << endl;
	}

	cout << "Chosen action with expected utility: " << bestReward << endl;
	action.Print();

	for (int i = 0; i < 5; ++i) {
		mGroups[i].clear();
	}
	int count = 0;
	for (int i = 0; i < mDuckInfo.size(); ++i) {
		if( UnknownPattern != mDuckInfo[i].missingPattern()) {
			count++;
		}
		mGroups[mDuckInfo[i].missingPattern()].push_back(i);
	}
	cout << count << " Patterns completely identified." << endl;

	cout << "size of groups";
	for (int i = 0; i < 5; ++i) {
		int size = mGroups[i].size();
		cout << mGroups[i].size() << ", ";
		if (size == 1 && !mBlackFound) {
			mDuckInfo[mGroups[i][0]].maybeUpdateSpecies(SPECIES_BLACK);
			mBlackFound = true;
		}
	}
	cout << endl;


	// FIXME
	return cDontShoot;
	return action;
}






void CPlayer::Guess(std::vector<CDuck> &pDucks,const CTime &pDue)
{
	for (int i = 1; i < mDuckInfo.size(); ++i) {
		mDuckInfo[i].getModel().print_warnings();
	}

	if(isPractice()) {
		PracticeModeGuess(pDucks, pDue);
		return;
	}

#ifdef DEBUG
	cout << "GUESSING" << endl;
#endif

	mDuckInfo[0].getModel().printState();
	mDuckInfo[1].getModel().printState();

	mDuckInfo[0].getModel().print_B_split(patternToString(mDuckInfo[0].getPatterns()));
	mDuckInfo[1].getModel().print_B_split(patternToString(mDuckInfo[1].getPatterns()));
//	mDuckInfo[2].getModel().print_B_split(patternToString(mDuckInfo[2].getPatterns()));
//	mDuckInfo[3].getModel().print_B_split(patternToString(mDuckInfo[3].getPatterns()));
//	mDuckInfo[4].getModel().print_B_split(patternToString(mDuckInfo[4].getPatterns()));

	// mDuckInfo[0].analyseDevelopment();

//	for (int i = 0; i < pDucks.size(); ++i) {
//		cout << "dist to   " << i << ": " << std::setprecision(10) << mDuckInfo[0].currDistance(mDuckInfo[i]) << endl;
//		cout << "dist from " << i << ": " << std::setprecision(10) << mDuckInfo[i].currDistance(mDuckInfo[0]) << endl;
//	}

//	std::fstream out("distances.csv", std::fstream::out);

//	for (int i = 0; i < pDucks.size(); ++i) {
//		for (int j = 0; j < pDucks.size(); ++j) {
//			out << std::setprecision(10) << mDuckInfo[i].currDistance(mDuckInfo[j]);
//			if (j < pDucks.size() - 1)
//				out << ";";
//		}
//		out << endl;
//	}
     
    for(int i=0;i<pDucks.size();i++)
    {
         if(pDucks[i].IsAlive()) {
             pDucks[i].SetSpecies(SPECIES_WHITE);
         }
    }
}

void CPlayer::Hit(int pDuck,ESpecies pSpecies)
{
    std::cout << "HIT DUCK!!!\n";
}










void CPlayer::PracticeModeGuess(std::vector<CDuck> &pDucks, const CTime &pDue) {
	mDuckInfo[0].getModel().printState();
}


CAction CPlayer::PracticeModeShoot(const CState &pState,const CTime &pDue)
{
	cout << "PRACTICE MODE" << endl;

	HMM<3,DuckObservation::numObservations, DuckObservation> duck_model(DuckObservation::getSplitNames());
	const CDuck &duck = pState.GetDuck(0);
	int T = duck.GetSeqLength();

#ifdef DEBUG
	cout << "T = " << T << endl;
#endif

	std::vector<DuckObservation> obs;
	for (int t = 0; t < T; ++t) {
		obs.push_back(DuckObservation(duck.GetAction(t)));
	}
	duck_model.setObservations(&obs);

	try {
		EnableTimer(pDue);
		duck_model.learnModel(200, 200, true, false);
		DisableTimer();
	}
	catch (timeout_exception &e) {
		cout << e.what() << endl;
	}

#ifdef DEBUG
	duck_model.printState();
#endif

	return duck_model.predictNextObs().toAction();
}



/*namespace ducks*/ }
