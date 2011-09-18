// Gui Checkers 1.05
// by Jonathan Kreuzer
// copyright 2005
// http://www.3dkingdoms.com/checkers.htm
// I compiled with Microsoft Visual C++ Express Edition.

//
// NOTICES & CONDITIONS
//
// Commercial use of this code, in whole or in any part, is prohibited. 
// Non-commercial use is welcome, but you should clearly mention this code, and where it came from.
// www.3dkingdoms.com/checkers.htm
//
// These conditions apply to the code itself only.
// You're welcome to read this code and learn from it without restrictions.


#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <conio.h>
#include <memory.h>
#include <windows.h>
#include "resource.h"

#define WHITE 2
#define BLACK 1 // Black == Red
#define EMPTY 0
#define BPIECE 1
#define WPIECE 2
#define KING 4
#define BKING  5
#define WKING  6
#define INVALID 8
#define NONE 255
#define TIMEOUT 31000
#define DOUBLEJUMP 2000
#define HUMAN 128
#define MAKEMOVE 129
#define SEARCHED 127
#define MAX_SEARCHDEPTH 96
#define MAX_GAMEMOVES 2048
const int BEGINNER_DEPTH = 2, NORMAL_DEPTH = 8, EXPERT_DEPTH = 52; // Max depth for each level
#define KING_VAL 33

// GLOBAL VARIABLES... ugg, too many?
char *g_sNameVer = "Gui Checkers 1.05", *g_sInfo = NULL;
float fMaxSeconds = 2.0f, g_fPanic; // The number of seconds the computer is allowed to search
int g_bEndHard = FALSE; // Set to true to stop search after fMaxSeconds no matter what.
int g_MaxDepth = EXPERT_DEPTH; 
int nodes, nodes2, SearchDepth, g_SelectiveDepth;
char g_cCompColor = WHITE;
clock_t starttime, endtime, lastTime = 0;
int g_SearchingMove = 0, g_SearchEval = 0;   
int g_nDouble = 0, g_nMoves = 0; // Number of moves played so far in the current game
int g_bSetupBoard, g_bThinking = false;
int bCheckerBoard = 0;
int *g_pbPlayNow, g_bStopThinking = 0;
int g_bDatabases = 0;
unsigned int HASHTABLESIZE = 700000; // 700,000 entries * 12 bytes per entry

struct CBoard;
struct SMove;
struct CMoveList;

// GUI function definitions
long CALLBACK WindowProc( HWND hwnd, unsigned msg, UINT wparam, LONG lparam );
long CALLBACK InfoProc( HWND hwnd, unsigned msg, UINT wparam, LONG lparam );
void DrawBitmap( HDC hdc, HBITMAP bitmap, int x, int y, int nSize );
void HighlightSquare ( HDC hdc, int Square, int sqSize, int bRedraw, unsigned long color, int border);
void DisplayText (char *sText);
void DrawBoard (const CBoard &board);
void DisplaySearchInfo ( CBoard &InBoard, int SearchDepth, int Eval, int nodes);
void RunningDisplay ( int bestmove, int bSearching);
void ReplayGame( int nMove, CBoard &Board );
int MakeMovePDN( CBoard &Board, int src, int dst);

// Database
int QueryDatabase( CBoard &Board);

// GUI Data
int hashing = 1, BoardFlip = 0;
int g_xAdd = 44, g_yAdd = 20, g_nSqSize = 64;
HBITMAP PieceBitmaps[32];    
HWND MainWnd= NULL, BottomWnd = NULL;
int g_nSelSquare = NONE;
char g_buffer[32768]; // For PDN copying/pasting/loading/saving

#include "ai.cpp"
// -----------------------
// Move Being Searched
// -----------------------
void RunningDisplay ( int bestMove, int bSearching )
	{
	char sTemp[4096];
	static int LastEval = 0, nps = 0;
	static int LastBest;
	int j = 0;
	if ( bestMove != -1 ) {	LastBest = bestMove; }
	bestMove = g_Movelist[1].Moves[ LastBest ].SrcDst;
	if (!bCheckerBoard)
		{
		j+=sprintf(sTemp+j,"Red: %d   White: %d                           ",  g_CBoard.nBlack, g_CBoard.nWhite);
		if (g_MaxDepth == BEGINNER_DEPTH) j+=sprintf(sTemp+j, "Level: Beginner  %ds  ", (int)fMaxSeconds);
		else if (g_MaxDepth == EXPERT_DEPTH) j+=sprintf(sTemp+j, "Level: Expert  %ds  ", (int)fMaxSeconds);
		else if (g_MaxDepth == NORMAL_DEPTH) j+=sprintf(sTemp+j, "Level: Advanced  %ds  ", (int)fMaxSeconds);
		if (bSearching) j+=sprintf (sTemp+j, "(searching...)\n");
			else j+=sprintf (sTemp+j,"\n");
		}
	float fDiv =  float (clock () - starttime) * CLOCKS_PER_SEC / 1000;
	if ( fDiv > 0.0f) nps = int (float(nodes) / fDiv); else nps = 0;
	if (abs (g_SearchEval) < 3000) LastEval = g_SearchEval;
	char cCap = (bestMove>>12) ? 'x' : '-';
	j+=sprintf( sTemp+j, "Depth: %d/%d (%d/%d)   Eval: %d  ", SearchDepth, g_SelectiveDepth, g_SearchingMove, g_Movelist[1].nMoves, -LastEval);
	if ( !bCheckerBoard ) j+=sprintf( sTemp+j, "\n" );
	j+=sprintf (sTemp+j, "Move: %d%c%d   Time: %.2fs   KNodes: %d   KN/Sec: %d", FlipX (BoardLocTo32[bestMove&63])+1, cCap, FlipX (BoardLocTo32[(bestMove>>6)&63])+1, (fDiv/1000.0f), (nodes/1000), nps);
	DisplayText (sTemp); 
	}
