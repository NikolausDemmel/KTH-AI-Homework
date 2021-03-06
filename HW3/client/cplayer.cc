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
	try {

	mFirstTime = true;
	cout.precision(5);
	cout << std::fixed;
	signal(SIGALRM,timeout_handler);

	} catch (...) {
		cout << "EXCEPTION (in CPlayer)" << endl;
	}
}

void CPlayer::Initialize(const CState &pState)
{
#ifdef DEBUG
	cout << "INITIALIZATION" << endl;
#endif

	mBlackFound = false;
	mBlackFoundCount = 0;
	mBlackDuckMaybeWronglyIdentified = false;
	mHitBlack = false;
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

    try {

	if(isPractice(&pState))
		return PracticeModeShoot(pState, pDue);

	if (mFirstTime == true)
		Initialize(pState);


	int startShooting = 20; // TODO: find optimal value
	if(isSingleplayer()) {
		startShooting = 20;
	}


	int newTurns = mState->GetNumNewTurns();
	mRound += newTurns;
#ifdef DEBUG
	cout << "\t\t\t\t\t\t\tROUND: " << mRound << endl;
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

		// UPDATE HMMs
		for (int i = 0; i < mState->GetNumDucks(); ++i) {
			mDuckInfo[i].update();
		}

		// ANALYSE AND CATEGORIZE
		FormGroups();

		// ESTIMATE REWARDS OF SHOOTING
		for (int i = 0; i < mState->GetNumDucks(); ++i) {

			if ( mDuckInfo[i].isDead() )
				continue;
			if ( mDuckInfo[i].getSpecies() == SPECIES_BLACK ) {
				mGroups[mDuckInfo[i].mMissingPattern].black_count++;
				continue;
			}
			if ( mDuckInfo[i].couldBeBlack() ) {
				mGroups[mDuckInfo[i].mMissingPattern].could_be_black_count++;
				continue;
			}

			int obs;
			double expRew = mDuckInfo[i].expectedReward(&obs);
			if (expRew > bestReward) {
				bestReward = expRew;
				bestDuck = i;
				bestObs = obs;
			}

			mGroups[mDuckInfo[i].mMissingPattern].update_best_reward(expRew);
			if(((mDuckInfo[i].mMissingPattern != UnknownPattern ) && (!mDuckInfo[i].mMissingPatternCertain)) ||
			   ((mDuckInfo[i].mMissingPattern == UnknownPattern ) && (mDuckInfo[i].mMigratingUnlikeBlack ||
					   	   	   	   	   	   	   	   	   	   	   	   	  mDuckInfo[i].mFeigningDeathUnlikeBlack))) {
				mGroups[mDuckInfo[i].mMissingPattern].update_uncertain_best_reward(expRew);
			}
		}


		// MAKE ACTION FOR BEST DUCK TO SHOOT
		if (bestDuck >= 0) {
			action = mDuckInfo[bestDuck].makeAction(bestObs);
		}

		DisableTimer();
	}
	catch(std::exception &e) {
		cout << "\t\t\t\t\tException: " << e.what() << endl;
		cerr << "\t\t\t\t\tException: " << e.what() << endl;
		mTimeouts++;
		return action;
	}

	cout << endl << "Reward info" << endl;
	PrintRewardInfo();
	cout << "Current reward of unknown ducks: " << mDuckInfo[0].getUnknownReward() << endl;
	cout << endl;

	cout << "Chosen to shoot duck " << bestDuck << " with expected utility: " << bestReward << endl;
	if (bestDuck >= 0) {
	cout << "This duck is " << (mDuckInfo[bestDuck].getMissingPatternCertain() ? "probably" : "maybe")
	     << " missing pattern " << patternToString(mDuckInfo[bestDuck].getMissingPattern()) << endl;
	}
	action.Print();

	return action;

    } catch (char const * e) {
    	cout << "EXCEPTION (in Shoot)" << e << endl;
    }

    return cDontShoot;
}

