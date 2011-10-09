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
		group(UnknownGroup),
		alive_rel_balance(0),
		dead_rel_balance(0),
		best_reward(-999),
		uncertain_best_reward(-999),
		black_count(0),
		could_be_black_count(0)
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
		alive_rel_balance = 0;
		dead_rel_balance = 0;
		best_reward = -999;
		uncertain_best_reward = -999;
		black_count = 0;
		could_be_black_count = 0;
	}

	int get_total_count() {
		return alive_ducks.size() + dead_ducks.size();
	}

	int get_total_certain() {
		return alive_certain + dead_certain;
	}

	double get_total_east_west_balance() {
		if ( get_total_count() == 0 )
			return 0;
		return (alive_east_west_balance * alive_ducks.size() + dead_east_west_balance * dead_ducks.size()) /
				get_total_count();
	}

	double get_total_rel_balance() {
		if ( get_total_count() == 0 )
			return 0;
		return (alive_rel_balance * alive_ducks.size() + dead_rel_balance * dead_ducks.size()) /
				get_total_count();
	}

	int get_total_west() {
		return alive_west + dead_west;
	}

	int get_total_east() {
		return alive_east + dead_east;
	}

	void update_best_reward(double curr) {
		best_reward = std::max(curr, best_reward);
	}

	void update_uncertain_best_reward(double curr) {
		uncertain_best_reward = std::max(curr, uncertain_best_reward);
	}

	int alive_certain;
	int dead_certain;
	std::vector<int> alive_ducks;
	std::vector<int> dead_ducks;
	double alive_east_west_balance;
	double dead_east_west_balance;
	double alive_rel_balance;
	double dead_rel_balance;
	int alive_west;
	int alive_east;
	int dead_west;
	int dead_east;
	Group group;

	double best_reward;
	double uncertain_best_reward;
	int black_count;
	int could_be_black_count;
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

    void PrintWarnings();
    void PrintGroupType();
    void PrintGroupMissingPattern();
    void PrintGroupInfo();
    void PrintRewardInfo();

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
    bool mHitBlack;
    bool mBlackDuckMaybeWronglyIdentified;
	int mBlackFoundCount;

    friend class DuckInfo;
};

/*namespace ducks*/ }

#endif
