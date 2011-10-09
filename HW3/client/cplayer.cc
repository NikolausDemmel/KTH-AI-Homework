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

	mTimeouts = 0;

	mGroups.resize(5);

	mDuckInfo.resize(mNumDucks);
	for (int i = 0; i < mNumDucks; ++i) {
		mDuckInfo[i].initialize(&(mState->GetDuck(i)), i, this, isPractice());
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
		mTimeouts++;
	}

	cout << "Chosen action with expected utility: " << bestReward << endl;
	action.Print();

	FormGroups();

	// FIXME: Decide

	return action;


//	fstream stream("./B-examples.csv", fstream::out);
//	stream << std::fixed;
//	cout << "numbirds: " << mDuckInfo.size() << endl;
//
//	for (int i = 0; i < mDuckInfo.size(); ++i) {
//		if (mDuckInfo[i].missingPattern() != UnknownPattern)
//		mDuckInfo[i].getModel().write_B_to_stream(stream, patternToString(mDuckInfo[i].getPatterns()));
//	}
}

void CPlayer::FormGroups() {
	for (int i = 0; i < 5; ++i) {
		mGroups[i].clear();
		mWhitePattern = UnknownPattern;
		mBlackPattern = UnknownPattern;
	}


	int countCertain = 0;
	int countKnown = 0;
	for (int i = 0; i < mDuckInfo.size(); ++i) {

		Pattern pat = mDuckInfo[i].getFixedModelMissingPattern();

		if( mDuckInfo[i].getFixedModelMissingPatternCertain() ) {
			if (mDuckInfo[i].isDead() ) {
				mGroups[pat].dead_certain++;
			} else {
				mGroups[pat].alive_certain++;
			}
			countCertain++;
		} else if ( pat != UnknownPattern ) {
			countKnown++;
		}

		if (mDuckInfo[i].isDead() ) {
			mGroups[pat].dead_ducks.push_back(i);
			int balance = mDuckInfo[i].getEastWestBalance();
			mGroups[pat].dead_east_west_balance += balance;
			if (balance > 0) {
				mGroups[pat].dead_east += 1;
			} else {
				mGroups[pat].dead_west += 1;
			}
		} else {
			mGroups[pat].alive_ducks.push_back(i);
			int balance = mDuckInfo[i].getEastWestBalance();
			mGroups[pat].alive_east_west_balance += balance;
			if (balance > 0) {
				mGroups[pat].alive_east += 1;
			} else {
				mGroups[pat].alive_west += 1;
			}
		}

	}
	cout << countCertain << " Patterns certainly identified." << endl;
	cout << countKnown <<   " Patterns maybe identified." << endl;


	std::vector<pair<int,Pattern>> sizes;
	for (int i = 0; i < 4; ++i) {
		pair<int,Pattern> p(mGroups[i].get_total_count(),(Pattern)i);
		sizes.push_back(p);
	}
	sort(sizes.begin(), sizes.end());
	cout << "Sorted sizes: ";
	for (int i = 0; i < 4; ++i) {
		cout << sizes[i].first << "(" << sizes[i].second << "), ";
	}
	cout << endl;

	mGroups[sizes[0].second].group = BlackGroup;
	mGroups[sizes[1].second].group = ColorGroup;
	mGroups[sizes[2].second].group = ColorGroup;
	mGroups[sizes[3].second].group = WhiteGroup;

	if( ! (sizes[0].first < 4 && sizes[1].first > 10) ) {
		mGroups[sizes[0].second].group = UnknownGroup;
		for (int i = 1; i < 4; ++i) {
			if (sizes[i].first <= 10 + sizes[0].first )
				mGroups[sizes[0].second].group = UnknownGroup;
		}
	}


	cout << "groups:" << endl;
	for (int i = 0; i < 5; ++i) {
		cout << groupToString(mGroups[i].group) << ", ";
	}
	cout << endl;

	cout << "size of groups" << endl;
	cout << setw(10) << "size:";
	for (int i = 0; i < 5; ++i) {
		int size = mGroups[i].get_total_count();
		cout << setw(4) << size << ", ";
//		if (size == 1 && !mBlackFound) {
//			if (mGroups[i].alive_ducks.empty() )
//				mDuckInfo[mGroups[i].[0]].maybeUpdateSpecies(SPECIES_BLACK);
//			else
//
//			mBlackFound = true;
//		}
	}
	cout << endl;
	cout << setw(10) << "certain:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].get_total_certain() << ", ";
	}
	cout << endl;
	cout << setw(10) << "balance:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].get_total_east_west_balance() << ", ";
	}
	cout << endl;
	cout << setw(10) << "east:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].get_total_east() << ", ";
	}
	cout << endl;
	cout << setw(10) << "west:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].get_total_west() << ", ";
	}
	cout << endl;
	cout << "size of alive groups" << endl;
	cout << setw(10) << "size:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].alive_ducks.size() << ", ";
	}
	cout << endl;
	cout << setw(10) << "certain:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].alive_certain << ", ";
	}
	cout << endl;
	cout << setw(10) << "balance:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].alive_east_west_balance << ", ";
	}
	cout << endl;
	cout << setw(10) << "east:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].alive_east << ", ";
	}
	cout << endl;
	cout << setw(10) << "west:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].alive_west << ", ";
	}
	cout << endl;
	cout << "size of dead groups" << endl;
	cout << setw(10) << "size:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].dead_ducks.size() << ", ";
	}
	cout << endl;
	cout << setw(10) << "certain:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].dead_certain << ", ";
	}
	cout << endl;
	cout << setw(10) << "balance:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].dead_east_west_balance << ", ";
	}
	cout << endl;
	cout << setw(10) << "east:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].dead_east << ", ";
	}
	cout << endl;
	cout << setw(10) << "west:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(4) << mGroups[i].dead_west << ", ";
	}
	cout << endl;


