#ifndef _DUCKS_CPLAYER_H_
#define _DUCKS_CPLAYER_H_

#include "defines.h"
#include "ctime.h"
#include "cstate.h"
#include "hmm.h"
#include "duckinfo.h"
#include <vector>

namespace ducks
{


struct GroupInfo {

	GroupInfo():
		alive_certain(0),
		dead_certain(0),
		alive_east_west_balance(0),
		dead_east_west_balance(0),
		alive_west(0),
		alive_east(0),
		dead_west(0),
		dead_east(0),
		group(UnknownGroup)
	{
	}

	void clear() {
		alive_certain = 0;
		dead_certain = 0;
		alive_east_west_balance = 0;
		dead_east_west_balance = 0;
		alive_ducks.clear();
		dead_ducks.clear();
		alive_west = 0;
		alive_east = 0;
		dead_west = 0;
		dead_east = 0;
		group = UnknownGroup;
	}

	int get_total_count() {
		return alive_ducks.size() + dead_ducks.size();
	}

	int get_total_certain() {
		return alive_certain + dead_certain;
	}

	int get_total_east_west_balance() {
		return alive_east_west_balance + dead_east_west_balance;
	}

	int get_total_west() {
		return alive_west + dead_west;
	}

	int get_total_east() {
		return alive_east + dead_east;
	}

	int alive_certain;
	int dead_certain;
	std::vector<int> alive_ducks;
	std::vector<int> dead_ducks;
	int alive_east_west_balance;
	int dead_east_west_balance;
	int alive_west;
	int alive_east;
	int dead_west;
	int dead_east;
	Group group;
};


class CPlayer
{
public:
    CPlayer();

    void Initialize(const CState &pState);


    CAction Shoot(const CState &pState,const CTime &pDue);

    CAction PracticeModeShoot(const CState &pState,const CTime &pDue);


    void Guess(std::vector<CDuck> &pDucks,const CTime &pDue);
    void PracticeModeGuess(std::vector<CDuck> &pDucks,const CTime &pDue);

    ///This function will be called whenever you hit a duck.
    void Hit(int pDuck,ESpecies pSpecies);

    void FormGroups();

    void DisableTimer();
    void EnableTimer(const CTime &pDue);

    bool isSingleplayer();
    bool isPractice(const CState *state = 0);

    void notifyBlackBirdFound();
    bool isBlackBirdFound();

    int totalNumberOfSpecies(ESpecies species);

private:

    std::vector<GroupInfo> mGroups;

    Pattern mWhitePattern;
    Pattern mBlackPattern;


    // std::vector<int> mDeadSpeciesCount;

    std::vector<DuckInfo> mDuckInfo;
    const CState *mState;
    bool mFirstTime;
    int mNumDucks;
    int mRound;
    int mTimeouts;

    bool mBlackFound;


    friend class DuckInfo;
};

/*namespace ducks*/ }

#endif