void CPlayer::FormGroups() {

	// RESET

	for (int i = 0; i < 5; ++i) {
		mGroups[i].clear();
		mWhitePattern = UnknownPattern;
		mBlackPattern = UnknownPattern;
	}
	for (int i = 0; i < mDuckInfo.size(); ++i) {
		mDuckInfo[i].mMigratingUnlikeBlack = false;
		mDuckInfo[i].mFeigningDeathUnlikeBlack = false;
	}



	// RECALCULATE


	int countCertain = 0;
	int countKnown = 0;
	for (int i = 0; i < mDuckInfo.size(); ++i) {

		Pattern pat = mDuckInfo[i].getMissingPattern();

		if( mDuckInfo[i].getMissingPatternCertain() ) {
			if (mDuckInfo[i].isDead() ) {
				mGroups[pat].certain_count[Dead]++;
			} else {
				mGroups[pat].certain_count[Alive]++;
			}
			countCertain++;
		} else if ( pat != UnknownPattern ) {
			countKnown++;
		}

		int ew_balance = mDuckInfo[i].getEastWestBalance();
		int ud_balance = mDuckInfo[i].getUpDownBalance();
		Aliveness al = (mDuckInfo[i].isDead() ? Dead : Alive);

		mGroups[pat].direction_balance[al][EastWest] += ew_balance;
		mGroups[pat].direction_balance[al][UpDown] += ud_balance;
		mGroups[pat].direction_rel_balance[al][EastWest] += mDuckInfo[i].getRelativeEastWestBalance();
		mGroups[pat].direction_rel_balance[al][UpDown] += mDuckInfo[i].getRelativeUpDownBalance();
		mGroups[pat].ducks[al].push_back(i);
		if (ew_balance > 0) {
			mGroups[pat].directions[al][East] += 1;
		} else {
			mGroups[pat].directions[al][West] += 1;
		}
		if (ud_balance > 0) {
			mGroups[pat].directions[al][Up] += 1;
		} else {
			mGroups[pat].directions[al][Down] += 1;
		}

	}
	cout << countCertain << " Patterns certainly identified." << endl;
	cout << countKnown <<   " Patterns maybe identified." << endl;

	for (int i = 0; i < 5; ++i) {
		foreach (Aliveness al, cAlivenesses) {
			if ( mGroups[i].ducks[al].size() > 0) {
				foreach (Axis axis, cAxes) {
					mGroups[i].direction_balance[al][axis] /= mGroups[i].ducks[al].size();
					mGroups[i].direction_rel_balance[al][axis] /= mGroups[i].ducks[al].size();
				}
			}
		}
	}


	std::vector<pair<int,Pattern>> sizes;
	for (int i = 0; i < 4; ++i) {
		pair<int,Pattern> p(mGroups[i].get_total_count(),(Pattern)i);
		sizes.push_back(p);
	}
	sort(sizes.begin(), sizes.end());

	mGroups[sizes[0].second].group = BlackGroup;
	mGroups[sizes[1].second].group = ColorGroup;
	mGroups[sizes[2].second].group = ColorGroup;
	mGroups[sizes[3].second].group = WhiteGroup;

	if( ! (sizes[0].first < 4 && sizes[1].first > 10) ) {
		mGroups[sizes[0].second].group = UnknownGroup;
		for (int i = 1; i < 4; ++i) {
			if (sizes[i].first <= 10 + sizes[0].first )
				mGroups[sizes[i].second].group = UnknownGroup;
		}
	}

	for (int i = 0; i < 4; ++i) {
		if(mGroups[i].group == BlackGroup)
			mBlackPattern = (Pattern) i;
		if(mGroups[i].group == WhiteGroup)
			mWhitePattern = (Pattern) i;
	}


	PrintGroupInfo();

	if (mRound > 300) {
		// Can we pin down the black duck?
		if (mBlackPattern != UnknownPattern) {
			// If we play against a good player, he will not shoot birds in the black group,
			// so we assume there is no dead ducks here and the black one is still out there.
			// If its not, we simply play it safe. If the opponent has shot the black bird,
			// we win anyway, so no need to detect this situation.
			if (mGroups[mBlackPattern].ducks[Alive].size() == 1 &&
			    mGroups[mBlackPattern].certain_count[Alive] == 1) {
				// mDuckInfo[mGroups[mBlackPattern].alive_ducks[0]].setSpecies(SPECIES_BLACK);
				mBlackFoundCount += 1;
				mBlackFound = true;
			} else {
				// For now lets not go back here, but instead issue a big fat warning
				// mBlackFound = false;
				if (mBlackFound && !mHitBlack) {
//					cout << " FOOOOO: we thought we know the black duck, but now we don't no more.";
					mBlackDuckMaybeWronglyIdentified = true;
					mBlackFoundCount -= 2;
					if (mBlackFoundCount < 0) {
						mBlackFound = false;
						mBlackFoundCount = 0;
					}
				}
			}
		}

		// even if we can't pin down the black duck, we can maybe identify some ducks that are definitely not it.
		if (!mBlackFound) {
			if (mBlackPattern == Migrating) {
				// we know that the black bird does not migrate, so look for birds that seem to be migrating

				foreach(int d, mGroups[UnknownPattern].ducks[Alive]) {
					int total = mDuckInfo[d].getEastWestBalance();
					// when this bird is not migrating, a difference of 100 in
					// west vs east movements would be a huge coincidence!
					// Could do some more analysis and scale the 100 up when
					// for higher turns, but probably not worth it
					if (abs(total) > 100) {
						mDuckInfo[d].mMigratingUnlikeBlack = true;
					}
				}
			} else if (mBlackPattern == FeigningDeath) {
				// TODO: like Migrating, but be more careful since panicking might lead to big variance
				foreach(int d, mGroups[UnknownPattern].ducks[Alive]) {
					int total = mDuckInfo[d].getUpDownBalance();
					if (total < -100) {
						mDuckInfo[d].mFeigningDeathUnlikeBlack = true;
					}
				}
			}
		}
	}


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

void CPlayer::PrintGroupType() {
	cout << setw(10) << " ";
	for (int i = 0; i < 5; ++i) {
		cout << setw(10) << groupToString(mGroups[i].group) << ", ";
	}
	cout << endl;
}

void CPlayer::PrintGroupMissingPattern() {
	cout << setw(10) << " ";
	for (int i = 0; i < 5; ++i) {
		cout << setw(10) << patternToString((Pattern)i) << ", ";
	}
	cout << endl;
}

void CPlayer::PrintRewardInfo() {
	PrintGroupType();

	cout << setw(10) << "black#:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(10) << mGroups[i].black_count << ", ";
	}
	cout << endl;

	cout << setw(10) << "cld b bk#:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(10) << mGroups[i].could_be_black_count << ", ";
	}
	cout << endl;

	cout << setw(10) << "best rwrd:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(10) << mGroups[i].best_reward << ", ";
	}
	cout << endl;
	cout << setw(10) << "ucrt rwrd:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(10) << mGroups[i].uncertain_best_reward << ", ";
	}
	cout << endl;
}

