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

    void DisableTimer();
    void EnableTimer(const CTime &pDue);

    bool isSingleplayer();
    bool isPractice(const CState *state = 0);

    void notifyBlackBirdFound();
    bool isBlackBirdFound();

    int totalNumberOfSpecies(ESpecies species);

private:

    std::vector<std::vector<int>> mGroups;
    std::vector<std::vector<int>> mGroupsDeadBirds;

    std::vector<int> mDeadSpeciesCount;

    std::vector<DuckInfo> mDuckInfo;
    const CState *mState;
    bool mFirstTime;
    int mNumDucks;
    int mRound;

    bool mBlackFound;
};

/*namespace ducks*/ }

#endif
