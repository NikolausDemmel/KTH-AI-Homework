// Gui Checkers by Jonathan Kreuzer
// copyright 2005, 2009

// =================================================
// I use my own bitboard representation detailed in movegen.cpp
// =================================================

#define PD 0
const int KV = KING_VAL;
const int KBoard[32] = { 
	KV-3,  KV+0,  KV+0,  KV+0,
	    KV+0,  KV+1,  KV+1,  KV+0,
	 KV+1,  KV+3,  KV+3,  KV+2,
	    KV+3,  KV+5,  KV+5,  KV+1,
	 KV+1,  KV+5,  KV+5,  KV+3,
	    KV+2,  KV+3,  KV+3,  KV+1,
	 KV+0,  KV+1,  KV+1,  KV+0,
	    KV+0,  KV+0,  KV+0, KV-3
};
							
struct CBoard 
{
	void StartPosition( int bResetRep );
	void Clear();
	void SetFlags();
	int EvaluateBoard( int ahead, int alpha, int beta );

	void ToFen( char *sFEN );
	int FromFen( char *sFEN );
	int FromPDN( char *sPDN );
	int ToPDN( char *sPDN );

	bool InDatabase( const SDatabaseInfo& dbInfo )
	{
		return (dbInfo.loaded && nWhite <= dbInfo.numWhite && nBlack <= dbInfo.numBlack );
	}

	int MakeMovePDN( int src, int dst);
	int DoMove( SMove &Move, int nType);
	void DoSingleJump( int src, int dst, int nPiece);
	void CheckKing( int src, int dst, int nPiece);
	void UpdateSqTable( int nPiece, int src, int dst );

	int inline GetPiece( int sq ) const
	{
		if ( S[sq] & C.BP )
		{
			return ( S[sq] & C.K ) ? BKING : BPIECE;
		}
		if ( S[sq] & C.WP )
		{
			return ( S[sq] & C.K ) ? WKING : WPIECE;
		}
		if ( sq >= 32 )
			return INVALID;

		return EMPTY;
	}
	void SetPiece( int sq, int piece )
	{	
		// Clear square first
		C.WP &= ~S[sq];
		C.BP &= ~S[sq];
		C.K &= ~S[sq];

		// Set the piece
		if ( piece & WPIECE )
			C.WP |= S[sq];
		if ( piece & BPIECE )
			C.BP |= S[sq];
		if ( piece & KING )
			C.K |= S[sq];
	}

	// Data
	SCheckerBoard C;
	char nWhite, nBlack, SideToMove, extra;
	short nPSq, eval;
	unsigned __int64 HashKey;
};

CBoard g_Boardlist[MAX_SEARCHDEPTH + 1];
CBoard g_CBoard;
CBoard g_StartBoard;

// Convert from 64 to 32 board
int BoardLoc[66] = 
	{	INV, 28, INV, 29, INV, 30, INV, 31,	24, INV, 25, INV, 26, INV, 27, INV, INV, 20, INV, 21, INV, 22, INV, 23,	16, INV, 17, INV, 18, INV, 19, INV, 
		INV, 12, INV, 13, INV, 14, INV, 15,	8, INV, 9, INV, 10, INV, 11, INV, INV, 4, INV, 5, INV, 6, INV, 7,	0, INV, 1, INV, 2, INV, 3, INV};


// =================================================
//
//	 			TRANSPOSITION TABLE 
//
// =================================================
unsigned char g_ucAge = 0;

struct TEntry
{
	// FUNCTIONS
	public:
	void inline Read( unsigned long CheckSum, short alpha, short beta, int &bestmove, int &value, int depth, int ahead)
		{
		if (m_checksum == CheckSum ) //To be almost totally sure these are really the same position.  
			{
			int tempVal;
			// Get the Value if the search was deep enough, and bounds usable
			if (m_depth >= depth)
               {
			   if (abs(m_eval) > 1800) // This is a game ending value, must adjust it since it depends on the variable ahead
					{
					if (m_eval > 0) tempVal = m_eval - ahead + m_ahead;
					if (m_eval < 0) tempVal = m_eval + ahead - m_ahead;
					}
			   else tempVal = m_eval;
			   switch ( m_failtype )
					{
					case 0: value = tempVal;  // Exact value
						break; 
					case 1: if (tempVal <= alpha) value = tempVal; // Alpha Bound (check to make sure it's usuable)
						break;
					case 2: if (tempVal >= beta) value = tempVal; //  Beta Bound (check to make sure it's usuable)
						break;
					}
               }
			// Otherwise take the best move from Transposition Table                                                
			bestmove = m_bestmove; 
			}
		}

