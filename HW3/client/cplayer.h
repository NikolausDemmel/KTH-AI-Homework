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


enum Direction {
	Up = 0,
	Down,
	East,
	West,
	DirectionCount
};

enum Aliveness {
	Alive = 0,
	Dead,
	AlivenessCount
};

enum Axis {
	UpDown = 0,
	EastWest,
	AxisCount
};



typedef const char * cstring_t;

const Direction cDirections[] = { Up, Down, East, West };
const cstring_t cDirectionNames[] = { "Up", "Down", "East", "West" };

const Aliveness cAlivenesses[] = { Alive, Dead };
const cstring_t cAlivenessNames[] = { "Alive", "Dead" };

const Axis cAxes[] = { UpDown, EastWest };
const cstring_t cAxisNames[] = { "UD", "EW" };



struct GroupInfo {

	GroupInfo()
	{
		clear();
	}

	void clear() {
		foreach (Aliveness al, cAlivenesses) {
			certain_count[al] = 0;
			ducks[al].clear();

			foreach (Axis ax, cAxes) {
				direction_balance[al][ax] = 0;
				direction_rel_balance[al][ax] = 0;
			}

			foreach (Direction dir, cDirections) {
				directions[al][dir] = 0;
			}
		}

		group = UnknownGroup;
		best_reward = -999;
		uncertain_best_reward = -999;
		black_count = 0;
		could_be_black_count = 0;
	}

	int get_total_count() {
		return ducks[Alive].size() + ducks[Dead].size();
	}

	int get_total_certain() {
		return certain_count[Alive] + certain_count[Dead];
	}

	double get_total_balance(Axis axis) {
		if ( get_total_count() == 0 )
			return 0;
		return (direction_balance[Alive][axis] * ducks[Alive].size() +
				direction_balance[Dead][axis] * ducks[Dead].size()) /
			   get_total_count();
	}

	double get_total_rel_balance(Axis axis) {
		if ( get_total_count() == 0 )
			return 0;
		return (direction_rel_balance[Alive][axis] * ducks[Alive].size() +
				direction_rel_balance[Dead][axis] * ducks[Dead].size()) /
			   get_total_count();
	}

	int get_total_direction(Direction dir) {
		return directions[Alive][dir] + directions[Dead][dir];
	}

	void update_best_reward(double curr) {
		best_reward = std::max(curr, best_reward);
	}

	void update_uncertain_best_reward(double curr) {
		uncertain_best_reward = std::max(curr, uncertain_best_reward);
	}

	int certain_count[AlivenessCount];
	std::vector<int> ducks[AlivenessCount];

	double direction_balance[AlivenessCount][AxisCount];
	double direction_rel_balance[AlivenessCount][AxisCount];
	int directions[AlivenessCount][DirectionCount];
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