// ---------------------------------------------
//  Check Possiblity/Execute Move from one selected square to another
//  returns 0 if the move is not possible, otherwise 1, or a double jump value the move is an uncompleted jump
// ---------------------------------------------
int SquareMove( CBoard &Board, int x , int y, int xloc, int yloc, char cColor)
	{
	 int i, src, dst, nMSrc, nMDst;
     CMoveList MoveList;
            
     if (cColor == BLACK) MoveList.FindMovesBlack( Board.Sqs, Board.C );
		else MoveList.FindMovesWhite( Board.Sqs, Board.C );

	 for (i = 1; i <= MoveList.nMoves; i++)
           {
		    dst = BoardLoc[xloc + yloc*8];
			src = BoardLoc[x + y*8];
			nMSrc = MoveList.Moves[i].SrcDst&63;
			nMDst = (MoveList.Moves[i].SrcDst>>6)&63;
			if ( nMSrc == src && nMDst == dst )  // Check if the src & dst match a move from the generated movelist
			    {
				if (g_nDouble > 0) {g_GameMoves[g_nMoves-1].cPath[ g_nDouble-1] = nMDst; 
									g_GameMoves[g_nMoves-1].cPath[ g_nDouble  ] = 0;}
				else
					{
					g_GameMoves[ g_nMoves ].SrcDst = MoveList.Moves[i].SrcDst;
					g_GameMoves[ g_nMoves++ ].cPath[0] = 0;
					g_GameMoves[ g_nMoves ].SrcDst = 0;
					}
				if ( Board.DoMove (MoveList.Moves[i], HUMAN) == DOUBLEJUMP ) 
					{
					g_nDouble++;
					Board.SideToMove ^=3;
					return DOUBLEJUMP;
					}
				g_nDouble = 0;
                return 1; 
				}
           }
     return 0;
	}

void NewGame( )
{
	g_nSelSquare = NONE;
	g_nMoves = 0;
	g_GameMoves[ g_nMoves ].SrcDst = 0;
	g_cCompColor = WHITE;
	DisplayText("The Game Begins...");
}

// ===============================================
//
//                 G U I   S T U F F
//
// ===============================================
void DisplayText( char *sText)
	{
	if (bCheckerBoard)
		{
		if (g_sInfo) {
			strcpy (g_sInfo, sText);
			}
		 return;
		}
	if (BottomWnd == NULL) return;
	SetDlgItemText( BottomWnd, 150, sText );
	}

// -------------------------------
// WINDOWS CLIPBOARD FUNCTIONS
// -------------------------------
int TextToClipboard( char *sText )
{
	DisplayText (sText);

	char *bufferPtr;
	static HGLOBAL clipTranscript;
	int nLen = strlen( sText );
	if (OpenClipboard( MainWnd ) == TRUE)
		{
		EmptyClipboard();
		clipTranscript = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE , nLen + 10);
		bufferPtr = (char*) GlobalLock ( clipTranscript );
		memcpy(bufferPtr, sText, nLen + 1);
		GlobalUnlock( clipTranscript );
		SetClipboardData(CF_TEXT, clipTranscript);
		CloseClipboard(); 
		return 1;
		}

	DisplayText ( "Cannot Open Clipboard");
	return 0;
}

int TextFromClipboard ( char *sText, int nMaxBytes )
{
	if (!IsClipboardFormatAvailable(CF_TEXT)) {
		DisplayText ("No Clipboard Data");
		return 0;
		}

    OpenClipboard(MainWnd);
    HANDLE hData = GetClipboardData(CF_TEXT);
    LPVOID pData = GlobalLock(hData);
	int nSize = strlen ( (char *) pData);
	if (nSize > nMaxBytes) nSize = nMaxBytes;
	memcpy (sText, (LPSTR)pData, nSize );
	sText[ nSize ] = NULL;
    GlobalUnlock(hData);
    CloseClipboard();
	return 1;
}