void CPlayer::PrintGroupInfo() {

	cout << "Have we identified the black bird? " << (mBlackFound ? "Yes" : "No") << endl;
	cout << "Black bird found count: " << mBlackFoundCount << endl;

	PrintGroupMissingPattern();


	foreach(Aliveness aliveness, cAlivenesses) {
		cout << "groups " << cAlivenessNames[aliveness] << endl;
		PrintGroupType();
		cout << setw(10) << "size:";
		for (int i = 0; i < 5; ++i) {
			cout << setw(10) << mGroups[i].ducks[aliveness].size() << ", ";
		}
		cout << endl;
		cout << setw(10) << "certain:";
		for (int i = 0; i < 5; ++i) {
			cout << setw(10) << mGroups[i].certain_count[aliveness] << ", ";
		}
		cout << endl;
		foreach(Axis axis, cAxes) {
			cout << setw(10) << (string(cAxisNames[axis]) + "-bal:");
			for (int i = 0; i < 5; ++i) {
				cout << setw(10) << mGroups[i].direction_balance[aliveness][axis] << ", ";
			}
			cout << endl;
			cout << setw(10) << (string(cAxisNames[axis]) + "-r-bal:");
			for (int i = 0; i < 5; ++i) {
				cout << setw(10) << mGroups[i].direction_rel_balance[aliveness][axis] << ", ";
			}
			cout << endl;
		}
		foreach(Direction dir, cDirections) {
			cout << setw(9) << cDirectionNames[dir] << ":";
			for (int i = 0; i < 5; ++i) {
				cout << setw(10) << mGroups[i].directions[aliveness][dir] << ", ";
			}
			cout << endl;
		}
	}

	cout << "groups total" << endl;
	PrintGroupType();
	cout << setw(10) << "size:";
	for (int i = 0; i < 5; ++i) {
		int size = mGroups[i].get_total_count();
		cout << setw(10) << size << ", ";
	}
	cout << endl;
	cout << setw(10) << "certain:";
	for (int i = 0; i < 5; ++i) {
		cout << setw(10) << mGroups[i].get_total_certain() << ", ";
	}
	cout << endl;

	foreach(Axis axis, cAxes) {
		cout << setw(10) << (string(cAxisNames[axis]) + "-bal:");
		for (int i = 0; i < 5; ++i) {
			cout << setw(10) << mGroups[i].get_total_balance(axis) << ", ";
		}
		cout << endl;
		cout << setw(10) << (string(cAxisNames[axis]) + "-r-bal:");
		for (int i = 0; i < 5; ++i) {
			cout << setw(10) << mGroups[i].get_total_rel_balance(axis) << ", ";
		}
		cout << endl;
	}
	foreach(Direction dir, cDirections) {
		cout << setw(9) << cDirectionNames[dir] << ":";
		for (int i = 0; i < 5; ++i) {
			cout << setw(10) << mGroups[i].get_total_direction(dir) << ", ";
		}
		cout << endl;
	}



	cout << "known colors" << endl;
	for(int x = -1; x < 6; ++x) {
		ESpecies s = (ESpecies)x;
		cout << setw(10) << speciesToString(s);
		for (int i = 0; i < 5; ++i) {
			int count = 0;
			foreach (int n, mGroups[i].ducks[Dead]) {
				if (mState->GetDuck(n).GetSpecies() == s)
					++count;
			}
			cout << setw(10) << count << ", ";
		}
		cout << endl;
	}
}