	void inline Write( unsigned long CheckSum, short alpha, short beta, int &bestmove, int &value, int depth, int ahead)
	{
		if (m_age == g_ucAge && m_depth > depth && m_depth > 14) return; // Don't write over deeper entries from same search
		m_checksum = CheckSum;
		m_eval = value;
		m_ahead = ahead;
		m_depth = depth;
		m_bestmove = bestmove;
		m_age = g_ucAge;
		if (value <= alpha) m_failtype = 1;
		else if (value >= beta)  m_failtype = 2;
		else m_failtype = 0;
	}

	static void Create_HashFunction();
	static unsigned __int64 HashBoard( const CBoard &Board );

	// DATA
	private:
		unsigned long m_checksum;
		short m_eval;
		short m_bestmove;
		char m_depth;
		char m_failtype, m_ahead;
		unsigned char m_age;
};

// Declare and allocate the transposition table
TEntry *TTable = (TEntry *) malloc( sizeof (TEntry) * HASHTABLESIZE + 4);

// ------------------
//  Hash Board
//
//  HashBoard is called to get the hash value of the start board.
// ------------------
unsigned __int64 HashFunction[32][12], HashSTM;

void TEntry::Create_HashFunction( )
{
	// FIXME : This function is used for the opening book
	for (int i = 0; i <32; i++)
        for (int x = 0; x < 9; x++)
		{
			HashFunction[i][x]  = rand() + (rand()*256) + (rand()*65536);
			HashFunction[i][x] <<= 32;
			HashFunction[i][x]  += rand() + (rand()*256) + (rand()*65536);
		}
	HashSTM = HashFunction[0][0];
}       

unsigned __int64 TEntry::HashBoard( const CBoard &Board )
{
	unsigned __int64 CheckSum = 0;

    for (int index = 0; index < 32; index++)
	{
		int nPiece = Board.GetPiece( index );
		if (nPiece != EMPTY) CheckSum ^= HashFunction [ index ][ nPiece ];
    }
    if (Board.SideToMove == BLACK) 
	{
		CheckSum ^= HashSTM;
	}
	return CheckSum;
}

// Check for a repeated board 
// (only checks every other move, because side to move must be the same)
__int64 RepNum[ MAX_GAMEMOVES ];

// with Hashvalue passed
int inline Repetition( const __int64 HashKey, int nStart, int ahead)
{
	int i;
	if (nStart > 0) i = nStart; else i = 0;
	
	if ((i&1) != (ahead&1)) i++;

	ahead-=2;
    for ( ; i < ahead; i+=2)
		 if (RepNum[i] == HashKey) return TRUE;
                
    return FALSE;
}

void inline AddRepBoard( const __int64 HashKey, int ahead )
{
	RepNum[ ahead ] = HashKey;
}

// =================================================
//
//	 			BOARD FUNCTIONS
//
// =================================================

void CBoard::Clear( )
{
	C.BP = 0;
	C.WP = 0;
	C.K = 0;
	SetFlags();
}

void CBoard::StartPosition( int bResetRep )
{
	Clear();
	C.BP = MASK_1ST | MASK_2ND | MASK_3RD;
	C.WP = MASK_6TH | MASK_7TH | MASK_8TH;

	SideToMove = BLACK;
	if (bResetRep) g_numMoves = 0;

	SetFlags();
	g_StartBoard = *this;
}