// Gray out certain items
void ThinkingMenu( int bOn )
{
	int SwitchItems[8] = { ID_GAME_NEW, ID_FILE_LOADGAME, ID_EDIT_SETUPBOARD, ID_EDIT_PASTE_PDN, ID_EDIT_PASTE_POS,
		ID_GAME_CLEAR_HASH, ID_GAME_COMPUTEROFF, ID_GAME_HASHING };
	int nSet = (bOn) ? MF_GRAYED : MF_ENABLED;

	for (int i = 0; i < 8; i++) {
		EnableMenuItem( GetMenu( MainWnd ), SwitchItems[i] , nSet );	
	}
	EnableMenuItem( GetMenu( MainWnd ), ID_GAME_MOVENOW , (bOn)?MF_ENABLED:MF_GRAYED );
}

// =================================================================================\
// Threaded Function (for multitasking while the computer thinks)
// =================================================================================/
HANDLE hEngineReady, hAction;
HANDLE hThread;

DWORD WINAPI ThinkingThread( void *parm) 
{
parm = parm;
hEngineReady = CreateEvent( NULL, TRUE, FALSE, NULL);

	// Think of Move
     while (1==1)
	 {
	 SetEvent( hEngineReady );
	 WaitForSingleObject( hAction, INFINITE);
	 ResetEvent( hEngineReady );

	 g_cCompColor = (char)g_CBoard.SideToMove;
	 ComputerMove( g_cCompColor, g_CBoard );
	 g_bThinking = false;
	 ThinkingMenu( false );
	 }
	 
CloseHandle( hThread );
} 

// DATABASE FUNCTIONS
#include "database.cpp"

// ENGINE INITILIZATION
void InitEngine( )
{
	InitBitTables( );
	TEntry::Create_HashFunction( ); 
	pBook = new COpeningBook;
	if ( !bCheckerBoard || !pBook->Load( "engines\\opening.gbk" ) )
		pBook->Load( "opening.gbk" );
	LoadAllDatabases();
	g_CBoard.StartPosition( 1 );

	if (!bCheckerBoard ) {
		static DWORD ThreadId;
		hAction = CreateEvent( NULL, FALSE, FALSE, NULL);
		hThread = CreateThread( NULL, 0, ThinkingThread, 0,  0, &ThreadId);
		if (hThread == NULL) {
			MessageBox( MainWnd, "Cannot Create Thread", "Error", MB_OK);
			}
		}
}

// INITIALIZATION FUNCTION
static int AnyInstance (HINSTANCE this_inst)
	{
	// create main window
	MainWnd = CreateWindow( "CheckClass",  g_sNameVer,    
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,  
		CW_USEDEFAULT, CW_USEDEFAULT, 620, 650,    // x,y Position  x,y Size
	    NULL, NULL, this_inst, NULL);
	if (MainWnd == NULL) return (FALSE);
	BottomWnd = CreateDialog( this_inst, "BotWnd", MainWnd, (DLGPROC)InfoProc);

	// Load Piece Bitmaps
	char* psBitmapNames[10] = {NULL, "Rcheck", "Wcheck", NULL, NULL, "RKing", "Wking", "Wsquare", "Bsquare", NULL};
	for (int i = 0; i < 9; i++)
		{
		if (psBitmapNames[i] != NULL) 
			PieceBitmaps[ i ] = (HBITMAP)LoadImage( this_inst, psBitmapNames[i], IMAGE_BITMAP, 0, 0, 0 );
		}

	MoveWindow (BottomWnd, 0, 550, 680, 150, TRUE);
	ShowWindow (MainWnd, SW_SHOW);
	ThinkingMenu( false );

	g_CBoard.StartPosition( 1 );
	DrawBoard( g_CBoard );
	InitEngine( );

	return (TRUE);
	}

// ------------------------------------
// FirstInstance - register window class for the application,
// ------------------------------------
static void RegisterClass( HINSTANCE this_inst )
{
    WNDCLASS    wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc =  WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof( DWORD );
    wc.hInstance = this_inst;
    wc.hIcon = LoadIcon( this_inst, "chIcon" );
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)GetStockObject( DKGRAY_BRUSH );
    wc.lpszMenuName = "CheckMenu";
    wc.lpszClassName = "CheckClass";
    BOOL rc = RegisterClass( &wc );
} 

// ------------------------------------
//  WinMain - initialization, message loop
// ------------------------------------
int PASCAL WinMain( HINSTANCE this_inst, HINSTANCE prev_inst, LPSTR cmdline,
                    int cmdshow )
{
    MSG msg;

    if( !prev_inst ) RegisterClass( this_inst );
    if (AnyInstance( this_inst ) == FALSE) return (FALSE);

    while( GetMessage( &msg, NULL, NULL, NULL ) ) {
        TranslateMessage( &msg );
        DispatchMessage( &msg );
		}

    return( msg.wParam );
} 

//---------------------------------------------------------
//Draw a frame around a square clicked on by user
//----------------------------------------------------------
 void HighlightSquare( HDC hdc, int Square, int sqSize, int bRedraw, unsigned long color, int border)
	{
    HBRUSH              brush;
    RECT                rect;

    brush = CreateSolidBrush( color );
    
     if (Square >= 0 && Square <=63) 
		{
		int x=Square%8, y=Square/8;
		if (BoardFlip == 1) {x = 7-x; y = 7-y;}
		rect.left = x*sqSize + g_xAdd;
		rect.right = (x+1)*sqSize + g_xAdd;
		rect.top = y * sqSize + g_yAdd;
		rect.bottom = (y+1)*sqSize + g_yAdd;
		FrameRect(hdc, &rect, brush);
		if (border == 2)
			{
			 rect.left+=1; rect.right -=1; rect.top+=1; rect.bottom-=1;
			 FrameRect(hdc, &rect, brush);
			}
		}
      DeleteObject( brush );
	}

