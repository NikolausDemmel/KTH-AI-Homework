#include "cplayer.h"
#include "duckobservation.h"
#include <cstdlib>
#include <iostream>
#include <vector>

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

	mRound = 0;
	mFirstTime = false;
	mState = &pState;
	mNumDucks = mState->GetNumDucks();

	mDuckInfo.resize(mNumDucks);
	for (int i = 0; i < mNumDucks; ++i) {
		mDuckInfo[i].setDuck(&(mState->GetDuck(i)));
	}
}


void CPlayer::DisableTimer() {
	setitimer(ITIMER_REAL, 0, 0);
}

void CPlayer::EnableTimer(const CTime &pDue) {
	struct itimerval diff;

	ITimevalUntil(CTime::GetCurrent(), pDue, diff);
	gTimeout = false;

	setitimer(ITIMER_REAL, &diff, 0);
}

CAction CPlayer::Shoot(const CState &pState,const CTime &pDue)
{
	if(pState.GetNumDucks() == 1)
		return PracticeModeShoot(pState, pDue);


	if (mFirstTime == true)
		Initialize(pState);



	int newTurns = mState->GetNumNewTurns();
	mRound += newTurns;
#ifdef DEBUG
	cout << "ROUND: " << mRound << endl;
#endif

	if (mRound < 100) {
		cout << "Too little info. Don't shoot" << endl;
		return cDontShoot;
	}

	try
	{
		EnableTimer(pDue);

		bool verbose = true;
		for (int i = 0; i < mState->GetNumDucks(); ++i) {
			if (i == 5) verbose = false;
			mDuckInfo[i].update(verbose);
		}
		mDuckInfo[0].getModel().printState();
		mDuckInfo[1].getModel().printState();

		DisableTimer();
	}
	catch(std::exception &e) {
		cout << "Exception: " << e.what() << endl;
	}

	cout << "Failsafe: Don't shoot." << endl;
	return cDontShoot;
}

CAction CPlayer::PracticeModeShoot(const CState &pState,const CTime &pDue)
{
	cout << "PRACTICE MODE" << endl << endl;

	HMM<3,9, DuckObservation> duck_model(DuckObservation::getSplitNames());
	const CDuck &duck = pState.GetDuck(0);
	int T = duck.GetSeqLength();

#ifdef DEBUG
	cout << "T = " << T << endl;
#endif

	std::vector<DuckObservation> obs;
	for (int t = 300; t < T; ++t) {
		obs.push_back(DuckObservation(duck.GetAction(t)));
	}
	duck_model.setObservations(&obs);

	try {
		EnableTimer(pDue);
		duck_model.learnModel(200, 	true);
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


void CPlayer::Guess(std::vector<CDuck> &pDucks,const CTime &pDue)
{
    /*
     * Here you should write your clever algorithms to guess the species of each alive bird.
     * This skeleton guesses that all of them are white... they were the most likely after all!
     */

#ifdef DEBUG
	cout << "GUESSING" << endl;
#endif

	//mDuckInfo[0].getModel().printState();
	//mDuckInfo[1].getModel().printState();
     
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

/*namespace ducks*/ }
