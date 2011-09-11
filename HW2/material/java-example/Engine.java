
import java.util.*;


class Engine 
{

final static int INFINITY = Integer.MAX_VALUE;
final static int CHECKER = 100; //one checker worth 100  
final static int POS=1;  //one position along the -j worth 1
final static int KING=200; //a king's worth
final static int EDGE=10; // effect of king being on the edge
final static int RANDOM_WEIGHT=10; // weight factor

public static int Evaluation(int[][] position)
  {
	
    int score=0;
    
    for (int i=0; i<8; i++)
      for (int j=0; j<8; j++)
	  {
	    if (position[i][j] == Checkers.WHITE)
	      {
			score-=CHECKER;
			score-=POS*j*j;
	      }
	    
	    else if (position[i][j] ==Checkers.WKING){
	      score-=KING;
		  if (i==0 || i==7)
			  score += EDGE;
		  if (j==0 || j==7)
			  score += EDGE;
	    }

	    else if (position[i][j] == Checkers.BKING){
	      score+=KING;
		  if (i==0 || i==7)
			  score -= EDGE;
		  if (j==0 || j==7)
			  score -= EDGE;
	    }
	    
	    else if (position[i][j] == Checkers.BLACK)
	    {
			score+=CHECKER;
			score+=POS*(7-j)*(7-j); 
	    }  
	  }//end for
        score += (int)(Math.random() * RANDOM_WEIGHT);
    return score;
  }//end Evaluation

  static int opponent(int turn){
	return turn==Checkers.BLACK ? Checkers.WHITE : Checkers.BLACK;
  }

  static int which_turn(int turn) {
    return Move.color(turn)==Checkers.BLACK ? -INFINITY : INFINITY;
  }
  
  public static int MiniMax(int[][] board, int depth, int max_depth, int[] the_move, int toMove, int[] counter)
  {
	  return MiniMax(board,depth,max_depth,the_move,toMove,counter,INFINITY,-INFINITY);
  }
  
  static int MiniMax(int[][] board, int depth, int max_depth,int[] the_move, int turn, int[] counter, int white_best, int black_best)
  {
	int the_score=0;
    int[][] new_board=new int[8][8];
    int best_score, chosen_score;
    int[] best_move=new int[4];
	
	Vector moves_list = new Vector();  //vector of 4x1 arrays
    
	Thread.yield();

	//assumes that depth is never equal to max_depth to begin with since
    //chosen_move is not set here
    if (depth==max_depth) 
	{
    	best_score = Evaluation(board);
		counter[0]++;
	}
    else {
		moves_list = Move.generate_moves(board,turn);
		best_score=which_turn(turn);
		switch (moves_list.size())
		{
		case 0:
			counter[0]++;
			return best_score;
		case 1:
		  if (depth == 0)  
		  {
			  // forced move: immediately return control
			best_move = (int[])moves_list.elementAt(0);
			for (int k=0; k<4; k++)
				the_move[k]=best_move[k];
			return 0;
		  }
		  else
		  {
			  // extend search since there is a forcing move
			  max_depth += 1;
		  }
		}    

		for (int i=0;i<moves_list.size();i++){
		new_board = copy_board(board);  //board need not be touched
        Move.move_board(new_board, (int[])moves_list.elementAt(i)); //returns new_board
		int temp[] = new int[4];
		the_score= MiniMax(new_board, depth+1, max_depth, temp, opponent(turn), counter, white_best, black_best);
	
		if (turn==Checkers.BLACK && the_score > best_score) {
			best_move = (int[])moves_list.elementAt(i);
			best_score = the_score;
			if (best_score > black_best) 
			{
				if (best_score >= white_best) 
					break;  /*  alpha_beta cutoff  */
				else
					black_best = best_score;  //the_score
			}
		}
	
		else if (turn==Checkers.WHITE && the_score < best_score) {
			best_move = (int[])moves_list.elementAt(i);
			best_score = the_score;
			if (best_score < white_best) 
			{
				if (best_score <= black_best) 
					break;  /*  alpha_beta cutoff  */
				else
				 	white_best = best_score;  //the_score
			}
		}
	
    } //end for

	}//end else
    for (int k=0; k<4; k++)
    	the_move[k]=best_move[k];
    return best_score;
  } //end minimax
  
  static int[][] copy_board(int[][] board){
    int[][] copy = new int[8][8];

    for (int i=0; i<8; i++)
      System.arraycopy(board[i],0,copy[i],0,8);
    return copy;
  }//end copy_board
  
  static boolean better(int the_score, int best, int turn){
    if (turn==Checkers.BLACK )
		return the_score>best;
	return the_score<best;
  }//end better           
}//end class engine