//---------------------------------------------------------------
// Function to Draw a single Bitmap
// --------------------------------------------------------------
 void DrawBitmap( HDC hdc, HBITMAP bitmap, int x, int y, int nSize )
{
    BITMAP      bitmapbuff;
    HDC         memorydc;
    POINT       origin, size;
    
    memorydc = CreateCompatibleDC( hdc );
    SelectObject( memorydc, bitmap );
    SetMapMode( memorydc, GetMapMode( hdc ) );
    GetObject( bitmap, sizeof( BITMAP ), (LPSTR) &bitmapbuff );

    origin.x = x;
    origin.y = y;
	if (nSize ==0)
		{size.x = bitmapbuff.bmWidth;
		 size.y = bitmapbuff.bmHeight;
		}
	else
		{size.x = nSize;
		 size.y = nSize;
		}

    DPtoLP( hdc, &origin, 1 );
    DPtoLP( memorydc, &size, 1 );
    if (nSize == bitmapbuff.bmWidth)
		{
		BitBlt( hdc, origin.x, origin.y, size.x, size.y, memorydc, 0, 0, SRCCOPY);
		}
	else {
		SetStretchBltMode( hdc, COLORONCOLOR/*STRETCH_HALFTONE*/ );
		SetBrushOrgEx( hdc, 0, 0, NULL);
		StretchBlt( hdc, origin.x, origin.y, size.x, size.y, memorydc, 0, 0, bitmapbuff.bmWidth, bitmapbuff.bmHeight, SRCCOPY);
		}
    DeleteDC( memorydc );
} 

// ------------------
// Draw Board
// ------------------
void DrawBoard (const CBoard &board)
	{
	HDC hdc = GetDC (MainWnd);
    int i, start = 0, add = 1, add2 = 1, x = 0, y = 0, mul = 1;
	int nSize = 64;

	if (BoardFlip == 1) {start = 63;
						add = -1;
						add2 = 0;}

    for (i = start; i>=0 && i<=63; i+=add)
		{
        // piece
        if      (board.Sqs [ BoardLoc[i] ] != EMPTY && board.Sqs [ BoardLoc[i] ] != INVALID) DrawBitmap (hdc, PieceBitmaps [ board.Sqs[ BoardLoc[i] ] ] , x*nSize*mul +g_xAdd , y*nSize*mul +g_yAdd, nSize );
        // empty square
        else if ( ((i%2 == 1) && (i/8)%2 == 1 ) || ((i%2 == 0) && (i/8)%2 == 0) )
               DrawBitmap (hdc, PieceBitmaps [ 7 ] , x*nSize*mul +g_xAdd , y*nSize*mul +g_yAdd, nSize );
        else  DrawBitmap (hdc, PieceBitmaps [ 8 ] , x*nSize*mul +g_xAdd , y*nSize*mul +g_yAdd, nSize );
       
   		x++;
		if (x == 8){x = 0; y++;}
        }
	if (g_nSelSquare != NONE) HighlightSquare ( hdc, g_nSelSquare, g_nSqSize, TRUE, 0xFFFFFF, 1); 
	ReleaseDC(MainWnd, hdc );  
    } 

// ------------------
// Replay Game from Game Move History up to nMove
// ------------------
void ReplayGame ( int nMove, CBoard &Board )
	{
	Board = g_StartBoard;
	g_nMoves = 0;

	int i = 0;
	while (g_GameMoves[i].SrcDst != 0 && i < nMove)
		{
		AddRepBoard ( Board.HashKey, i);
		Board.DoMove ( g_GameMoves[i], SEARCHED);
		i++;
		}
	g_nMoves = i;
	g_nSelSquare = NONE;
	}

// --------------------
// GetFileName - get a file name using common dialog
// --------------------
static unsigned char  sSaveGame[] = "Checkers Game (*.pdn)" \
                                       "\0" \
                                       "*.pdn" \
                                       "\0\0";
static BOOL GetFileName(  HWND hwnd, BOOL save, char *fname, unsigned char *filterList)
{
    OPENFILENAME        of;
    int                 rc;

    memset( &of, 0, sizeof( OPENFILENAME ) );
    of.lStructSize = sizeof( OPENFILENAME );
    of.hwndOwner = hwnd;
    of.lpstrFilter = (LPSTR) filterList;
    of.lpstrDefExt = "";
    of.nFilterIndex = 1L;
    of.lpstrFile = fname;
    of.nMaxFile = _MAX_PATH;
    of.lpstrTitle = NULL;
    of.Flags = OFN_HIDEREADONLY;
    if( save ) {
		rc = GetSaveFileName( &of );
        } else {
          rc = GetOpenFileName( &of );
        }
    return( rc );
}

