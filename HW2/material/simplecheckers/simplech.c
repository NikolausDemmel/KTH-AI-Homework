/*______________________________________________________________________________

  ----------> name: simple checkers
  ----------> author: martin fierz
  ----------> purpose: platform independent checkers engine
  ----------> version: 1.11
  ----------> date: 8th october 98
  ----------> description: checkers.c contains a simple but fast checkers engine
  				  and a simple interface to this engine. checkers.c contains three
              main parts: interface, search and move generation. these parts are
              separated in the code. if you want to improve on the play of
              checkers, you will mainly have to improve on the evaluation
              function; this version does nothing but count material.
              even though it only counts pieces, this checkers engine will blow
              away all shareware checkers programs you will find on the internet,
              with the exception of blitz54, which is written for DOS.

              board representation: the standard checkers notation is

                  (white)
                32  31  30  29
              28  27  26  25
                24  23  22  21
              20  19  18  17
                16  15  14  13
              12  11  10   9
                 8   7   6   5
               4   3   2   1
                  (black)

              the internal representation of the board is different, it is a
              array of int with length 46, the checkers board is numbered
              like this:

                  (white)
                37  38  39  40
              32  33  34  35
                28  29  30  31
              23  24  25  26
                19  20  21  22
              14  15  16  17
                10  11  12  13
               5   6   7   8
                  (black)

              let's say, you would like to teach the program that it is
              important to keep a back rank guard. you can for instance
              add the following (not very sophisticated) code for this:

              if(b[6] & (BLACK|MAN)) eval++;
              if(b[8] & (BLACK|MAN)) eval++;
              if(b[37] & (WHITE|MAN)) eval--;
              if(b[39] & (WHITE|MAN)) eval--;

              the evaluation function is seen from the point of view of the
              black player, so you increase the value v if you think the
              position is good for black.

              if you want to program a different interface, you call the
              function "checkers":

              checkers(int b[46],int color, double maxtime, char *str);

              in b[46] you store the position, each square having one of
              the values of {FREE, BLACK|MAN, BLACK|KING, WHITE|MAN, WHITE|KING,
              OCCUPIED}. OCCUPIED are those squares which do not appear in the
              above board representation, i.e. 0,1,2,3,9,18,27,36,41-45.
              color is the color to move, either BLACK or WHITE.
              maxtime is the about half the average time it will take to
              calculate a move.

              after checkers completes, you will have the new board position in
              b[46] and some information on the search in str.

              initcheckers(int b[46]) initializes board b to the starting
              position of checkers.

              have fun!

              questions, comments, suggestions to:

              		Martin Fierz
              		mafierz@ibm.net

  ----------> choose your platform */
#define WIN95
#undef DOS
#undef MAC
#undef UNIX

#ifdef WIN95
#include <windows.h>
   unsigned int _stklen=50000;
#endif
#ifdef DOS
#define int long
	unsigned int _stklen=40000;
#endif
#ifdef MAC
#define CLK_TCK 60
#endif
#ifdef UNIX
#define CLK_TCK 1000000
#endif


/*----------> includes */
#include <stdio.h>
#include <string.h>
#include <time.h>

/*----------> definitions */
#define OCCUPIED 0
#define WHITE 1
#define BLACK 2
#define MAN 4
#define KING 8
#define FREE 16
#define CHANGECOLOR 3
#define MAXDEPTH 99
#define MAXMOVES 20

/*----------> compile options  */
#undef MUTE
#undef VERBOSE
#define STATISTICS
#undef SHOWLONGMOVES
#ifdef SHOWLONGMOVES
#define LONGMOVE 5
#endif

/*----------> structure definitions  */
struct move2
	{
   short n;
   int m[8];
   };

/*----------> function prototypes  */
/*----------> part I: interface */
void printboard(int b[46]);
void initcheckers(int b[46]);
void wait(double t);
void timerroutine(int b[46]);
void movetonotation(struct move2 move,char str[80]);
/*----------> part II: search */
int  checkers(int b[46],int color, double maxtime, char *str);
int  alphabeta(int b[46],int depth, int alpha, int beta, int color);
int  firstalphabeta(int b[46],int depth, int alpha, int beta, int color,struct move2 *best);
void domove(int b[46],struct move2 move);
void undomove(int b[46],struct move2 move);
int  evaluation(int b[46], int color);
/*----------> part III: move generation */
int  generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color);
int  generatecapturelist(int b[46], struct move2 movelist[MAXMOVES], int color);
void blackmancapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int square);
void blackkingcapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int square);
void whitemancapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int square);
void whitekingcapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int square);
int  testcapture(int b[46], int color);