void CBoard::SetFlags( )
{
	nWhite = 0; 
	nBlack = 0; 
	nPSq = 0;
	C.empty = ~(C.WP | C.BP);
	for (int i = 0; i < 32; i++) 
	{
		int piece = GetPiece( i );
		if ( (piece&WHITE)) 
		{
			nWhite++;
			if (piece == WKING) {nPSq += KBoard[i];} 
		}
		if ( (piece&BLACK) ) 
		{
			nBlack++;
			if (piece == BKING) {nPSq -= KBoard[i];} 
		}
	}
	HashKey = TEntry::HashBoard( *this );
}

// ------------------
//  Evaluate Board
//
//  I don't know much about checkers, this is the best I could do for an evaluation function
// ------------------
const int LAZY_EVAL_MARGIN = 64;

int CBoard::EvaluateBoard (int ahead, int alpha, int beta)
{
	// Game is over?        
	if (C.WP == 0) return -2001 + ahead;
	if (C.BP == 0) return 2001 - ahead;
	//

	if ( g_dbInfo.type == DB_EXACT_VALUES && InDatabase( g_dbInfo ) ) 
	{
		int value = QueryEdsDatabase( *this, ahead );

		if ( value != INVALID_VALUE )
		{
			// If we get an exact score from the database we want to return right away
			databaseNodes++;
			return value;
		}
	 }
	
	int eval;

	// Evaluate number of pieces present. Scaled higher for less material
	if ((nWhite+nBlack) > 12 ) 
		eval = (nWhite-nBlack) * 100;
	else 
		eval = (nWhite-nBlack) * (160 - (nWhite + nBlack) * 5 );

	eval+=nPSq;	

	// Probe the W/L/D bitbase
	if ( g_dbInfo.type == DB_WIN_LOSS_DRAW && InDatabase( g_dbInfo ) ) 
	{
		int Result = QueryGuiDatabase( *this );
		if (Result <= 2 )
			databaseNodes++;
		if (Result == 2) eval+=400;
		if (Result == 1) eval-=400;
		if (Result == 0) return 0;
	} 
	else 
	{
		// surely winning advantage? note : < 8 is to not overflow the eval
		if (nWhite == 1 && nBlack >= 3 && nBlack < 8) 
			eval += (eval>>1);
		if (nBlack == 1 && nWhite >= 3 && nWhite < 8)
			eval += (eval>>1);
	}

	// Too far from the alpha beta window? Forget about the rest of the eval, it probably won't get value back within the window
	if (eval+LAZY_EVAL_MARGIN < alpha) return eval;
	if (eval-LAZY_EVAL_MARGIN > beta) return eval;

	
	// Keeping checkers off edges
	eval -= 2 * (BitCount(C.WP & ~C.K & MASK_EDGES) - BitCount(C.BP & ~C.K & MASK_EDGES) );

	// Mobility
	// eval += ( C.GetWhiteMoves() - C.GetBlackMoves() ) * 2;

	// The losing side can make it tough to win the game by moving a King back and forth on the double corner squares.
	int WK = C.WP & C.K;
	int BK = C.BP & C.K;

	if (eval > 18)
		{if ((BK & MASK_2CORNER1)) {eval-=8; if ((MASK_2CORNER1 & ~C.BP &~C.WP)) eval-=10;}
		 if ((BK & MASK_2CORNER2)) {eval-=8; if ((MASK_2CORNER2 & ~C.BP &~C.WP)) eval-=10;}
		}
	if (eval < -18)
		{if ((WK & MASK_2CORNER1)) {eval+=8; if ((MASK_2CORNER1 & ~C.BP &~C.WP)) eval+=10;}
		 if ((WK & MASK_2CORNER2)) {eval+=8; if ((MASK_2CORNER2 & ~C.BP &~C.WP)) eval+=10;}
		}

	int nWK = BitCount( WK );
	int nBK = BitCount( BK );
	int WPP = C.WP & ~C.K;
	int BPP = C.BP & ~C.K;
	// Advancing checkers in endgame
	if ( (nWK*2) >= nWhite || (nBK*2) >= nBlack || (nBK+nWK)>=4) 
	{
		int Mul;
		if ( nWK == nBK && (nWhite+nBlack) < (nWK+nBK+2) ) Mul = 8; else Mul = 2;
		eval -=  Mul * ( BitCount( (WPP & MASK_TOP) ) - BitCount( (WPP & MASK_BOTTOM) )
					  -BitCount( (BPP & MASK_BOTTOM) ) + BitCount( (BPP & MASK_TOP) ) );
	}

	static int BackRowGuardW[8] = { 0, 4, 5, 13, 4, 20, 18, 25 };
	static int BackRowGuardB[8] = { 0, 4, 5, 20, 4, 18, 13, 25 };
	int nBackB = 0, nBackW = 0;
	if ( (nWK*2) < nWhite ) 
	{
		nBackB = (( BPP ) >> 1) & 7;
		eval -= BackRowGuardB[ nBackB ];
	}
	if ( (nBK*2) < nBlack )
	{
		nBackW = (( WPP ) >> 28) & 7;
		eval += BackRowGuardW[ nBackW ];
	}

	// Number of Active Kings
	int nAWK = nWK;
	int nABK = nBK;
	// Kings trapped on back row
	if ( WK & MASK_TRAPPED_W )
	{
		if ( (S[3]&WK) && (S[7]&~C.empty) && (S[2]&BPP) && (S[11]&C.empty)) {eval-=16; nAWK--;}
		if ( (S[2]&WK) && (S[10]&C.empty) && (S[1]&BPP) && (S[3]&BPP)) {eval-=14; nAWK--;}
		if ( (S[1]&WK) && (S[9]&C.empty) && (S[0]&BPP) && (S[2]&BPP)) {eval-=14; nAWK--;}
		if ( (S[4]&WK) && (S[1]&BPP) && (S[8]&~C.empty) ) {eval-=10; nAWK--;}
		if ( (S[0]&WK) && (S[1]&BPP) ) {eval-=22; nAWK--; if (S[8] & ~C.empty) eval+=7; }
	}
	if ( BK & MASK_TRAPPED_B ) 
	{
		if ( (S[28]&BK) && (S[24]&~C.empty) && (S[29]&WPP) && (S[20]&C.empty)) {eval+=16; nABK--;}
		if ( (S[29]&BK) && (S[21]&C.empty) && (S[30]&WPP) && (S[28]&WPP)) {eval+=14; nABK--;}
		if ( (S[30]&BK) && (S[22]&C.empty) && (S[31]&WPP) && (S[29]&WPP)) {eval+=14; nABK--;}
		if ( (S[27]&BK) && (S[30]&WPP) && (S[23]&~C.empty) ) {eval+=10; nABK--;}
		if ( (S[31]&BK) && (S[30]&WPP) ) {eval+=22; nABK--; if (S[23] & ~C.empty) eval-=7; }
	}

	// Reward checkers that will king on the next move
	int BNearKing = 0;
	if ( (BPP & MASK_7TH) )
	{
		// Checker can king next move
		if ( C.CanBlackCheckerMove( BPP & MASK_7TH ) )
		{
			eval -= KING_VAL - 5;
			BNearKing = 1;
		}
	}

	// Opponent has no Active Kings and backrow is still protected, so give a bonus
	if (nAWK > 0 && !BNearKing && nABK == 0) 
	{
		eval+=20;
		if (nAWK > 1) eval+=10;
		if (BackRowGuardW[ nBackW ] > 10) eval += 15;
		if (BackRowGuardW[ nBackW ] > 20) eval += 20;
	}

	int WNearKing = 0;
	if ( (WPP & MASK_2ND) ) 
	{
		// Checker can king next move
		if ( C.CanWhiteCheckerMove( WPP & MASK_2ND ) )
		{
			eval += KING_VAL - 5;
			WNearKing = 1;
		}
	} 

	if (nABK > 0 && !WNearKing && nAWK == 0)
	{
		eval-=20;
		if (nABK > 1) eval-=10;
		if (BackRowGuardB[ nBackB ] > 10) eval -= 15;
		if (BackRowGuardB[ nBackB ] > 20) eval -= 20;
	}

	if (nWhite == nBlack ) // equal number of pieces, but one side has all kinged versus all but one kinged (or all kings)
	{// score should be about equal, unless it's in a database, then don't reduce
		if (eval > 0 && eval < 200  && nWK >= nBK && nBK >= nWhite-1) eval = (eval>>1) + (eval>>3);
		if (eval < 0 && eval > -200 && nBK >= nWK && nWK >= nBlack-1) eval = (eval>>1) + (eval>>3);
	}

	return eval;
}

