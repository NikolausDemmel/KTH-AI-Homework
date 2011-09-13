// graphics.c
//
// part of checkerboard
//
// implements graphics functionality of CB: printboard, selectstone, animation
//
// ugly: it has a lot of extern variables!
//
// animation can be turned on and off by defining ANIMATION or not
#define ANIMATION

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "standardheader.h"
#include "CBstructs.h"
#include "CBconsts.h"
#include "graphics.h"
#include "checkerboard.h"
#include "coordinates.h"
#include "utility.h"
#include "bmp.h"

//enum state {NORMAL, AUTOPLAY, ENGINEMATCH, ENGINEGAME, ANALYZEGAME, OBSERVEGAME, ENTERGAME, BOOKVIEW, BOOKADD, RUNTESTSET};

// lots of external variables from checkerboard.c - very ugly
extern struct CBmove move;
extern struct CBoptions gCBoptions;
extern int board8[8][8];
extern int color;
extern struct PDNgame GPDNgame;


// disable double-to-int warning in this file to avoid getting dozens of warnings
#pragma warning(disable : 4244)

static int g_size = 0;			// holds size of square, if it is different from previous call, we have to update stuff
static HDC memdc,bgdc,bmpdc,stretchdc,boarddc;							// stores the virtual device handle 
static HBITMAP hbit,bgbitmap,clipbitmap,stretchbitmap,boardbitmap;		// store the virtual bitmap 
static HBRUSH hbrush; 													// store the brush handle 
static int offset=0, upperoffset=0;
static HFONT myfont;			// font for board numbers
	
int setoffsets(int _offset, int _upperoffset)
	{
	// receives offsets from checkerboard.c instead of using extern variables.
	offset = _offset;
	upperoffset = _upperoffset;
	return 1;
	}


int initgraphics(HWND hwnd)
	{
	HDC hdc;
	int maxX, maxY;

	// get screen coordinates 
	maxX = GetSystemMetrics(SM_CXSCREEN);
	maxY = GetSystemMetrics(SM_CYSCREEN);

	// make a compatible memory image 
	hdc = GetDC(hwnd);
	
	memdc = CreateCompatibleDC(hdc);
	bmpdc = CreateCompatibleDC(hdc);
	bgdc = CreateCompatibleDC(hdc);
	stretchdc = CreateCompatibleDC(hdc);
	boarddc = CreateCompatibleDC(hdc);

	hbit = CreateCompatibleBitmap(hdc, maxX, maxY);
	SelectObject(memdc, hbit);

	bgbitmap=CreateCompatibleBitmap(hdc,maxX,maxY);
	SelectObject(bgdc, bgbitmap);

	stretchbitmap=CreateCompatibleBitmap(hdc,maxX,maxY);
	SelectObject(stretchdc, stretchbitmap);

	boardbitmap = CreateCompatibleBitmap(hdc, maxX, maxY);
	SelectObject(boarddc, boardbitmap);
	
	hbrush = (HBRUSH) GetStockObject(WHITE_BRUSH);
	SelectObject(memdc, hbrush);
	
	PatBlt(memdc, 0, 0, maxX, maxY, PATCOPY);
	
	ReleaseDC(hwnd, hdc);

	// call resizegraphics to get everything in right size
	resizegraphics(hwnd);

	return 1;
	}

int updategraphics(HWND hwnd)
	{
	// is called on WM_PAINT and bitblits the memory graphics onto the screen
	HDC hdc; 
	PAINTSTRUCT paintstruct;
	
	hdc = BeginPaint(hwnd, &paintstruct);
	if(hdc == NULL)
		{
		CBlog("could not allocate HDC on WM_PAINT");
		return 0;
		}
	
	BitBlt(hdc,paintstruct.rcPaint.left,paintstruct.rcPaint.top,
		paintstruct.rcPaint.right-paintstruct.rcPaint.left,
		paintstruct.rcPaint.bottom-paintstruct.rcPaint.top,
		memdc,paintstruct.rcPaint.left,paintstruct.rcPaint.top,SRCCOPY);
	EndPaint(hwnd,&paintstruct);
	ReleaseDC(hwnd,hdc);
	return 1;
	}

