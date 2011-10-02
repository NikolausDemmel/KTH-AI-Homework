#ifndef _DUCKS_CPLAYER_H_
#define _DUCKS_CPLAYER_H_

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

    ///guess the species!

    ///This function will be called at the end of the game, to give you
    ///a chance to identify the species of the surviving ducks for extra
    ///points.
    ///
    ///For each alive duck in the vector, you must call the SetSpecies function,
    ///passing one of the ESpecies constants as a parameter
    ///\param pDucks the vector of all ducks. You must identify only the ones that are alive
    ///\param pDue time before which we must have returned
    void Guess(std::vector<CDuck> &pDucks,const CTime &pDue);

    ///This function will be called whenever you hit a duck.
    ///\param pDuck the duck index
    ///\param pSpecies the species of the duck (it will also be set for this duck in pState from now on)    
    void Hit(int pDuck,ESpecies pSpecies);

    void DisableTimer();
    void EnableTimer(const CTime &pDue);

private:

    std::vector<DuckInfo> mDuckInfo;
    const CState *mState;
    bool mFirstTime;
    int mNumDucks;
    int mRound;

};

/*namespace ducks*/ }

#endif
