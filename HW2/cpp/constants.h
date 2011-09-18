#ifndef _CHECKERS_CONSTANTS_H_
#define _CHECKERS_CONSTANTS_H_

#include <limits>

#define DEBUG
#define INFO
//#define LINEAR_EVAL
#define EXTEND_FORCE_MOVE


namespace chk {

#ifdef LINEAR_EVAL
typedef int eval_t;
#else
typedef float eval_t;
#endif

const float Infinity = std::numeric_limits<eval_t>::max();

///this enumeration is used as the contents of squares in CBoard.
///the CELL_OWN and CELL_OTHER constants are also used to refer
///to this and the other player
enum ECell
{
    CELL_EMPTY=0,			///< the cell is empty
    CELL_OWN=(1<<0),		///< the cell belongs to us (the one playing)
    CELL_OTHER=(1<<1),		///< the cell belongs to the other player
    CELL_KING=(1<<2),		///< the cell is a king
    CELL_INVALID=(1<<3)		///< the cell is invalid
};


/*namespace chk*/ }

#endif