/*----------> globals  */
#ifdef STATISTICS
int alphabetas,generatemovelists,evaluations,generatecapturelists,testcaptures;
#endif
int value[17]={0,0,0,0,0,1,256,0,0,16,4096,0,0,0,0,0,0};


/*-------------- PART I: INTERFACE -------------------------------------------*/

int main()
/*----------> purpose: provide a simple interface to checkers.
  ----------> version: 1.0
  ----------> date: 24th october 97 */
	{

   char str[80];
   int b[46];
   int choice=0;
   int n;
   struct move2 movelist[MAXMOVES];
   int i;
   double maxtime=5.0;

#ifdef WIN95
/*----------> WIN95: get a new console, set its title, input and output */
	FreeConsole();
	AllocConsole();
  	SetConsoleTitle("Checkers 99");
#endif

   printf("\nsimple checkers version 1.11");wait(0.1);
   printf("\n8th october 98");wait(0.1);
   printf("\nby martin fierz");wait(0.1);

   initcheckers(b);

   do
		{
      printboard(b);
      if(testcapture(b,BLACK))
			n=generatecapturelist(b, movelist, BLACK);
      else
			n=generatemovelist(b, movelist, BLACK);
      for(i=0;i<n;i++)
			{
	 		if( !(i%6)) printf("\n");
	 		movetonotation(movelist[i],str);
	 		printf("%li: %s   ",i+1,str);
	 		}
      fflush(stdin);
      printf("\nyour move? (0 exits, 99 tests)  ");
      do
			{
	 		scanf("%d",&choice);
			if(choice==99) {timerroutine(b);scanf("%d",&choice);}
	 		} while(choice==99);
      if(choice>0 && choice <=n)
			{
	 		domove(b,movelist[choice-1]);
   		}
   	else break;
      checkers(b,WHITE,maxtime,&str[0]);
      printf("%s",str);
      }
   while(1);
   printf("\nsimple checkers version 1.11");wait(0.1);
   printf("\n8th october 98");wait(0.1);
   printf("\nby martin fierz");wait(1.0);
   return(0);
   }

void movetonotation(struct move2 move,char str[80])
	{
	int j,from,to;
	char c;

	from=move.m[0]%256;
	to=move.m[1]%256;
	from=from-(from/9);
	to=to-(to/9);
	from-=5;
	to-=5;
	j=from%4;from-=j;j=3-j;from+=j;
	j=to%4;to-=j;j=3-j;to+=j;
	from++;
	to++;
	c='-';
	if(move.n>2) c='x';
	sprintf(str,"%2li%c%2li",from,c,to);
	}

void initcheckers(int b[46])
/*----------> purpose: initialize the checkerboard
  ----------> version: 1.0
  ----------> date: 24th october 97 */
	{
   int i;

   for(i=0;i<46;i++)
	b[i]=OCCUPIED;

   for(i=5;i<=40;i++)
   	b[i]=FREE;

   for(i=5;i<=17;i++)
      b[i]=(BLACK|MAN);

   for(i=28;i<=40;i++)
   	b[i]=(WHITE|MAN);

   for(i=9;i<=36;i+=9)
   	b[i]=OCCUPIED;
   }

void timerroutine(int b[46])
/*----------> purpose: time the other routines
  ----------> version: 1.0
  ----------> date: 24th october 97 */
	{

#ifdef VERBOSE
   	printf("\nno timing in verbose mode!");
#else
      int i;
	   double start,time;
      struct move2 movelist[20];
      double factor;
      int capture;

      start=clock();
   	for(i=0;i<1000000;i++)
      	evaluation(b,BLACK);
   	time=(clock()-start)/CLK_TCK;
      factor=(1.44/time);
   	printf("\n\nevaluations: time for 1M iterations is %2.2f s, %2.2f",time,factor);

      start=clock();
		for(i=0;i<1000000;i++)
      	capture=testcapture(b, BLACK);
   	time=(clock()-start)/CLK_TCK;
      factor=(2.44/time);
   	printf("\ntestcapture: time for 1M iterations is %2.2f s, %2.2f",time,factor);

      if(capture)
      	{
         start=clock();
			for(i=0;i<1000000;i++)
      		generatecapturelist(b, movelist, BLACK);
   		time=(clock()-start)/CLK_TCK;
   		printf("\ncapturelist: time for 1M iterations is %2.2f s",time);
         }
      else
      	{
      	start=clock();
			for(i=0;i<1000000;i++)
      		generatemovelist(b, movelist, BLACK);
         time=(clock()-start)/CLK_TCK;
      	factor=(2.44/time);
   		printf("\nmovelist: time for 1M iterations is %2.2f s, %2.2f",time,factor);
         }

      start=clock();
   	for(i=0;i<2000000;i++)
      	domove(b,movelist[0]);
   	time=(clock()-start)/CLK_TCK;
      factor=(0.79/time);
	printf("\ndomove: time for 2M iterations is %2.2f s, %2.2f\n\n",time,factor);
#endif
   }