int resizegraphics(HWND hwnd)
	{
	// is called on WM_SIZE or WM_SIZING, i.e. whenever the board size has to be adjusted
	double size;
	HBITMAP bmp_dark, bmp_light, hOldBitmap;
	int x0, y0, x1, y1;
	int success;
	

	getxymetrics(&size, &size, hwnd);
	updatestretchDC(hwnd, bmpdc, stretchdc, 100);

	// print board without stones to board dc
	success = SetStretchBltMode(boarddc,HALFTONE);
	bmp_dark = getCBbitmap(BMPDARK);
	bmp_light = getCBbitmap(BMPLIGHT);

	// dark squares with BMPs
	// TODO: all of this should be done just once on resize?!
	hOldBitmap = (HBITMAP) SelectObject(bmpdc, bmp_dark);

	if(hOldBitmap == NULL)
		CBlog("dark square bitmap is null");

	if(bmp_dark == NULL)
		CBlog("bmp dark is null");

	if(gCBoptions.mirror)
		{
		// make a single dark square:
		x0 = 0; 
		y0 = 0;
		success = StretchBlt(boarddc, 0, 0, size, size, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
		// make a single light square
		x1 = 1; 
		y1 = 0;
		SelectObject(bmpdc, bmp_light);
		success = StretchBlt(boarddc, size, 0, size, size, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
		}
	else
		{
		// make a single dark square
		x0 = 1; 
		y0 = 0;
		success = StretchBlt(boarddc, size, 0, size, size, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
		// make a single light square
		x1 = 0; 
		y1 = 0;

		SelectObject(bmpdc, bmp_light);
		success = StretchBlt(boarddc, 0, 0, size, size, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
		}

	// now, flood-fill the board with what we already have:
	success = BitBlt(boarddc, size*x1, size*1, size, size, boarddc, x0*size, y0*size, SRCCOPY);
	success = BitBlt(boarddc, size*x0, size*1, size, size, boarddc, x1*size, y1*size, SRCCOPY);

	// copy to the right
	success = BitBlt(boarddc, size*2, 0, size*2, size*2, boarddc, 0,0, SRCCOPY);
	success = BitBlt(boarddc, size*4, 0, size*4, size*2, boarddc, 0,0, SRCCOPY);

	// copy down
	success = BitBlt(boarddc, 0, size*2, size*8, size*2, boarddc, 0,0, SRCCOPY);
	success = BitBlt(boarddc, 0, size*4, size*8, size*4, boarddc, 0,0, SRCCOPY);

	SelectObject(bmpdc, hOldBitmap);

	// Create a font:
	DeleteObject(myfont);
	myfont = CreateFont((int)(size/4+1),0,0,0,0,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SCRIPT,"Courier New");
	
	return 1;
	}


int updateboardgraphics(HWND hwnd)
	{
	// is called whenever the board has to be redrawn
	printboard(hwnd, memdc, bmpdc, stretchdc, board8);
			
	return 1;
	}
			
int boardtoHDC(HWND hwnd, HDC hdc)
	{
	printboard(hwnd, hdc, bmpdc, stretchdc, board8);
	return 1;
	}


int diagramtoclipboard(HWND hwnd)
	{
	//copies a diagram to the clipboard
	RECT WinDim;
	int cxClient, cyClient;

	// create a bitmap for this, with the right size for the diagram only:
	GetClientRect(hwnd, &WinDim);
	cxClient = WinDim.right;
	cyClient = WinDim.bottom;
	clipbitmap = CreateCompatibleBitmap(bgdc,cxClient,cyClient-offset);
	// select bitmap in animation device context
	SelectObject(bgdc, clipbitmap);
	// print board in mem dc
	printboard(hwnd, memdc, bmpdc, stretchdc, board8);
	// and copy it shifted into the ani dc, which means that the
	// bitmap now has the board!
	BitBlt(bgdc,0,0,cxClient,cyClient-offset,memdc,0,upperoffset,BLACKNESS);
	BitBlt(bgdc,0,0,cxClient,cyClient-offset,memdc,0,upperoffset,SRCCOPY);

	if(OpenClipboard(hwnd))
		{
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, clipbitmap);
		CloseClipboard();
		}
	SelectObject(bgdc, bgbitmap);
	DeleteObject(clipbitmap);

	return 1;
	}

int samplediagramtoclipboard(HWND hwnd)
	{
	//copies a diagram to the clipboard
	RECT WinDim;
	int cxClient, cyClient;
	int x,y;

	// create a bitmap for this, with the right size for the sample
	// diagram, 3x3 squares.
	GetClientRect(hwnd, &WinDim);
	cxClient = WinDim.right;
	cyClient = WinDim.bottom;
	x = cxClient;
	x /= 8;
	x *= 3;
	y = x;
	clipbitmap = CreateCompatibleBitmap(bgdc,x,y);
	// select bitmap in animation device context
	SelectObject(bgdc, clipbitmap);
	// print board in mem dc
	printsampleboard(hwnd, memdc, bmpdc, stretchdc);
	// and copy it shifted into the ani dc, which means that the
	// bitmap now has the board!
	BitBlt(bgdc,0,0,cxClient,cyClient-offset,memdc,0,upperoffset,BLACKNESS);
	BitBlt(bgdc,0,0,cxClient,cyClient-offset,memdc,0,upperoffset,SRCCOPY);

	if(OpenClipboard(hwnd))
		{
		EmptyClipboard();
		SetClipboardData(CF_BITMAP, clipbitmap);
		CloseClipboard();
		}
	SelectObject(bgdc, bgbitmap);
	DeleteObject(clipbitmap);

	// restore old board, else the sample bitmap is shown in CB
	printboard(hwnd, memdc, bmpdc, stretchdc, board8);

	return 1;
	}


DWORD AnimationThreadFunc(HWND hwnd)
	{
	/* this thread drives the animation of the checker */
	/* a background image without the animated stone is printed into bgdc */
	/* this is copied into  and the stone is added */
	/* and this is finally copied into memdc */

	/* the global 'move' is used by this function */
	// this function is also responsible for executing the move on the internal board of CB.
	//char str[32];

	int i,j;
	int maxX,maxY;
	int x,y,x2,y2;   // the move goes from x,y to x2,y2;
	RECT WinDim,r;
	HPEN whitepen,blackpen,hOldPen,hPen;
	double dx,dy;
	int blackking=0,whiteman=0,whiteking=0,blackman=0;
	int jumps;
	double xmetric, ymetric;
	// load bitmaps from bmp.c
	HBITMAP bmp_bm, bmp_bk, bmp_wm, bmp_wk, bmp_manmask, bmp_kingmask;
	clock_t ticks;
	clock_t timer;
	int elapsed_ms;
	extern char str[1024];
	//extern enum state CBstate;
	int xoffset = 0;
	int yoffset = 0;
	int size;
	

	//return 0; 

	setanimationbusy(TRUE);

	bmp_bm = getCBbitmap(BMPBLACKMAN);
	bmp_bk = getCBbitmap(BMPBLACKKING);
	bmp_wm = getCBbitmap(BMPWHITEMAN);
	bmp_wk = getCBbitmap(BMPWHITEKING);
	bmp_manmask = getCBbitmap(BMPMANMASK);
	bmp_kingmask = getCBbitmap(BMPKINGMASK);

	blackpen = (HPEN) GetStockObject(BLACK_PEN);
	whitepen = (HPEN) GetStockObject(WHITE_PEN);					// getstockobject - object does not need to be destroyed
	maxX = GetSystemMetrics(SM_CXSCREEN);
	maxY = GetSystemMetrics(SM_CYSCREEN);


	getxymetrics(&xmetric, &ymetric, hwnd);
	size = xmetric;

	x = move.from.x;
	y = move.from.y;

	if(move.oldpiece==(BLACK|KING)) 
		blackking=1;
	if(move.oldpiece==(WHITE|MAN))  
		whiteman=1;
	if(move.oldpiece==(WHITE|KING)) 
		whiteking=1;
	if(move.oldpiece==(BLACK|MAN)) 
		blackman = 1;

	// remove pieces on from and to square, which 
	// would disturb animation
	board8[x][y]=0;
	x2=move.to.x;
	y2=move.to.y;
	board8[x2][y2]=0;

	// create a background image for the animation

	// PUT THIS BACK IN FOR ANIMATION!!
#ifdef ANIMATION
	printboard(hwnd, bgdc, bmpdc, stretchdc, board8);
#endif

	coorstocoors(&x,&y, gCBoptions.invert, gCBoptions.mirror);
	coorstocoors(&x2,&y2, gCBoptions.invert, gCBoptions.mirror);
	/* print the board without the stone which moves into bgdc */

	/* find the number of jumps to do */
	jumps = move.jumps;

#ifdef ANIMATION
	
	if(jumps == 0)
		// a normal move without jumps 
		{
		for(i=0;i<=STEPS;i++)
			{
			timer = clock();
			// find redrawing rectangle 
			dx=((double)(x2*i+x*(STEPS-i)))/(STEPS);
			dy=((double)(y2*i+y*(STEPS-i)))/(STEPS);
			r.left=(int)(size*(dx)+xoffset);
			r.top=(int)(size*(7-dy) + upperoffset+yoffset);
			r.right=(int)(size*(dx+1) + xoffset);
			r.bottom=(int)(size*(8-dy) + upperoffset+yoffset);


			// restore clean background
			BitBlt(memdc,r.left,r.top,r.right,r.bottom,bgdc,r.left,r.top,SRCCOPY);

			//----------------------------------------------
			// make the image of the mask
			// select mask
			if(whiteking || blackking)
				SelectObject(bmpdc, bmp_kingmask);
			else 
				SelectObject(bmpdc, bmp_manmask);
			// paint mask
			// 128,128 is the dimension of the bitmap.
			StretchBlt(memdc,r.left, r.top, size, size, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCAND);

			//----------------------------------------------
			// make image of the piece
			// select bmp
			if(blackman)
				SelectObject(bmpdc,bmp_bm);
			if(whiteman)
				SelectObject(bmpdc,bmp_wm);
			if(blackking)
				SelectObject(bmpdc,bmp_bk);
			if(whiteking)
				SelectObject(bmpdc,bmp_wk);
			// paint bmp
			StretchBlt(memdc,r.left, r.top, size, size, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCPAINT);

			//----------------------------------------------
			// tell windows to redraw
			r.left-=5;
			r.top-=5;
			r.right+=5;
			r.bottom+=5;
			InvalidateRect(hwnd,&r,0);
			SendMessage(hwnd, WM_PAINT, (WPARAM) 0, (LPARAM) 0);

			//-----------------------------------------------
			// wait a bit, depending on how fast the system is
			// get elapsed time up to now
			ticks = clock() - timer;
			// convert to miliseconds
			elapsed_ms = (ticks * 1000) / CLOCKS_PER_SEC;
			if(elapsed_ms < ANIMATIONSLEEPTIME)
				Sleep(ANIMATIONSLEEPTIME - elapsed_ms);
			//sprintf(str, "elapsed: %i, %i", elapsed_ms, ticks);
			}
		}
	else
		{
		// a jumping move 
		x = move.from.x;
		y = move.from.y;
		coorstocoors(&x,&y,gCBoptions.invert, gCBoptions.mirror);
		for(j=0;j<jumps;j++)
			{
			x2 = move.path[j+1].x;
			y2 = move.path[j+1].y;
			coorstocoors(&x2,&y2,gCBoptions.invert, gCBoptions.mirror);
			// now animate the part-move:
			for(i=0;i<=2*STEPS;i++)
				{
				timer = clock();
				// find redrawing region 
				dx=((double)(x2*i+x*(2*STEPS-i)))/(2*STEPS);
				dy=((double)(y2*i+y*(2*STEPS-i)))/(2*STEPS);
				r.left=(int)(size*(dx) + xoffset);
				r.top=(int)(size*(7-dy)+upperoffset+yoffset);
				r.right = r.left + size;
				r.bottom = r.top + size;



				// restore clean background
				BitBlt(memdc,r.left,r.top,r.right,r.bottom,bgdc,r.left,r.top,SRCCOPY);
				// select mask
				if(whiteking || blackking)
					SelectObject(bmpdc, bmp_kingmask);
				else 
					SelectObject(bmpdc, bmp_manmask);
				// paint mask from bmpdc to memdc
				//BitBlt(memdc,r.left,r.top,r.right,r.bottom,bmpdc,0,0,SRCAND);
				// 128,148 is the dimension of the bitmap.
				// stretchblt from bmpdc to memdc
				StretchBlt(memdc,r.left, r.top, size, size, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCAND);
				// select bmp
				if(blackman)
					SelectObject(bmpdc,bmp_bm);
				if(whiteman)
					SelectObject(bmpdc,bmp_wm);
				if(blackking)
					SelectObject(bmpdc,bmp_bk);
				if(whiteking)
					SelectObject(bmpdc,bmp_wk);

				// paint bmp
				//BitBlt(memdc,r.left,r.top,r.right,r.bottom,bmpdc,0,0,SRCPAINT);
				StretchBlt(memdc,r.left, r.top, size, size, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCPAINT);

				InvalidateRect(hwnd,&r,0);
				SendMessage(hwnd, WM_PAINT, (WPARAM) 0, (LPARAM) 0);
				//-----------------------------------------------
				// wait a bit, depending on how fast the system is
				// get elapsed time up to now
				ticks = clock() - timer;
				// convert to miliseconds
				elapsed_ms = (ticks * 1000) / CLOCKS_PER_SEC;
				if(elapsed_ms < ANIMATIONSLEEPTIME)
					Sleep(ANIMATIONSLEEPTIME - elapsed_ms);
				}
			x=x2;
			y=y2;
			}
		}

#endif // ANIMATION

	// make a clean image now
	// TODO: don't mix up game play with animation!
	domove(move, board8);
	printboard(hwnd, memdc, bmpdc, stretchdc, board8);

	// if move highlighting is on, we do it! 
	// this should go in a separate function!
	if(gCBoptions.highlight)
		{
		// create a pen:
		hPen = CreatePen(PS_SOLID, 1, gCBoptions.colors[0]);	

		x=move.from.x;
		y=move.from.y;
		x2=move.to.x;
		y2=move.to.y;
		coorstocoors(&x,&y,gCBoptions.invert, gCBoptions.mirror);
		coorstocoors(&x2,&y2,gCBoptions.invert, gCBoptions.mirror);
		hOldPen = (HPEN) SelectObject(memdc,hPen);

		MoveToEx(memdc,(int)(size*x + xoffset),(int)(size*(7-y)+upperoffset + yoffset),NULL);
		LineTo(memdc,(int)(size*(x+1)-1 + xoffset),(int)(size*(7-y)+upperoffset + yoffset));
		LineTo(memdc,(int)(size*(x+1)-1 + xoffset),(int)(size*(8-y)+upperoffset-1 + yoffset));
		LineTo(memdc,(int)(size*x + xoffset),(int)(size*(8-y)+upperoffset-1 + yoffset));
		LineTo(memdc,(int)(size*x + xoffset),(int)(size*(7-y)+upperoffset + yoffset));
		MoveToEx(memdc,(int)(size*x2 + xoffset),(int)(size*(7-y2)+upperoffset + yoffset),NULL);
		LineTo(memdc,(int)(size*(x2+1)-1 + xoffset),(int)(size*(7-y2)+upperoffset + yoffset));
		LineTo(memdc,(int)(size*(x2+1)-1 + xoffset),(int)(size*(8-y2)+upperoffset-1 + yoffset));
		LineTo(memdc,(int)(size*x2 + xoffset),(int)(size*(8-y2)+upperoffset-1  + yoffset));
		LineTo(memdc,(int)(size*x2 + xoffset),(int)(size*(7-y2)+upperoffset + yoffset));

		SelectObject(memdc,hOldPen);
		DeleteObject(hPen);
		}// end movehighlighting 


	// get redrawing region
	GetClientRect(hwnd, &WinDim);
	r.left = 0;
	r.right = WinDim.right;
	r.bottom = WinDim.bottom -(offset - upperoffset); // the thing in brackets is the status bar height
	r.top = upperoffset;

	InvalidateRect(hwnd,&r,0);

	// TODO: should not mix game state stuff with animation!
	color = color^CHANGECOLOR;

	setanimationbusy(FALSE);
	setenginestarting(FALSE);
	return 0;
	}

int getxymetrics(double *xmetric, double *ymetric, HWND hwnd)
	{
	// get board size
	RECT WinDim;
	int cxClient, cyClient;

	GetClientRect(hwnd, &WinDim);
	cxClient = WinDim.right - WinDim.left;
	cyClient = WinDim.bottom - WinDim.top;

	*xmetric = (int)(cxClient/8.0);
	*ymetric = (int)((cyClient - offset)/8.0);  

	return 1;
	}

void updatestretchDC(HWND hwnd, HDC bmpdc, HDC stretchdc, int size)
	{
	// updatestretchDC gets called whenever the CB window is resized.
	// it stretchblts pieces and masks into stretchdc, from where
	// printboard can then only bitblt them for faster graphics rendering.
	double xmetric, ymetric;
	HBITMAP bmp_bm, bmp_bk, bmp_wm, bmp_wk, bmp_manmask, bmp_kingmask, bmp_dark, bmp_light;
	HBITMAP hOldBitmap;				// old bitmap for restoration

	bmp_bm = getCBbitmap(BMPBLACKMAN);
	bmp_bk = getCBbitmap(BMPBLACKKING);
	bmp_wm = getCBbitmap(BMPWHITEMAN);
	bmp_wk = getCBbitmap(BMPWHITEKING);
	bmp_manmask = getCBbitmap(BMPMANMASK);
	bmp_kingmask = getCBbitmap(BMPKINGMASK);
	bmp_dark = getCBbitmap(BMPDARK);
	bmp_light = getCBbitmap(BMPLIGHT);

	getxymetrics(&xmetric, &ymetric, hwnd);

	SetStretchBltMode(stretchdc,HALFTONE);
	// stretchblt man mask, king mask, black man, white man, black king, white king
	// into positions 0,1,2,3,4,5 of stretchdc

	xmetric = size;

	hOldBitmap = (HBITMAP) SelectObject(bmpdc, bmp_manmask);
	StretchBlt(stretchdc, 8*xmetric, 0, xmetric, xmetric, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
	SelectObject(bmpdc, bmp_kingmask);
	StretchBlt(stretchdc, 8*xmetric, xmetric, xmetric, xmetric, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
	SelectObject(bmpdc, bmp_bm);
	StretchBlt(stretchdc, 8*xmetric, 2*xmetric, xmetric, xmetric, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
	SelectObject(bmpdc, bmp_wm);
	StretchBlt(stretchdc, 8*xmetric, 3*xmetric, xmetric, xmetric, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
	SelectObject(bmpdc, bmp_bk);
	StretchBlt(stretchdc, 8*xmetric, 4*xmetric, xmetric, xmetric, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
	SelectObject(bmpdc, bmp_wk);
	StretchBlt(stretchdc, 8*xmetric, 5*xmetric, xmetric, xmetric, bmpdc, 0,0,BMPSIZE,BMPSIZE,SRCCOPY);
	SelectObject(bmpdc, hOldBitmap);
	return;
	}


int printboard(HWND hwnd, HDC hdc, HDC bmpdc, HDC stretchdc, int b[8][8])
	{
	// printboard prints the board given in b[8][8] into hdc

	// a word about this: we get ourselves the size of the client rectangle of
	// our window. but: in this rectangle, there is a toolbar at the top (toolbarheight), and
	// a status window at the bottom (statusbarheight). therefore, the true y extent of the
	// graphics should be ysize-toolbarheight-statusbarheight

	// we use bitmaps to draw the board, which are loaded in the file bmp.c
	// the bitmaps have to be stretched to fit the board. this is an expensive
	// operation, therefore, the dark and light square bitmaps are strechblitted only
	// once onto one square each, and then copied from there on. 
	// the same should be done for the pieces, but i was too lazy to implement it right now
	// performance seems to be ok now, much better than when all squares were stretched.

	int i,j,x,y;					// loop variables for board
	char s[80];						// char to output coordinates
	double xmetric, ymetric;		// size of squares
	RECT WinDim, r;					// dimension
	// bitmaps for pieces/background below
	int xoffset = 0;
	int yoffset = 0;
	int size = 0;
	HFONT oldfont;

	if(hdc == NULL)
		{
		CBlog("hdc is null in printboard");
		return 0;
		}
	
	getxymetrics(&xmetric, &ymetric, hwnd);
	size = xmetric;

	// BUGFIX CB 1.651 - with the next line in place, it doesn't display pieces properly
	// after changing the piece set.
	//if(size != g_size)
		{
		updatestretchDC(hwnd,bmpdc,stretchdc,size);
		g_size = size;
		}

	SetStretchBltMode(hdc,HALFTONE);

	// bitblt background from boarddc
	BitBlt(hdc, xoffset, yoffset+upperoffset, size*8,size*8, boarddc, 0,0,SRCCOPY);

	for(i=0;i<=7;i++)
		{
		for(j=0;j<=7;j++)
			{
			x=i;
			y=j;
			coorstocoors(&x,&y, gCBoptions.invert, gCBoptions.mirror);
			if(b[x][y]==0)
				continue;

			// bitblt man mask, king mask, black man, white man, black king, white king
			// from positions 0,1,2,3,4,5 of stretchdc

			assert(b[x][y]==(BLACK|MAN) || b[x][y]==(BLACK|KING) || b[x][y]==(WHITE|MAN) || b[x][y]==(WHITE|KING));
			// masks
			if(b[x][y] & KING)
				BitBlt(hdc, size*i+xoffset, size*(7-j)+upperoffset+yoffset, size, size, stretchdc, 8*size, size,  SRCAND);
			else
				BitBlt(hdc, size*i+xoffset, size*(7-j)+upperoffset+yoffset, size, size, stretchdc, 8*size , 0, SRCAND);

			// pieces
			if(b[x][y] == (BLACK|MAN))
				BitBlt(hdc, size*i+xoffset, size*(7-j)+upperoffset+yoffset, size, size, stretchdc, 8*size , 2*size, SRCPAINT);

			if(b[x][y]== (WHITE|MAN))
				BitBlt(hdc, size*i+xoffset, size*(7-j)+upperoffset+yoffset, size, size, stretchdc, 8*size , 3*size, SRCPAINT);

			if(b[x][y]== (BLACK|KING))
				BitBlt(hdc, size*i+xoffset, size*(7-j)+upperoffset+yoffset, size, size, stretchdc, 8*size , 4*size, SRCPAINT);

			if(b[x][y]== (WHITE|KING))
				BitBlt(hdc, size*i+xoffset, size*(7-j)+upperoffset+yoffset, size, size, stretchdc, 8*size , 5*size, SRCPAINT);
			}
		}

	// add board numbers 
	if(gCBoptions.numbers)
		{
		oldfont = (HFONT) SelectObject(hdc,myfont);
		SetTextColor(hdc,gCBoptions.colors[1]);
		SetBkMode(hdc,TRANSPARENT);
		for(i=0;i<=7;i++)
			{
			for(j=0;j<=7;j++)
				{
				x=i;
				y=j;
				coorstocoors(&x,&y,gCBoptions.invert, gCBoptions.mirror);
				if(!((x+y)%2))
					{
					sprintf(s,"%i",coorstonumber(x,y, GPDNgame.gametype));
					TextOut(hdc,(int)(size*(i)+1+xoffset),(int)(size*(7-j)+upperoffset+yoffset),s,strlen(s));
					}
				}
			}
		SelectObject(hdc,oldfont);
		}

	// get redrawing region
	GetClientRect(hwnd, &WinDim);
	r.left = xoffset;
	r.right = r.left + 8*size;
	r.top = upperoffset+yoffset;
	r.bottom = r.top + 8*size;

	InvalidateRect(hwnd,&r,0);

	return 1;
	}


int printsampleboard(HWND hwnd, HDC hdc, HDC bmpdc, HDC stretchdc)
	{
	// is used as a "hidden function" to produce a 5x5 clipboard image of current piece
	// set.

	int i,j;						// loop variables for board
	double xmetric, ymetric;		// size of squares
	// bitmaps for pieces/background below
	HBITMAP bmp_bm, bmp_bk, bmp_wm, bmp_wk, bmp_manmask, bmp_kingmask, bmp_dark, bmp_light;
	int xoffset = 0, yoffset = 0;
	
	bmp_bm = getCBbitmap(BMPBLACKMAN);
	bmp_bk = getCBbitmap(BMPBLACKKING);
	bmp_wm = getCBbitmap(BMPWHITEMAN);
	bmp_wk = getCBbitmap(BMPWHITEKING);
	bmp_manmask = getCBbitmap(BMPMANMASK);
	bmp_kingmask = getCBbitmap(BMPKINGMASK);
	bmp_dark = getCBbitmap(BMPDARK);
	bmp_light = getCBbitmap(BMPLIGHT);

	if(hdc==NULL)
		{
		CBlog("hdc is null in printboard");
		return 0;
		}

	getxymetrics(&xmetric, &ymetric, hwnd);

	SetStretchBltMode(hdc,HALFTONE);

	// bitblt background from boarddc
	BitBlt(hdc, xoffset, yoffset+upperoffset, xmetric*8,xmetric*8, boarddc, 0,0,SRCCOPY);
	
	//kingmask
	i=0;
	j=7;
	BitBlt(hdc, xmetric*i, ymetric*(7-j)+upperoffset, xmetric, xmetric, stretchdc, 8*xmetric, xmetric,  SRCAND);
	// black king
	BitBlt(hdc, xmetric*i, ymetric*(7-j)+upperoffset, xmetric, xmetric, stretchdc, 8*xmetric , 3*xmetric, SRCPAINT);
	i=0;
	j=5;
	BitBlt(hdc, xmetric*i, ymetric*(7-j)+upperoffset, xmetric, xmetric, stretchdc, 8*xmetric, xmetric,  SRCAND);
	// white king
	BitBlt(hdc, xmetric*i, ymetric*(7-j)+upperoffset, xmetric, xmetric, stretchdc, 8*xmetric , 5*xmetric, SRCPAINT);

	//manmask	
	i=2;
	j=5;
	BitBlt(hdc, xmetric*i, ymetric*(7-j)+upperoffset, xmetric, xmetric, stretchdc, 8*xmetric , 0, SRCAND);
	//black man
	BitBlt(hdc, xmetric*i, ymetric*(7-j)+upperoffset, xmetric, xmetric, stretchdc, 8*xmetric , 2*xmetric, SRCPAINT);

	i=2;
	j=7;
	BitBlt(hdc, xmetric*i, ymetric*(7-j)+upperoffset, xmetric, xmetric, stretchdc, 8*xmetric , 0, SRCAND);
	//white man
	BitBlt(hdc, xmetric*i, ymetric*(7-j)+upperoffset, xmetric, xmetric, stretchdc, 8*xmetric , 4*xmetric, SRCPAINT);
	return 1;
	}

void selectstone(int x, int y, HWND hwnd, int board[8][8])
	{
	// new: when the user clicks on a stone, mark the square.
	RECT WinDim,r;
	HPEN hOldPen, hPen;
	double xmetric, ymetric;

	hPen = CreatePen(PS_SOLID, 1, gCBoptions.colors[0]);
	if(board[x][y]!=0)
		{
		coorstocoors(&x,&y,gCBoptions.invert, gCBoptions.mirror);
		getxymetrics(&xmetric, &ymetric, hwnd);

		hOldPen = (HPEN) SelectObject(memdc,hPen);

		MoveToEx(memdc,(int)(xmetric*x),(int)(ymetric*(7-y)+upperoffset),NULL);
		LineTo(memdc,(int)(xmetric*(x+1)-1),(int)(ymetric*(7-y)+upperoffset));
		LineTo(memdc,(int)(xmetric*(x+1)-1),(int)(ymetric*(8-y)+upperoffset)-1);
		LineTo(memdc,(int)(xmetric*x),(int)(ymetric*(8-y)+upperoffset)-1);
		LineTo(memdc,(int)(xmetric*x),(int)(ymetric*(7-y)+upperoffset));

		SelectObject(memdc,hOldPen);
		DeleteObject(hPen);

		// get redrawing region
		GetClientRect(hwnd, &WinDim);
		r.left = 0;
		r.right = WinDim.right;
		r.bottom = WinDim.bottom - (offset - upperoffset); // ( - ) = statusbarheight;
		r.top = upperoffset;

		InvalidateRect(hwnd,&r,0);
		}
	}