//
// MOVE EXECUTION
//
// ---------------------
//  Helper Functions for DoMove 
// ---------------------
const UINT MASK_ODD_ROW = MASK_1ST | MASK_3RD | MASK_5TH | MASK_7TH;

void inline CBoard::DoSingleJump( int src, int dst, const int nPiece )
{
	int jumpedSq = ((dst + src) >> 1 );
	if ( S[jumpedSq] & MASK_ODD_ROW ) jumpedSq += 1; // correct for square number since the jumpedSq sometimes up 1 sometimes down 1 of the average

	int jumpedPiece = GetPiece( jumpedSq );

	// Update Piece Count
    if ( jumpedPiece == WPIECE ) {nWhite--; }
        else if ( jumpedPiece == BPIECE) {nBlack--; }
        else if ( jumpedPiece == BKING)  {nBlack--; nPSq += KBoard[jumpedSq]; }
        else if ( jumpedPiece == WKING)  {nWhite--; nPSq -= KBoard[jumpedSq]; }

	// Update Hash Key
	HashKey ^= HashFunction[ src ][ nPiece ] ^ HashFunction[ dst ][ nPiece ] ^ HashFunction[ jumpedSq ][ jumpedPiece ];

	// Update the bitboards
	UINT BitMove = (S[ src ] | S[ dst ]);
	if (SideToMove == BLACK) {
		C.WP ^= BitMove; 
		C.BP ^= S[jumpedSq];
		C.K  &= ~S[jumpedSq];
	} else {
		C.BP ^= BitMove;
		C.WP ^= S[jumpedSq];
		C.K  &= ~S[jumpedSq];
	}
	C.empty = ~(C.WP|C.BP);
	if (nPiece & KING) C.K ^= BitMove;
}