void wait(double t)
	{
   double start;
   start=clock();
   do
   	{
      }
   while( ((clock()-start)/CLK_TCK)<t);
   }

void printboard(int b[46])
/*----------> purpose: print the checkerboard
  ----------> version: 1.0
  ----------> date: 24th october 97 */
	{
   int i,j;
   char c[18]="     wb  WB     --";

   printf("\n");
   for(i=37;i>=10;i-=9)
   	{
   	printf("\n ");
   	for(j=i;j<=i+3;j++)
   		printf(" %c",c[b[j]]);
   	printf("\n ");
   	for(j=i-5;j<=i-2;j++)
		printf("%c ",c[b[j]]);
   	}
   printf("\n");
   }

/*-------------- PART II: SEARCH ---------------------------------------------*/


int  checkers(int b[46],int color, double maxtime, char *str)
/*----------> purpose: entry point to checkers. find a move on board b for color
  ---------->          in the time specified by maxtime, write the best move in
  ---------->          board, returns information on the search in str
  ----------> returns 1 if a move is found & executed, 0, if there is no legal
  ----------> move in this position.
  ----------> version: 1.1
  ----------> date: 9th october 98 */
	{
   int i,numberofmoves;
   double start;
   int eval;
   struct move2 best,movelist[MAXMOVES];

#ifdef STATISTICS
	alphabetas=0;
   generatemovelists=0;
   generatecapturelists=0;
   evaluations=0;
#endif

/*--------> check if there is only one move */
	numberofmoves=generatecapturelist(b,movelist,color);
   if(numberofmoves==1) {domove(b,movelist[0]);sprintf(str,"forced capture");return(1);}
   else
   	{
   	numberofmoves=generatemovelist(b,movelist,color);
   	if(numberofmoves==1) {domove(b,movelist[0]);sprintf(str,"only move");return(1);}
   	if(numberofmoves==0) {sprintf(str,"no legal moves in this position");return(0);}
      }
   start=clock();
   eval=firstalphabeta(b,1,-10000,10000,color,&best);
   for(i=2;(i<=MAXDEPTH) && ( (clock()-start)/CLK_TCK < maxtime );i++)
		{
      eval=firstalphabeta(b,i,-10000,10000,color,&best);
#ifndef MUTE
	 	printf("\nt %2.2f, d %2li, v %4li",(clock()-start)/CLK_TCK,i,eval);
#ifdef STATISTICS
	 	printf("  nod %li, gm %li, gc %li, eva %li",
		alphabetas,generatemovelists,generatecapturelists,
		evaluations);
#endif
#endif
      }
   i--;
#ifdef STATISTICS
	sprintf(str,"\n\nt %2.2f, d %2li, v %4li  nod %li, gm %li, gc %li, eva %li",(clock()-start)/CLK_TCK,i,eval,alphabetas,generatemovelists,generatecapturelists,evaluations);
#else
   sprintf(str,"\\nntime %2.2f, depth %li, eval %li",(clock()-start)/CLK_TCK,i,eval);
#endif
   domove(b,best);
   return(1);
   }

int firstalphabeta(int b[46], int depth, int alpha, int beta, int color, struct move2 *best)
/*----------> purpose: search the game tree and find the best move.
  ----------> version: 1.0
  ----------> date: 25th october 97 */
	{
   int i;
   int value;
   int numberofmoves;
   int capture;
   struct move2 movelist[MAXMOVES];

#ifdef STATISTICS
	alphabetas++;
#endif

/*----------> test if captures are possible */
	capture=testcapture(b,color);

/*----------> recursion termination if no captures and depth=0*/
   if(depth==0)
   	{
      if(capture==0)
      	return(evaluation(b,color));
      else
			depth=1;
      }

/*----------> generate all possible moves in the position */
   if(capture==0)
      {
		numberofmoves=generatemovelist(b,movelist,color);
/*----------> if there are no possible moves, we lose: */
		if(numberofmoves==0)  {if (color==BLACK) return(-5000); else return(5000);}
		}
   else
		numberofmoves=generatecapturelist(b,movelist,color);

/*----------> for all moves: execute the move, search tree, undo move. */
   for(i=0;i<numberofmoves;i++)
		{
      domove(b,movelist[i]);

      value=alphabeta(b,depth-1,alpha,beta,(color^CHANGECOLOR));

      undomove(b,movelist[i]);
      if(color == BLACK)
			{
			if(value>=beta) return(value);
			if(value>alpha) {alpha=value;*best=movelist[i];}
	 		}
      if(color == WHITE)
			{
	 		if(value<=alpha) return(value);
	 		if(value<beta)   {beta=value;*best=movelist[i];}
	 		}
      }
   if(color == BLACK)
		return(alpha);
	return(beta);
   }