// ------------------
// File I/O
// ------------------
void SaveGame( char *sFilename)
{
 FILE* pFile = fopen (sFilename, "wb");
 if (pFile == NULL) return;

 g_CBoard.ToPDN( g_buffer );
 fwrite( g_buffer, 1, strlen( g_buffer ), pFile);
 
 fclose( pFile );
}

void LoadGame ( char *sFilename )
{
 FILE* pFile = fopen (sFilename, "rb");
 if (pFile == NULL) return;
 int x = 0;
 while (x < 32000 && !feof(pFile) )
	 g_buffer[x++] = getc (pFile);
 g_buffer[x-1] = 0;

 NewGame();
 g_CBoard.FromPDN( g_buffer );

 fclose (pFile);
 ReplayGame ( 1000, g_CBoard);
 DrawBoard (g_CBoard);
}

//
// GUI HELPER FUNCTIONS
//
void EndSetup( )
{
	g_CBoard.SetFlags( );
	g_StartBoard = g_CBoard;
	g_nMoves = 0;
	g_bSetupBoard = FALSE;
	DisplayText("");
}

void MoveNow( )
{
	if ( g_bThinking ) {
		g_bStopThinking = true;
		WaitForSingleObject( hEngineReady, 500 );
		RunningDisplay( -1, 0 );
	}
}

void ComputerGo( )
{
	if ( g_bSetupBoard ) EndSetup();
	if ( g_bThinking ) {
		MoveNow( );
	}
	else {
		ThinkingMenu( true );
		WaitForSingleObject( hEngineReady, 1000 );
		g_bStopThinking = false;
		g_bThinking = true;
		SetEvent( hAction );
	}
}


int GetSquare( int &x, int &y)
{
	 // Calculate the square the user clicked on (0-63)
     x = (x-g_xAdd) / g_nSqSize;
     y = (y-g_yAdd) / g_nSqSize;
	 if (BoardFlip == 1) {x=7-x; y = 7-y;}
	 return x + y*8;
}

// ===============================================
// Process messages to the Bottom Window
// ===============================================
long CALLBACK InfoProc( HWND hwnd, unsigned msg, UINT wparam, LONG lparam )
{
	switch (msg)
	{
	case WM_COMMAND:
		switch (LOWORD(wparam))
		{
		case IDC_TAKEBACK:
			MoveNow();
			if (g_nMoves < 0) g_nMoves = 0;
			g_nMoves-=2;
			ReplayGame( g_nMoves, g_CBoard );
			DrawBoard( g_CBoard );
			SetFocus( MainWnd );
		break;

		case IDC_PREV:
			MoveNow();
			if (g_nMoves > 0)
				{
				g_nMoves--;
				ReplayGame( g_nMoves, g_CBoard);
				DrawBoard( g_CBoard);
				}
			SetFocus ( MainWnd );
		break;

		case IDC_NEXT:
			MoveNow();
			g_nMoves++;
			ReplayGame( g_nMoves, g_CBoard );
			DrawBoard( g_CBoard );
			SetFocus( MainWnd );
		break;

		case IDC_START:
			MoveNow();
			g_nMoves = 0;
			ReplayGame( g_nMoves, g_CBoard );
			DrawBoard( g_CBoard);
			SetFocus( MainWnd );
		break;
		case IDC_END:
			MoveNow();
			g_nMoves = 1000;
			ReplayGame( g_nMoves, g_CBoard );
			DrawBoard( g_CBoard);
			SetFocus( MainWnd );
		break;
		case IDC_GO:
			if (g_bSetupBoard) 
				{EndSetup(); break;}
			ComputerGo( );
			SetFocus( MainWnd );
		break;
		}
	}
return (0);
}