// This function will test if a checker needs to be upgraded to a king, and upgrade if necessary
void inline CBoard::CheckKing (const int src, const int dst, int const nPiece)
{
	if ( dst <= 3 ) 
	{
		nPSq += KBoard[dst];
		HashKey  ^= HashFunction[ dst ][ nPiece ] ^ HashFunction[ dst ][ WKING ];
		C.K |= S[ dst ];
	}
	if ( dst >= 28 ) 
	{
		nPSq -= KBoard[dst];
		HashKey ^= HashFunction[ dst ][ nPiece ] ^ HashFunction[ dst ][ BKING ];
		C.K |= S[ dst ];
	}
}

void inline CBoard::UpdateSqTable( const int nPiece, const int src, const int dst )
{
	switch (nPiece) {
		case BKING: nPSq -= KBoard[ dst ] - KBoard[ src ]; return;
		case WKING: nPSq += KBoard[ dst ] - KBoard[ src ]; return;
	}
}

// ---------------------
//  Execute a Move 
// ---------------------
int CBoard::DoMove( SMove &Move, int nType )
{
	unsigned int src = (Move.SrcDst&63);
	unsigned int dst = (Move.SrcDst>>6)&63;
	unsigned int bJump = (Move.SrcDst>>12);
	unsigned int nPiece = GetPiece(src); // FIXME : AVOID GET PIECE HERE

	SideToMove ^=3;
	HashKey ^= HashSTM; // Flip hash stm
		
	if (bJump == 0) 
	{
		// Update the bitboards
		UINT BitMove = S[ src ] | S[ dst ];
		if (SideToMove == BLACK) C.WP ^= BitMove; else C.BP ^= BitMove;
		if (nPiece & KING) C.K ^= BitMove;
		C.empty ^= BitMove;

		// Update hash values
		HashKey ^= HashFunction[ src ][ nPiece ] ^ HashFunction[ dst ][ nPiece ];
		
		UpdateSqTable( nPiece, src, dst );
		if (nPiece < KING)	{
			 CheckKing( src, dst, nPiece );
			 return 1;
		}
		return 0;
	}

	DoSingleJump ( src, dst, nPiece);
	if (nPiece < KING) 
		CheckKing( src, dst, nPiece );

	// Double Jump?
	if (Move.cPath[0] == 33 ) 
	{
		UpdateSqTable ( nPiece, src, dst );
		return 1;
	}
	if (nType == HUMAN) return DOUBLEJUMP;

	for (int i = 0; i < 8; i++)
	{
		int nDst = Move.cPath[i];

		if ( nDst == 33 ) break;

		if (nType == MAKEMOVE) // pause a bit on displaying double jumps
		{
			DrawBoard ( *this );
			Sleep (300);
		}
		DoSingleJump( dst, nDst, nPiece);
		dst = nDst;
	}

	if (nPiece < KING) 
		CheckKing( src, dst, nPiece );
	else 
		UpdateSqTable( nPiece, src, dst );

    return 1;
}