int alphabeta(int b[46], int depth, int alpha, int beta, int color)
/*----------> purpose: search the game tree and find the best move.
  ----------> version: 1.0
  ----------> date: 24th october 97 */
	{
   int i;
   int value;
   int capture;
   int numberofmoves;
   struct move2 movelist[MAXMOVES];

#ifdef STATISTICS
	alphabetas++;
#endif

/*----------> test if captures are possible */
	capture=testcapture(b,color);

/*----------> recursion termination if no captures and depth=0*/
   if(depth==0)
		{
      if(capture==0)
			return(evaluation(b,color));
      else
			depth=1;
      }

/*----------> generate all possible moves in the position */
   if(capture==0)
      {
		numberofmoves=generatemovelist(b,movelist,color);
/*----------> if there are no possible moves, we lose: */
		if(numberofmoves==0)  {if (color==BLACK) return(-5000); else return(5000);}
		}
   else
		numberofmoves=generatecapturelist(b,movelist,color);

/*----------> for all moves: execute the move, search tree, undo move. */
   for(i=0;i<numberofmoves;i++)
		{
      domove(b,movelist[i]);

      value=alphabeta(b,depth-1,alpha,beta,color^CHANGECOLOR);

      undomove(b,movelist[i]);

      if(color == BLACK)
			{
			if(value>=beta) return(value);
			if(value>alpha) alpha=value;
	 		}
      if(color == WHITE)
			{
	 		if(value<=alpha) return(value);
	 		if(value<beta)   beta=value;
	 		}
      }
   if(color == BLACK)
		return(alpha);
	return(beta);
   }

void domove(int b[46],struct move2 move)
/*----------> purpose: execute move on board
  ----------> version: 1.1
  ----------> date: 25th october 97 */
	{
   int square,after;
   int i;

   for(i=0;i<move.n;i++)
	{
      square=(move.m[i] % 256);
      after=((move.m[i]>>16) % 256);
      b[square]=after;
      }
   }

void undomove(int b[46],struct move2 move)
/*----------> purpose:
  ----------> version: 1.1
  ----------> date: 25th october 97 */
	{
   int square,before;
   int i;

   for(i=0;i<move.n;i++)
   	{
      square=(move.m[i] % 256);
      before=((move.m[i]>>8) % 256);
      b[square]=before;
      }
   }

int evaluation(int b[46], int color)
/*----------> purpose:
  ----------> version: 1.1
  ----------> date: 18th april 98 */
	{
   int i;
   int eval;
   int v1,v2;
   int nbm,nbk,nwm,nwk;

   int code=0;



#ifdef STATISTICS
	evaluations++;
#endif


   for(i=5;i<=40;i++)
	code+=value[b[i]];

   nwm = code % 16;
   nwk = (code>>4) % 16;
   nbm = (code>>8) % 16;
   nbk = (code>>12) % 16;


   v1=100*nbm+130*nbk;
   v2=100*nwm+130*nwk;

   eval=v1-v2;                       /*material values*/
   eval+=(250*(v1-v2))/(v1+v2);      /*favor exchanges if in material plus*/

   return(eval);
   }



/*-------------- PART III: MOVE GENERATION -----------------------------------*/