void CPlayer::PrintWarnings() {
	for (int i = 0; i < mDuckInfo.size(); ++i) {
		mDuckInfo[i].printWarnings();
	}

	if (mTimeouts > 0) {
		cout << "[warning] timeouts: " << mTimeouts << endl;
	}

	if (mHitBlack) {
		cout << "[warning] we hit the black duck..." << endl;
	}

	if (mBlackDuckMaybeWronglyIdentified) {
		cout << "[warning] we identified black duck, but later weren't sure no more..." << endl;
	}
}

void CPlayer::Guess(std::vector<CDuck> &pDucks,const CTime &pDue)
{
#ifdef DEBUG
	cout << "GUESSING" << endl;
	cout.flush();
#endif

	try {

	PrintWarnings();

	if(isPractice()) {
		PracticeModeGuess(pDucks, pDue);
		return;
	}



    for(int i=0;i<pDucks.size();i++)
    {
         if(pDucks[i].IsAlive()) {
             pDucks[i].SetSpecies(SPECIES_BLACK);
         }
    }

	} catch (...) {
		cout << "EXCEPTION (in Guess)" << endl;
	}
}

void CPlayer::Hit(int pDuck,ESpecies pSpecies)
{
	if(isPractice()) {
		return;
	}

	try {

	mDuckInfo[pDuck].setSpecies(pSpecies);

	if (pSpecies == SPECIES_BLACK) {
		std::cout << "FOOOOOOOOO......................... HIT BLACK DUCK!!!\n";
		std::cout << "FOOOOOOOOO......................... HIT BLACK DUCK!!!\n";
		mHitBlack = true;
	}
	else
		std::cout << "HIT DUCK!!! Color: " << speciesToString(pSpecies) << endl;

    } catch (...) {
    	cout << " (in Hit) " << endl;
    }

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
		cerr << "Exception: " << e.what() << endl;
		cout << "Exception: " << e.what() << endl;
	}

#ifdef DEBUG
	mDuckInfo[0].getModel().printState();
//	mDuckInfo[0].getFixedModel().printState();

	cout << "logProb: " << mDuckInfo[0].getModel().sumLogScaleFactors() << endl;
//	cout << "logProbFixed: " << mDuckInfo[0].getFixedModel().sumLogScaleFactors() << endl;
#endif

	PrintWarnings();

	return mDuckInfo[0].getModel().predictNextObs().toAction();
}



/*namespace ducks*/ }