// ==============================================================
//  Level Select Dialog Procedure
// ==============================================================
BOOL CALLBACK LevelDlgProc( HWND hwnd, UINT msg,  UINT wparam, LONG lparam )
{
 lparam = lparam;                    /* turn off warning */
  
 switch( msg ) {
    case WM_INITDIALOG:
		SetDlgItemInt ( hwnd, IDC_DEPTH, g_MaxDepth, 1);
		SetDlgItemInt ( hwnd, IDC_TIME, (int)fMaxSeconds, 1);
	     return( TRUE );

    case WM_COMMAND: 
		if ( LOWORD( wparam ) == IDOK )	{
			g_MaxDepth = GetDlgItemInt ( hwnd, IDC_DEPTH, 0, 1 );
			if (g_MaxDepth > 52) g_MaxDepth = 52;
            fMaxSeconds = (float)GetDlgItemInt( hwnd, IDC_TIME, 0, 1 );
			}
		if( LOWORD( wparam ) == IDOK || LOWORD( wparam ) == IDCANCEL)	{
			EndDialog( hwnd, TRUE );
            return( TRUE );
			}
	break;                                       
    }
        
 return (FALSE);
}
// ===============================================
// Process messages to the MAIN Window
// ===============================================
long CALLBACK WindowProc( HWND hwnd, unsigned msg, UINT wparam, LONG lparam )
{
PAINTSTRUCT         ps;
HDC hdc;
int nSquare;
int x, y;
int Eval;
static char sFilename[256*20];

switch (msg)
	{
	// Process Menu Commands
	case WM_COMMAND:
		switch (LOWORD(wparam))
		{
		case ID_GAME_FLIPBOARD:
			BoardFlip^=1;
			DrawBoard(g_CBoard);
		break;
		case ID_GAME_NEW:
			NewGame( );
			g_CBoard.StartPosition( 1 );
			DrawBoard( g_CBoard );
		break;
		case ID_GAME_COMPUTERMOVE:
			ComputerGo();
		break;
		case ID_GAME_COMPUTEROFF:
			g_cCompColor = 0;
		break;

        case ID_GAME_HASHING:
            hashing ^=1;
        break;

		case ID_GAME_CLEAR_HASH:
			ZeroMemory( TTable, sizeof (TEntry) * HASHTABLESIZE );
		break;

		case ID_OPTIONS_BEGINNER:
			g_MaxDepth = BEGINNER_DEPTH;
		break;
		case ID_OPTIONS_NORMAL:
			g_MaxDepth = NORMAL_DEPTH;
		break;
		case ID_OPTIONS_EXPERT:
			g_MaxDepth = EXPERT_DEPTH;
		break;
		case ID_OPTIONS_2SECONDS:  fMaxSeconds = 2.0f; 	break;
		case ID_OPTIONS_5SECONDS:  fMaxSeconds = 5.0f; 	break;
		case ID_OPTIONS_10SECONDS: fMaxSeconds = 10.0f; break;
		case ID_OPTIONS_30SECONDS: fMaxSeconds = 30.0f; break;

		case ID_OPTIONS_CUSTOMLEVEL:
			  DialogBox( (HINSTANCE)GetWindowLong( hwnd, GWL_HINSTANCE )  , "LevelWnd", hwnd, (DLGPROC)LevelDlgProc);
		break;

		case ID_FILE_SAVEGAME:
			if (GetFileName( hwnd, 1, sFilename, sSaveGame))
				SaveGame(sFilename);
			break;
		case ID_FILE_LOADGAME:
			if (GetFileName( hwnd, 0, sFilename, sSaveGame))
				LoadGame(sFilename);
			break;

		case ID_FILE_EXIT:
			PostMessage (hwnd, WM_DESTROY, 0, 0);
		break;

		case ID_EDIT_SETUPBOARD:
			if (g_bSetupBoard) EndSetup( );				
			else {g_bSetupBoard = TRUE;
				  DisplayText ("BOARD SETUP MODE.       (Click GO to end) \n(shift+click to erase pieces) \n(alt+click on a piece to set that color to move)");
				 }
		break;

		case ID_EDIT_PASTE_POS:
			if (TextFromClipboard( g_buffer, 512 ))
				{
				if (g_CBoard.FromFen( g_buffer ))
					{
					NewGame( );
					g_StartBoard = g_CBoard;
					}
				}
			DrawBoard( g_CBoard );
		break;

		case ID_EDIT_COPY_POS:
			g_CBoard.ToFen( g_buffer );
			TextToClipboard( g_buffer );
		break;

		case ID_EDIT_COPY_PDN:
			g_CBoard.ToPDN( g_buffer );
			TextToClipboard( g_buffer );
		break;

		case ID_GAME_MOVENOW: MoveNow(); 
			break;

		case ID_EDIT_PASTE_PDN:
			if (TextFromClipboard( g_buffer, 16380 ))
				{
				NewGame( );
				g_CBoard.FromPDN( g_buffer );
				}		
			DrawBoard( g_CBoard );
		break;
		}
	break;

	// Keyboard Commands
     case WM_KEYDOWN:

     if (int(wparam) == '2') pBook->AddPosition( g_CBoard, -1, false); 
     if (int(wparam) == '3') pBook->AddPosition( g_CBoard, 0, false); 
     if (int(wparam) == '4') pBook->AddPosition( g_CBoard, 1, false); 
 	 if (int(wparam) == '6') pBook->RemovePosition( g_CBoard, false);
     if (int(wparam) == 'C') {
		 delete pBook; 
		 pBook = new COpeningBook;
		 DisplayText("Book Cleared");
		}
	 if (int(wparam) == 'S') pBook->Save( "opening.gbk");
	 if (int(wparam) == 'G') ComputerGo( );
	 if (int(wparam) == 'M') MoveNow( );
     
     break;

	// Process Mouse Clicks on the board
    case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:

    SetFocus (hwnd);
                                       
    x = (int)(short)LOWORD(lparam );
    y = (int)(short)HIWORD(lparam );
	nSquare = GetSquare (x, y);

	if (g_bSetupBoard)
		{
		if (msg ==	WM_LBUTTONUP) return TRUE;
		int nBoardLoc = BoardLoc[nSquare];
		if (nBoardLoc == 0) break;
		if (GetKeyState( VK_SHIFT) < 0) 
			{g_CBoard.Sqs[ nBoardLoc ] = EMPTY; DrawBoard (g_CBoard); break;}
		if (GetKeyState( VK_MENU) < 0) 
			{g_CBoard.SideToMove = (g_CBoard.Sqs[ nBoardLoc ] & BPIECE) ? BLACK : WHITE; break;}
		if ( g_CBoard.Sqs[ nBoardLoc ] == BPIECE ) g_CBoard.Sqs [ nBoardLoc ] = BKING;
		else if ( g_CBoard.Sqs[ nBoardLoc ] == BKING ) g_CBoard.Sqs [ nBoardLoc ] = EMPTY;
		else g_CBoard.Sqs[ nBoardLoc ] = (nBoardLoc < 36) ? BPIECE : BKING;
		DrawBoard (g_CBoard);
		break;
		}

	 if ( g_bThinking ) return TRUE; // Don't move pieces when computer is thinking

	 // Did the user click on his/her own piece?
	 if (nSquare >=0 && nSquare <=63 && g_CBoard.Sqs[ BoardLoc[nSquare] ] != EMPTY)
		{
		 if (msg ==	WM_LBUTTONUP) return TRUE;
		 if (( (g_CBoard.Sqs [ BoardLoc[nSquare] ]&BPIECE) && g_CBoard.SideToMove == BLACK)  || 
			 ( (g_CBoard.Sqs [ BoardLoc[nSquare] ]&WPIECE) && g_CBoard.SideToMove == WHITE ))
			{
			g_nSelSquare = nSquare;
			if (g_nSelSquare != NONE) 
				{
				DrawBoard (g_CBoard);
				}

			hdc = GetDC( MainWnd );
			HighlightSquare( hdc, g_nSelSquare, g_nSqSize, TRUE, 0xFFFFFF, 1); 
			ReleaseDC( MainWnd, hdc ); 
			}
		}
	 else // Did the user click on a valid destination square?
		{
		int MoveResult = SquareMove( g_CBoard, g_nSelSquare %8 , g_nSelSquare/8 , x, y, g_CBoard.SideToMove);
		if (MoveResult == 1)
			{
			g_nSelSquare = NONE;
			DrawBoard (g_CBoard);
			if (g_cCompColor == g_CBoard.SideToMove) ComputerGo();
			}
		else if (MoveResult == DOUBLEJUMP) 
			{
			g_nSelSquare = nSquare;
			DrawBoard (g_CBoard);
			}
		}
	return TRUE;         

	case WM_RBUTTONDOWN:

     x = (int)(short)LOWORD(lparam );
     y = (int)(short)HIWORD(lparam );
	 nSquare = GetSquare (x, y);

		if (g_bSetupBoard)
			{
			int nBoardLoc = BoardLoc[nSquare];
			if (nBoardLoc == 0) break;
			if (GetKeyState ( VK_SHIFT) < 0) 
				{g_CBoard.Sqs[ nBoardLoc ] = EMPTY; DrawBoard (g_CBoard); break;}
			if (GetKeyState ( VK_MENU) < 0) 
				{g_CBoard.SideToMove = (g_CBoard.Sqs[ nBoardLoc ] & BPIECE) ? BLACK : WHITE; break;}
			if (g_CBoard.Sqs[ nBoardLoc ] == WPIECE) g_CBoard.Sqs[ nBoardLoc ] = WKING;
			else if (g_CBoard.Sqs[ nBoardLoc ] == WKING) g_CBoard.Sqs[ nBoardLoc ] = EMPTY;
			else g_CBoard.Sqs[ nBoardLoc ] = (nBoardLoc > 9) ? WPIECE : WKING;
			DrawBoard (g_CBoard);
			break;
			}

			Eval = -g_CBoard.EvaluateBoard ( 0, -100000, 10000 );
			sprintf( g_buffer, "Eval: %d  Mobility B: %d  W: %d", Eval, g_CBoard.C.GetBlackMoves( ), g_CBoard.C.GetWhiteMoves( ) );
			if ( g_CBoard.InDatabase() ) 
				{
				if (QueryDatabase( g_CBoard ) == 0) strcat( g_buffer, "\nDRAW");
				else if (QueryDatabase( g_CBoard ) == 2) strcat( g_buffer, "\nWHITE WIN");
				else if (QueryDatabase( g_CBoard ) == 1) strcat( g_buffer, "\nBLACK WIN");
				else strcat( g_buffer, "\nDATABASE ERROR");
				}
			DisplayText( g_buffer );
		
	return (TRUE);

	case WM_SETCURSOR:
		if (!g_bThinking) SetCursor( LoadCursor( NULL, IDC_ARROW ) );
	return (TRUE);

	case WM_PAINT:
        hdc = BeginPaint( hwnd, &ps );
        DrawBoard( g_CBoard );
        EndPaint(hwnd, &ps);
    break;

    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;

	default:
		return( DefWindowProc( hwnd, msg, wparam, lparam ) );
	}
return (0);
}