int generatemovelist(int b[46], struct move2 movelist[MAXMOVES], int color)
/*----------> purpose:generates all moves. no captures. returns number of moves
  ----------> version: 1.0
  ----------> date: 25th october 97 */
	{
   int n=0,m;
   int i;

#ifdef STATISTICS
	generatemovelists++;
#endif

   if(color == BLACK)
   	{
      for(i=5;i<=40;i++)
      	{
         if( (b[i]&BLACK) !=0 )
         	{
            if( (b[i]&MAN) !=0 )
            	{
               if( (b[i+4] & FREE) !=0 )
               	{
						movelist[n].n=2;
                  if(i>=32) m=(BLACK|KING); else m=(BLACK|MAN); m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i+4;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
					  	m+=(BLACK|MAN);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               if( (b[i+5] & FREE) !=0 )
               	{
                  movelist[n].n=2;
                  if(i>=32) m=(BLACK|KING); else m=(BLACK|MAN); m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i+5;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(BLACK|MAN);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               }
            if( (b[i]&KING) !=0 )
					{
               if( (b[i+4] & FREE) !=0 )
               	{
						movelist[n].n=2;
                  m=(BLACK|KING);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i+4;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(BLACK|KING);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               if( (b[i+5] & FREE) !=0 )
               	{
                  movelist[n].n=2;
                  m=(BLACK|KING);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i+5;
		  				movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(BLACK|KING);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               if( (b[i-4] & FREE) !=0 )
               	{
						movelist[n].n=2;
                  m=(BLACK|KING);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i-4;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(BLACK|KING);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
	       if( (b[i-5] & FREE) !=0 )
               	{
                  movelist[n].n=2;
                  m=(BLACK|KING);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i-5;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(BLACK|KING);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               }
            }
         }
      }
   else    /* color = WHITE */
		{
      for(i=5;i<=40;i++)
			{
         if( (b[i]&WHITE) !=0 )
         	{
            if( (b[i]&MAN) !=0 )
            	{
               if( (b[i-4] & FREE) !=0 )
               	{
                  movelist[n].n=2;
                  if(i<=13) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i-4;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(WHITE|MAN);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               if( (b[i-5] & FREE) !=0 )
               	{
		  				movelist[n].n=2;
                  if(i<=13) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i-5;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(WHITE|MAN);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
		  				}
               }
            if( (b[i]&KING) !=0 )  /* or else */
            	{
               if( (b[i+4] & FREE) !=0 )
               	{
						movelist[n].n=2;
                  m=(WHITE|KING);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i+4;
		  				movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(WHITE|KING);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               if( (b[i+5] & FREE) !=0 )
               	{
                  movelist[n].n=2;
                  m=(WHITE|KING);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i+5;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(WHITE|KING);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
	       		if( (b[i-4] & FREE) !=0 )
               	{
						movelist[n].n=2;
                  m=(WHITE|KING);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i-4;
                  movelist[n].m[1]=m;
                  m=FREE;m=m<<8;
                  m+=(WHITE|KING);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               if( (b[i-5] & FREE) !=0 )
               	{
                  movelist[n].n=2;
                  m=(WHITE|KING);m=m<<8;
                  m+=FREE;m=m<<8;
                  m+=i-5;
                  movelist[n].m[1]=m;
		  				m=FREE;m=m<<8;
                  m+=(WHITE|KING);m=m<<8;
                  m+=i;
                  movelist[n].m[0]=m;
                  n++;
                  }
               }
            }
         }
      }
   return(n);
   }

int  generatecapturelist(int b[46], struct move2 movelist[MAXMOVES], int color)
/*----------> purpose: generate all possible captures
  ----------> version: 1.0
  ----------> date: 25th october 97 */
  	{
   int n=0;
   int m;
   int i;
   int tmp;

#ifdef STATISTICS
  	generatecapturelists++;
#endif

   if(color == BLACK)
   	{
      for(i=5;i<=40;i++)
      	{
         if( (b[i] & BLACK) !=0)
         	{
            if( (b[i] & MAN) !=0)
            	{
               if( (b[i+4] & WHITE) !=0)
               	{
                  if( (b[i+8] & FREE) !=0)
                  	{
                     movelist[n].n=3;
							if(i>=28) m=(BLACK|KING); else m=(BLACK|MAN);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i+8;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(BLACK|MAN);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i+4];m=m<<8;
                     m+=i+4;
                     movelist[n].m[2]=m;
                  	blackmancapture(b, &n, movelist, i+8);
                     }
                  }
               if( (b[i+5] & WHITE) !=0)
               	{
                  if( (b[i+10] & FREE) !=0)
							{
                     movelist[n].n=3;
                  	if(i>=28) m=(BLACK|KING); else m=(BLACK|MAN);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i+10;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(BLACK|MAN);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i+5];m=m<<8;
                     m+=i+5;
                     movelist[n].m[2]=m;
                  	blackmancapture(b, &n, movelist, i+10);
                     }
                  }
               }
		else /* b[i] is a KING */
         		{
            	if( (b[i+4] & WHITE) !=0)
               	{
                  if( (b[i+8] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	m=(BLACK|KING);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i+8;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(BLACK|KING);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i+4];m=m<<8;
                     m+=i+4;
                     movelist[n].m[2]=m;
                     tmp=b[i+4];
		     				b[i+4]=FREE;
                  	blackkingcapture(b, &n, movelist, i+8);
                     b[i+4]=tmp;
                     }
                  }
               if( (b[i+5] & WHITE) !=0)
               	{
                  if( (b[i+10] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	m=(BLACK|KING);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i+10;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(BLACK|KING);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i+5];m=m<<8;
		    				 m+=i+5;
                     movelist[n].m[2]=m;
                     tmp=b[i+5];
                     b[i+5]=FREE;
                  	blackkingcapture(b, &n, movelist, i+10);
                     b[i+5]=tmp;
                     }
                  }
               if( (b[i-4] & WHITE) !=0)
               	{
                  if( (b[i-8] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	m=(BLACK|KING);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i-8;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(BLACK|KING);m=m<<8;
                  	m+=i;
							movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i-4];m=m<<8;
                     m+=i-4;
                     movelist[n].m[2]=m;
                  	tmp=b[i-4];
                     b[i-4]=FREE;
                  	blackkingcapture(b, &n, movelist, i-8);
                     b[i-4]=tmp;
                     }
                  }
               if( (b[i-5] & WHITE) !=0)
               	{
                  if( (b[i-10] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	m=(BLACK|KING);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i-10;
                  	movelist[n].m[1]=m;
							m=FREE;m=m<<8;
                  	m+=(BLACK|KING);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i-5];m=m<<8;
                     m+=i-5;
                     movelist[n].m[2]=m;
                  	tmp=b[i-5];
                     b[i-5]=FREE;
                  	blackkingcapture(b, &n, movelist, i-10);
                     b[i-5]=tmp;
                     }
                  }
               }
            }
         }
      }
   else /* color is WHITE */
      {
      for(i=5;i<=40;i++)
      	{
         if( (b[i] & WHITE) !=0)
         	{
            if( (b[i] & MAN) !=0)
            	{
               if( (b[i-4] & BLACK) !=0)
               	{
                  if( (b[i-8] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	if(i<=17) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i-8;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(WHITE|MAN);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
		    	 			m+=b[i-4];m=m<<8;
                     m+=i-4;
                     movelist[n].m[2]=m;
                  	whitemancapture(b, &n, movelist, i-8);
                     }
                  }
               if( (b[i-5] & BLACK) !=0)
               	{
                  if( (b[i-10] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	if(i<=17) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i-10;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(WHITE|MAN);m=m<<8;
							m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i-5];m=m<<8;
                     m+=i-5;
                     movelist[n].m[2]=m;
                  	whitemancapture(b, &n, movelist, i-10);
                     }
                  }
               }
         	else /* b[i] is a KING */
         		{
            	if( (b[i+4] & BLACK) !=0)
               	{
                  if( (b[i+8] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	m=(WHITE|KING);m=m<<8;
							m+=FREE;m=m<<8;
                  	m+=i+8;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(WHITE|KING);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i+4];m=m<<8;
                     m+=i+4;
                     movelist[n].m[2]=m;
                  	tmp=b[i+4];
                     b[i+4]=FREE;
                  	whitekingcapture(b, &n, movelist, i+8);
                     b[i+4]=tmp;
                     }
                  }
               if( (b[i+5] & BLACK) !=0)
               	{
                  if( (b[i+10] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	m=(WHITE|KING);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i+10;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(WHITE|KING);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i+5];m=m<<8;
                     m+=i+5;
                     movelist[n].m[2]=m;
                     tmp=b[i+5];
                     b[i+5]=FREE;
                  	whitekingcapture(b, &n, movelist, i+10);
                     b[i+5]=tmp;
                     }
                  }
	       		if( (b[i-4] & BLACK) !=0)
               	{
                  if( (b[i-8] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	m=(WHITE|KING);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i-8;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(WHITE|KING);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i-4];m=m<<8;
                     m+=i-4;
                     movelist[n].m[2]=m;
                     tmp=b[i-4];
                     b[i-4]=FREE;
                  	whitekingcapture(b, &n, movelist, i-8);
                     b[i-4]=tmp;
                     }
                  }
               if( (b[i-5] & BLACK) !=0)
               	{
                  if( (b[i-10] & FREE) !=0)
                  	{
                     movelist[n].n=3;
                  	m=(WHITE|KING);m=m<<8;
                  	m+=FREE;m=m<<8;
                  	m+=i-10;
                  	movelist[n].m[1]=m;
                  	m=FREE;m=m<<8;
                  	m+=(WHITE|KING);m=m<<8;
                  	m+=i;
                  	movelist[n].m[0]=m;
                     m=FREE;m=m<<8;
                     m+=b[i-5];m=m<<8;
                     m+=i-5;
                     movelist[n].m[2]=m;
                  	tmp=b[i-5];
                     b[i-5]=FREE;
                  	whitekingcapture(b, &n, movelist, i-10);
                     b[i-5]=tmp;
                     }
                  }
               }
            }
         }
      }
   return(n);
  	}

void  blackmancapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int i)
	{
   int m;
   int found=0;
   struct move2 move,orgmove;

   orgmove=movelist[*n];
   move=orgmove;

   if( (b[i+4] & WHITE) !=0)
     	{
      if( (b[i+8] & FREE) !=0)
       	{
         move.n++;
        	if(i>=28) m=(BLACK|KING); else m=(BLACK|MAN);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=(i+8);
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i+4];m=m<<8;
         m+=(i+4);
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         blackmancapture(b, n, movelist, i+8);
	      }
      }
   move=orgmove;
   if( (b[i+5] & WHITE) !=0)
     	{
      if( (b[i+10] & FREE) !=0)
       	{
         move.n++;
        	if(i>=28) m=(BLACK|KING); else m=(BLACK|MAN);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=(i+10);
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i+5];m=m<<8;
         m+=(i+5);
	 		move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         blackmancapture(b, n, movelist, i+10);
         }
      }
   if(!found) (*n)++;
   }

void  blackkingcapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int i)
	{
   int m;
   int tmp;
   int found=0;
   struct move2 move,orgmove;

   orgmove=movelist[*n];
   move=orgmove;

   if( (b[i-4] & WHITE) !=0)
     	{
      if( (b[i-8] & FREE) !=0)
       	{
         move.n++;
        	m=(BLACK|KING);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i-8;
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i-4];m=m<<8;
         m+=i-4;
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
			tmp=b[i-4];
			b[i-4]=FREE;
         blackkingcapture(b, n, movelist, i-8);
         b[i-4]=tmp;
         }
      }
   move=orgmove;
   if( (b[i-5] & WHITE) !=0)
     	{
      if( (b[i-10] & FREE) !=0)
       	{
         move.n++;
        	m=(BLACK|KING);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i-10;
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i-5];m=m<<8;
         m+=i-5;
	 		move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         tmp=b[i-5];
      	b[i-5]=FREE;
         blackkingcapture(b, n, movelist, i-10);
         b[i-5]=tmp;
	      }
      }
	move = orgmove;
   if( (b[i+4] & WHITE) !=0)
     	{
      if( (b[i+8] & FREE) !=0)
       	{
         move.n++;
        	m=(BLACK|KING);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i+8;
			move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i+4];m=m<<8;
         m+=i+4;
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         tmp=b[i+4];
      	b[i+4]=FREE;
         blackkingcapture(b, n, movelist, i+8);
         b[i+4]=tmp;
         }
      }
   move=orgmove;
   if( (b[i+5] & WHITE) !=0)
     	{
      if( (b[i+10] & FREE) !=0)
			{
         move.n++;
        	m=(BLACK|KING);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i+10;
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i+5];m=m<<8;
         m+=i+5;
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         tmp=b[i+5];
      	b[i+5]=FREE;
         blackkingcapture(b, n, movelist, i+10);
	 		b[i+5]=tmp;
         }
      }
   if(!found) (*n)++;
   }

void  whitemancapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int i)
	{
   int m;
   int found=0;
   struct move2 move,orgmove;

   orgmove=movelist[*n];
   move=orgmove;

   if( (b[i-4] & BLACK) !=0)
     	{
      if( (b[i-8] & FREE) !=0)
       	{
         move.n++;
        	if(i<=17) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i-8;
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i-4];m=m<<8;
         m+=i-4;
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         whitemancapture(b, n, movelist, i-8);
         }
      }
   move=orgmove;
   if( (b[i-5] & BLACK) !=0)
     	{
      if( (b[i-10] & FREE) !=0)
       	{
         move.n++;
			if(i<=17) m=(WHITE|KING); else m=(WHITE|MAN);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i-10;
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i-5];m=m<<8;
         m+=i-5;
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         whitemancapture(b, n, movelist, i-10);
         }
      }
   if(!found) (*n)++;
   }

