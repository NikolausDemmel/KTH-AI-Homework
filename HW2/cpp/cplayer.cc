#include "cplayer.h"
#include <cstdlib>
#include <iostream>

using namespace std;

namespace chk
{

CPlayer::CPlayer()
{
}

bool CPlayer::Idle(const CBoard &pBoard)
{
    return false;
}

void CPlayer::Initialize(bool pFirst,const CTime &pDue)
{
    srand(CTime::GetCurrent().Get());
}
    
CMove CPlayer::Play(const CBoard &pBoard,const CTime &pDue)
{
    //Use the commented version if your system supports ANSI color (linux does)
    pBoard.Print();
    //pBoard.PrintNoColor();

    std::vector<CMove> lMoves;
    pBoard.FindPossibleMoves(lMoves,CELL_OWN);
    /*
     * Here you should write your clever algorithms to get the best next move.
     * This skeleton returns a random movement instead.
     */

    for(std::vector<CMove>::iterator it = lMoves.begin(); it != lMoves.end(); ++it) {
    	cout << it->ToString() << endl;
    }

    return lMoves[rand()%lMoves.size()];
}

/*namespace chk*/ }
