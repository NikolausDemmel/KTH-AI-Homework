#include "cplayer.h"
#include "duckobservation.h"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace ducks
{

CPlayer::CPlayer()
{
	cout.precision(5);
}

CAction CPlayer::Shoot(const CState &pState,const CTime &pDue)
{
	HMM<3,9, DuckObservation> duck_model(DuckObservation::getSplitNames());

	const CDuck &duck = pState.GetDuck(0);

	int T = duck.GetSeqLength();
	cout << "T = " << T << endl;

	std::vector<DuckObservation> obs(T);
	for (int t = 0; t < T; ++t) {
		obs[t] = DuckObservation(duck.GetAction(t));
	}

	duck_model.printState();

	duck_model.setObservations(obs);
	duck_model.learnModel();

	duck_model.printState();



    //this line doesn't shoot any bird
    return cDontShoot;

    //this line would predict that bird 0 is totally stopped and shoot at it
    //return CAction(0,ACTION_STOP,ACTION_STOP,BIRD_STOPPED);
}

void CPlayer::Guess(std::vector<CDuck> &pDucks,const CTime &pDue)
{
    /*
     * Here you should write your clever algorithms to guess the species of each alive bird.
     * This skeleton guesses that all of them are white... they were the most likely after all!
     */
     
    for(int i=0;i<pDucks.size();i++)
    {
         if(pDucks[i].IsAlive())
             pDucks[i].SetSpecies(SPECIES_WHITE);
    }
}

void CPlayer::Hit(int pDuck,ESpecies pSpecies)
{
    std::cout << "HIT DUCK!!!\n";
}

/*namespace ducks*/ }