// Flip square horizontally because the internal board is flipped.

long FlipX( int x )
{	
	int y = x&3;
	x ^= y;
	x += 3-y;
	return x;
}
// ------------------
// Position Copy & Paste Functions
// ------------------
void CBoard::ToFen ( char *sFEN )
{
	char buffer[80];
	int i;
	if (SideToMove == WHITE) strcpy( sFEN, "W:"); 
		else strcpy( sFEN, "B:");

	strcat( sFEN, "W");
	for (i = 0; i < 32; i++) {
		if ( GetPiece( i ) == WKING) strcat(sFEN, "K");
		if ( GetPiece( i )&WPIECE)  {
			strcat( sFEN, _ltoa (FlipX(i)+1, buffer, 10) ); 
			strcat( sFEN, ","); }
		}
	if (strlen(sFEN) > 3) sFEN [ strlen(sFEN)-1 ] = NULL;
	strcat( sFEN, ":B");
	for (i = 0; i < 32; i++) {
		if ( GetPiece( i ) == BKING) strcat(sFEN, "K");
		if ( GetPiece( i )&BPIECE)  {
			strcat( sFEN, _ltoa (FlipX(i)+1, buffer, 10) ); 
			strcat( sFEN, ","); }
		}
	sFEN[ strlen(sFEN)-1 ] = '.';
}

int CBoard::FromFen ( char *sFEN )
{
	DisplayText( sFEN );
	if ((sFEN[0] != 'W' && sFEN[0]!='B') || sFEN[1]!=':') return 0;
	Clear ();
	if (sFEN[0] == 'W') SideToMove = WHITE;
	if (sFEN[0] == 'B') SideToMove = BLACK;

	int nColor=0, i=0;
	while (sFEN[i] != 0 && sFEN[i]!= '.' && sFEN[i-1]!= '.')
		{
		int nKing = 0;
		if (sFEN[i] == 'W') nColor = WPIECE;
		if (sFEN[i] == 'B') nColor = BPIECE;
		if (sFEN[i] == 'K') {nKing = 4; i++; }
		if (sFEN[i] >= '0' && sFEN[i] <= '9')
			{
			int sq = sFEN[i]-'0';
			i++;
			if (sFEN[i] >= '0' && sFEN[i] <= '9') sq = sq*10 + sFEN[i]-'0';
			SetPiece( FlipX(sq-1), nColor | nKing ); 
			}
		i++;
		}

	SetFlags ( );
	g_StartBoard = *this;
	return 1;
}

// For PDN support
//
int GetFinalDst( SMove &Move ) 
{
	int sq = ((Move.SrcDst>>6)&63);
	if ( (Move.SrcDst>>12) )
		for (int i = 0; i < 8; i++)
		{
			if (Move.cPath[i] == 33) break;
			sq = Move.cPath[i];
		}
	return sq;
}