//
// This will interface CheckerBoard, you can get it at http://www.fierz.ch/
// 
struct CBmove { int notused; };
int ConvertFromCB[16] = { 0, 0, 0, 0, 0, 2, 1, 0, 0, 6, 5, 0, 0, 0, 0, 0};
int ConvertToCB  [16] = { 0, 6, 5, 0, 0, 10,9, 0, 0, 0, 0, 0, 0, 0, 0, 0};

int WINAPI getmove(int board[8][8], int color, double maxtime, char str[1024], int *playnow, int info, int unused, struct CBmove *move)
{
static int nDraw = 0, Eval = 0, Adjust = 0;
g_bEndHard = ( info&2 ) ? TRUE : FALSE;
if ( !bCheckerBoard )
	{ // initialization               
	bCheckerBoard = TRUE; 
	InitEngine( );
	g_nMoves = 0;
	}
if (g_nMoves > 500) g_nMoves = 0;
g_CBoard.StartPosition( (info&1) );
g_sInfo = str;
g_pbPlayNow = playnow;

int i;
for (i = 0; i < 64; i++)
	g_CBoard.Sqs [ BoardLoc[ i ] ] = ConvertFromCB [ board[i%8][7-i/8] ];
g_CBoard.Sqs[0] = INVALID;
g_CBoard.SideToMove = color ^ 3;
g_CBoard.SetFlags();
AddRepBoard ( TEntry::HashBoard (g_CBoard), g_nMoves);

// Work around for a bug in checkerboard not sending reset
static int LastTotal = 0;
if ( (g_CBoard.nBlack + g_CBoard.nWhite) > LastTotal) {

	/*
	Adjust = 0;
	if (Eval < -50) Adjust = -1;
	if (Eval > 50)  Adjust = 1;
	if (g_nMoves > 150 && abs(Eval) < 90) Adjust = 0;
	pBook->LearnGame ( 15, Adjust );
	pBook->Save ( "opening.gbk" );
	*/

	g_nMoves = 0;
	nDraw = 0;
	AddRepBoard ( TEntry::HashBoard(g_CBoard), g_nMoves);
	}
LastTotal = g_CBoard.nBlack + g_CBoard.nWhite;

fMaxSeconds = (float)maxtime *.985f;
Eval = ComputerMove ( g_CBoard.SideToMove, g_CBoard );

for (i = 0; i < 64; i++)
   if (BoardLoc[i]>0 && g_CBoard.Sqs [ BoardLoc[i] ] != INVALID) 
		board[i%8][7-i/8] = ConvertToCB [ g_CBoard.Sqs [ BoardLoc[i] ] ];
AddRepBoard ( TEntry::HashBoard (g_CBoard), g_nMoves);

int retVal = 3;
if ((Eval > 320 && g_CBoard.SideToMove == BLACK) || (Eval < -320 && g_CBoard.SideToMove == WHITE)) retVal = 1;
if ((Eval > 320 && g_CBoard.SideToMove == WHITE) || (Eval < -320 && g_CBoard.SideToMove == BLACK)) retVal = 2;
if ((g_nMoves > 90 && abs (Eval) < 11) || (g_nMoves > 150 && abs(Eval) < 50) ) nDraw++; else nDraw = 0;
if (nDraw >= 8) retVal = 0;
if (g_CBoard.InDatabase() && abs(Eval) < 2) retVal = 0;

g_nMoves++; // Increment again since the next board coming in will be after the opponent has played his move

return retVal;
}