//	for (int i = 0; i < 5; ++i) {
//		int size = mGroups[i].alive_ducks.size();
//
//		if (size < 10) {
//			cout << "small group " << i << endl;
//			foreach( int d, mGroups[i].alive_ducks ) {
//				mDuckInfo[d].getFixedModel().printA();
//			}
//		}
//	}
}




void CPlayer::Guess(std::vector<CDuck> &pDucks,const CTime &pDue)
{
	for (int i = 0; i < mDuckInfo.size(); ++i) {
		mDuckInfo[i].printWarnings();
	}

	if (mTimeouts > 0) {
		cout << "[warning] timeouts: " << mTimeouts << endl;
	}

	if(isPractice()) {
		PracticeModeGuess(pDucks, pDue);
		return;
	}

#ifdef DEBUG
	cout << "GUESSING" << endl;
#endif



//	mDuckInfo[0].getModel().printState();
//	mDuckInfo[1].getModel().printState();
//
//	mDuckInfo[0].getModel().print_B_split(patternToString(mDuckInfo[0].getPatterns()));
//	mDuckInfo[1].getModel().print_B_split(patternToString(mDuckInfo[1].getPatterns()));
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
	mDuckInfo[pDuck].setSpecies(pSpecies);
	// TODO
    std::cout << "HIT DUCK!!!\n";
}










void CPlayer::PracticeModeGuess(std::vector<CDuck> &pDucks, const CTime &pDue) {
	cout << "GUESS" << endl;
	// mDuckInfo[0].getModel().printState();
}


CAction CPlayer::PracticeModeShoot(const CState &pState,const CTime &pDue)
{
	cout << "PRACTICE MODE" << endl;

	Initialize(pState);

	mRound = pState.GetDuck(0).GetSeqLength();

	try
	{
		EnableTimer(pDue);

		mDuckInfo[0].iteration(mRound);
		mDuckInfo[0].update();

		DisableTimer();
	}
	catch(std::exception &e) {
		cout << "Exception: " << e.what() << endl;
	}

#ifdef DEBUG
	mDuckInfo[0].getModel().printState();
	mDuckInfo[0].getFixedModel().printState();

	cout << "logProb: " << mDuckInfo[0].getModel().sumLogScaleFactors() << endl;
	cout << "logProbFixed: " << mDuckInfo[0].getFixedModel().sumLogScaleFactors() << endl;
#endif

	return mDuckInfo[0].getModel().predictNextObs().toAction();
}



/*namespace ducks*/ }