//
//
int CBoard::MakeMovePDN( int src, int dst)
{
	CMoveList Moves;
	if (SideToMove == BLACK) Moves.FindMovesBlack( C );
	if (SideToMove == WHITE) Moves.FindMovesWhite( C );
	for (int x = 1; x <= Moves.numMoves; x++) 
	{
		int nMove = Moves.Moves[x].SrcDst;
		if ( (nMove&63) == src )
			if ( ((nMove>>6)&63) == dst || GetFinalDst( Moves.Moves[x] ) == dst )
		{
			DoMove ( Moves.Moves[x], SEARCHED );
			g_GameMoves [ g_numMoves++ ] = Moves.Moves[x];
			g_GameMoves [ g_numMoves ].SrcDst = 0;
			return 1;
		}
	}
	return 0;
}

//
//
int CBoard::FromPDN ( char *sPDN )
{
	int i = 0;
	int nEnd = (int)strlen(sPDN);
	int src = 0, dst = 0;
	char sFEN[512];

	StartPosition ( 1 );

	while ( i < nEnd )
		{
		if ( !memcmp (&sPDN[i], "1-0",3) || !memcmp (&sPDN[i], "0-1",3) || !memcmp (&sPDN[i], "1/2-1/2",7) || sPDN[i]=='*')
			break;
		if ( !memcmp (&sPDN[i], "[FEN \"",6))
			{
			i+=6;
			int x = 0;
			while (sPDN[i] !='"' && i < nEnd) sFEN[x++] = sPDN[i++];
			sFEN[x++] = 0;
			FromFen( sFEN );
			}
		if (sPDN[i] == '[') while (sPDN[i] != ']' && i < nEnd) i++;
		if (sPDN[i] == '{') while (sPDN[i] != '}' && i < nEnd) i++;
		if (sPDN[i] >= '0' && sPDN[i] <= '9' && sPDN[i+1] == '.') i++;
		if (sPDN[i] >= '0' && sPDN[i] <= '9')
			{
			int sq = sPDN[i]-'0';
			i++;
			if (sPDN[i] >= '0' && sPDN[i] <= '9') sq = sq*10 + sPDN[i]-'0';
			src = FlipX(sq-1);
			}
		if ((sPDN[i] == '-' || sPDN[i] == 'x')
			&& sPDN[i+1] >= '0' && sPDN[i+1] <= '9')
			{
			i++;
			int sq = sPDN[i]-'0';
			i++;
			if (sPDN[i] >= '0' && sPDN[i] <= '9') sq = sq*10 + sPDN[i]-'0';
			dst = FlipX(sq-1);

			MakeMovePDN ( src, dst);
			}
		i++;
		}
	DisplayText (sPDN);
	return 1;
}

int CBoard::ToPDN ( char *sPDN )
{
	sPDN[0] = 0;
	int i = 0, j = 0;
	char cType;
	j+=sprintf ( sPDN+j, "[Event \"%s game\"]\015\012", g_sNameVer);

	char sFEN[512];
	g_StartBoard.ToFen ( sFEN );
	if ( strcmp ( sFEN, "B:W24,23,22,21,28,27,26,25,32,31,30,29:B4,3,2,1,8,7,6,5,12,11,10,9.") )
		{
		j+=sprintf ( sPDN+j, "[SetUp \"1\"]\015\012" );
		j+=sprintf ( sPDN+j, "[FEN \"%s\"]\015\012", sFEN );
		}

	while (g_GameMoves[i].SrcDst !=0)
		{
		if ((i%2)==0) j+=sprintf ( sPDN+j, "%d. ", (i/2)+1 );
		unsigned int src = (g_GameMoves[i].SrcDst&63);
		unsigned int dst = GetFinalDst( g_GameMoves[i] );
		if ( (g_GameMoves[i].SrcDst>>12) ) cType = 'x'; else cType = '-';
		j+=sprintf ( sPDN+j, "%d%c%d ", FlipX( src )+1, cType, FlipX( dst )+1 );
		i++;
		if ((i%12)==0) j+=sprintf ( sPDN+j, "\015\012" );
		}
	sprintf ( sPDN+j, "*" );
	return 1;
}