int WINAPI enginecommand(char str[256], char reply[1024])
{
	char command[256],param1[256],param2[256];
	char *stopstring;
	
	sscanf(str,"%s %s %s",command,param1,param2);

	if(strcmp(command,"name")==0)
		{
		sprintf(reply, g_sNameVer);
		return 1;
		}
	if(strcmp(command,"about")==0)
		{
		if (pBook)	sprintf(reply,"%s\nby Jonathan Kreuzer\n\nOpening Book is %d Moves ", g_sNameVer, pBook->m_nPositions);
		else 		sprintf(reply,"%s\nby Jonathan Kreuzer\n", g_sNameVer );
		return 1;
		}

	if(strcmp(command,"set")==0)
		if(strcmp(param1,"hashsize")==0)
			{
			int nMegs = strtol( param2, &stopstring, 10 );
			if (nMegs < 1) return 0;
			if (nMegs > 128) nMegs = 128;
			HASHTABLESIZE = nMegs * (1000000 / 12);
			if (TTable) free (TTable);
			TTable = (TEntry *) malloc( sizeof (TEntry) * HASHTABLESIZE + 2);

			return 1;
			}
	if(strcmp(command,"get")==0)
		{
		if(strcmp(param1,"hashsize")==0)
			{
			sprintf (reply, "%d", HASHTABLESIZE / (1000000 / 12));
			return 1;
			}
		if(strcmp(param1,"protocolversion")==0)
			{
			sprintf(reply,"2");
			return 1;
			}
		if(strcmp(param1,"gametype")==0)
			{
			sprintf(reply,"21");
			return 1;
			}
		}
	strcpy (reply, "?");
	return 0;
}