void  whitekingcapture(int b[46],  int *n, struct move2 movelist[MAXMOVES],int i)
	{
   int m;
   int tmp;
   int found=0;
   struct move2 move,orgmove;

   orgmove=movelist[*n];
   move=orgmove;

   if( (b[i-4] & BLACK) !=0)
     	{
      if( (b[i-8] & FREE) !=0)
       	{
	 		move.n++;
        	m=(WHITE|KING);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i-8;
        	move.m[1]=m;
	 		m=FREE;m=m<<8;
         m+=b[i-4];m=m<<8;
         m+=i-4;
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         tmp=b[i-4];
      	b[i-4]=FREE;
         whitekingcapture(b, n, movelist, i-8);
         b[i-4]=tmp;
         }
      }
   move=orgmove;
   if( (b[i-5] & BLACK) !=0)
     	{
      if( (b[i-10] & FREE) !=0)
       	{
         move.n++;
			m=(WHITE|KING);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i-10;
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i-5];m=m<<8;
         m+=i-5;
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         tmp=b[i-5];
      	b[i-5]=FREE;
	 		whitekingcapture(b, n, movelist, i-10);
         b[i-5]=tmp;
         }
      }
   move=orgmove;
   if( (b[i+4] & BLACK) !=0)
     	{
      if( (b[i+8] & FREE) !=0)
       	{
         move.n++;
        	m=(WHITE|KING);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i+8;
        	move.m[1]=m;
         m=FREE;m=m<<8;
         m+=b[i+4];m=m<<8;
         m+=i+4;
         move.m[move.n-1]=m;
	 		found=1;
         movelist[*n]=move;
         tmp=b[i+4];
      	b[i+4]=FREE;
         whitekingcapture(b, n, movelist, i+8);
	 		b[i+4]=tmp;
         }
      }
   move=orgmove;
   if( (b[i+5] & BLACK) !=0)
     	{
      if( (b[i+10] & FREE) !=0)
       	{
         move.n++;
        	m=(WHITE|KING);m=m<<8;
       	m+=FREE;m=m<<8;
       	m+=i+10;
        	move.m[1]=m;
	 		m=FREE;m=m<<8;
         m+=b[i+5];m=m<<8;
         m+=i+5;
         move.m[move.n-1]=m;
         found=1;
         movelist[*n]=move;
         tmp=b[i+5];
      	b[i+5]=FREE;
         whitekingcapture(b, n, movelist, i+10);
         b[i+5]=tmp;
         }
      }
   if(!found) (*n)++;
   }

