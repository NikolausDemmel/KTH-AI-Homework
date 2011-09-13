#include "cplayer.h"
#include <cstdlib>
#include <signal.h>
#include <sys/time.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace chk
{

bool gTimeout = false;

void timeout_handler (int i) {
	gTimeout = true;
}

void check_timeout() {
	if(gTimeout) {
		throw timeout_exception();
	}
}

CPlayer::CPlayer()
{
}

void CPlayer::DisableTimer() {
	setitimer(ITIMER_REAL, 0, 0);
}

void CPlayer::EnableTimer(const CTime &pDue) {
	struct itimerval diff;

	CTime::GetCurrent().ToITimevalUntil(pDue, diff);
	gTimeout = false;

	setitimer(ITIMER_REAL, &diff, 0);
}

bool CPlayer::Idle(const CBoard &pBoard)
{
    return false;
}

void CPlayer::Initialize(bool pFirst,const CTime &pDue)
{
	signal(SIGALRM,timeout_handler);
    srand(CTime::GetCurrent().Get());
}
    
CMove CPlayer::Play(const CBoard &pBoard,const CTime &pDue)
{
#ifdef INFO
	cout << endl << "### NEXT ROUND ###" << endl << endl;

    pBoard.Print();

    std::vector<CMove> lMoves;
    pBoard.FindPossibleMoves(lMoves);
#endif

#ifdef INFO
    cout << "Board value: " << pBoard.Evaluate(lMoves) << endl;
#endif

#ifdef DEBUG
    cout << "Possible moves:" << endl;
    for(std::vector<CMove>::iterator it = lMoves.begin(); it != lMoves.end(); ++it) {
    	cout << it->ToString() << endl;
    }
#endif

#ifdef DEBUG
    cout << "Max move history score: " << mMoveHistory.MaxScore() << endl;
#endif

    // mMoveHistory.DampScores(2); // FIXME: bit twiddeling about how much to damp

    const int ultimateDepthLimit = 1000;
    pair<CMove,bool> result;

    EnableTimer(pDue);

    try {
    	// NOTE: possible variation: increase 2 ply at a time.
    	for(mMaxDepth = 1; mMaxDepth <= ultimateDepthLimit; mMaxDepth += 1) {
#ifdef INFO
    		cout << "                     	Searching depth " << mMaxDepth << endl;
#endif
    		result = AlphaBetaSearch(pBoard);
    		if (! result.second)
    			break;
    	}
    	DisableTimer();
    } catch(exception &e) {
#ifdef DEBUG
    	cout << "Exception: " << e.what() << endl;
#endif
    }

    return result.first;

    //return lMoves[rand()%lMoves.size()];
}

bool CPlayer::CutoffTest(const CBoard &pBoard, const std::vector<CMove> &pMoves, int depth) const {
	if (pBoard.GameOver(pMoves))
		return true;
	if (depth >= mMaxDepth)
		return true;
	return false;
}

pair<CMove,bool> CPlayer::AlphaBetaSearch(const CBoard &pBoard)
{
#ifdef DEBUG
	mNumberOfBoards = 0;
#endif

    vector<CMove> lMoves;
    pBoard.FindPossibleMoves(lMoves);

    if (lMoves.size() == 1) {
    	return pair<CMove,bool>(lMoves[0], false);
    }

    float v = -INFINITY;
    CMove m = NullMove;

    // FIXME: call MaxValue really, and add history ordering this way.
    for(vector<CMove>::iterator iter = lMoves.begin(); iter != lMoves.end(); ++iter) {
    	float vcurr = MinValue(CBoard(pBoard, *iter), v, INFINITY, 0);
#ifdef DEBUG
    	cout << "Move " << iter->ToString() << " has value " << vcurr << endl;
#endif
    	if (vcurr > v) {
    		v = vcurr;
    		m = *iter;
    	}
    }

#ifdef DEBUG
    cout << "Number of Boards looked at: " << mNumberOfBoards << endl;
#endif

    return pair<CMove, bool>(m, (v == 1.0 || v == 0.0) ? false : true); // don't search on if we know we will win or loose.
}

float CPlayer::MaxValue(const CBoard &pBoard, float a, float b, int depth)
{
	check_timeout();

#ifdef DEBUG
	++mNumberOfBoards;
#endif

	vector<CMove> lMoves;
	pBoard.FindPossibleMoves(lMoves);

	if (lMoves.size() == 1) {
		return MinValue(CBoard(pBoard,lMoves[0]), a, b, depth);
	}

	if (CutoffTest(pBoard, lMoves, depth)) {
		return pBoard.Evaluate(lMoves);
	}

	OrderMoves(lMoves);

	float v = -INFINITY;
    CMove m = NullMove;

    for(vector<CMove>::iterator iter = lMoves.begin(); iter != lMoves.end(); ++iter) {
    	float vcurr = MinValue(CBoard(pBoard, *iter), a, b, depth+1);

    	if (vcurr > v) {
    		v = vcurr;
    		m = *iter;
    	}
    	if (v >= b) {
    		RecordSufficientMove(*iter,depth);
    		return v;
    	}
    	a = max(a,v);
    }

	RecordSufficientMove(m,depth);
    return v;
}

float CPlayer::MinValue(const CBoard &pBoard, float a, float b, int depth)
{
	check_timeout();

#ifdef DEBUG
	++mNumberOfBoards;
#endif

	vector<CMove> lMoves;
	pBoard.FindPossibleMoves(lMoves);

	if (lMoves.size() == 1) {
		return MaxValue(CBoard(pBoard,lMoves[0]), a, b, depth);
	}

	if (CutoffTest(pBoard, lMoves, depth)) {
		return pBoard.Evaluate(lMoves);
	}

	OrderMoves(lMoves);

	float v = INFINITY;
    CMove m = NullMove;

    for(vector<CMove>::iterator iter = lMoves.begin(); iter != lMoves.end(); ++iter) {
    	float vcurr = MaxValue(CBoard(pBoard, *iter), a, b, depth+1);
    	if (vcurr < v) {
    		v = vcurr;
    		m = *iter;
    	}
    	if (v <= a) {
    		RecordSufficientMove(*iter,depth);
    		return v;
    	}
    	b = min(b,v);
    }

	RecordSufficientMove(m,depth);
    return v;
}

void CPlayer::OrderMoves(vector<CMove> &moves)
{
	// sort by history for now, maybe factor in value of jumps.
	sort(moves.begin(), moves.end(), mMoveHistory.mCompareMoves);
}

void CPlayer::RecordSufficientMove(const CMove &move, int curr_depth)
{
	int subtree_depth = mMaxDepth - curr_depth;
	// FIXME: find out best value.
	//        1<<depth has been suggested, but then we need to worry about overflow.
	//		  depth*depth, or 1 would also be possible
	int score = subtree_depth*subtree_depth;// 1<<subtree_depth;
	mMoveHistory.Increase(move, score);
}

/*namespace chk*/ }