int  testcapture(int b[46], int color)
/*----------> purpose: test if color has a capture on b
  ----------> version: 1.0
  ----------> date: 25th october 97 */
  	{
   int i;

#ifdef STATISTICS
  	testcaptures++;
#endif

  	if(color == BLACK)
   	{
      for(i=5;i<=40;i++)
      	{
         if( (b[i] & BLACK) !=0)
         	{
            if( (b[i] & MAN) !=0)
            	{
               if( (b[i+4] & WHITE) !=0)
               	{
                  if( (b[i+8] & FREE) !=0)
                  	return(1);
		  				}
               if( (b[i+5] & WHITE) !=0)
               	{
                  if( (b[i+10] & FREE) !=0)
                  	return(1);
                  }
	       		}
            else /* b[i] is a KING */
            	{
               if( (b[i+4] & WHITE) !=0)
               	{
                  if( (b[i+8] & FREE) !=0)
                  	return(1);
                  }
               if( (b[i+5] & WHITE) !=0)
               	{
                  if( (b[i+10] & FREE) !=0)
                  	return(1);
                  }
               if( (b[i-4] & WHITE) !=0)
						{
                  if( (b[i-8] & FREE) !=0)
                  	return(1);
                  }
               if( (b[i-5] & WHITE) !=0)
               	{
                  if( (b[i-10] & FREE) !=0)
                  	return(1);
                  }
               }
            }
         }
      }
   else /* color is WHITE */
   	{
      for(i=5;i<=40;i++)
      	{
         if( (b[i] & WHITE) !=0)
         	{
            if( (b[i] & MAN) !=0)
					{
               if( (b[i-4] & BLACK) !=0)
               	{
                  if( (b[i-8] & FREE) !=0)
                  	return(1);
                  }
               if( (b[i-5] & BLACK) !=0)
               	{
                  if( (b[i-10] & FREE) !=0)
                  	return(1);
                  }
               }
            else /* b[i] is a KING */
            	{
               if( (b[i+4] & BLACK) !=0)
               	{
                  if( (b[i+8] & FREE) !=0)
                  	return(1);
                  }
               if( (b[i+5] & BLACK) !=0)
               {
                  if( (b[i+10] & FREE) !=0)
                  	return(1);
                  }
               if( (b[i-4] & BLACK) !=0)
               	{
		  				if( (b[i-8] & FREE) !=0)
                  	return(1);
                  }
               if( (b[i-5] & BLACK) !=0)
               	{
                  if( (b[i-10] & FREE) !=0)
                  	return(1);
                  }
               }
            }
         }
      }
   return(0);
	}
