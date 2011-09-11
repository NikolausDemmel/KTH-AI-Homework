import java.util.*;

class Move 
{
        final static int LEGALMOVE = 1;
        final static int ILLEGALMOVE = 2;
        final static int INCOMPLETEMOVE = 3;
        
        static boolean noMovesLeft(int[][] position,int toMove)
        {
                for (int i=0; i<8; i++)
                        for (int j=0; j<8; j++)
                if ( (float)(i+j)/2 != (i+j)/2 )
                {
                        if (toMove == Checkers.WHITE &&
                                (position[i][j] == Checkers.WHITE ||
                                position[i][j] == Checkers.WKING))
                        {
                                if (canWalk(position,i,j)) return false;
                                else if (canCapture(position,i,j)) return false;
                        }
                        else if (toMove == Checkers.BLACK &&
                                (position[i][j] == Checkers.BLACK ||
                                position[i][j] == Checkers.BKING))
                        {
                                if (canWalk(position,i,j)) return false;
                                else if (canCapture(position,i,j)) return false;
                        }
                } // if and for
                return true;
        }
                        


        // ApplyMove checks if the move entered is legal, illegal,
        // or incomplete.

        // If IsMoveLegal returns INCOMPLETEMOVE, this means a capture has just been made.
        // Call canCapture() to see     if a further capture is possible.
        static int ApplyMove(int[][] position,int start_i,int start_j,int end_i,int end_j)
        {
		        int result = IsMoveLegal(position,start_i,start_j,end_i,end_j,color(position[start_i][start_j]));
                if (result != ILLEGALMOVE)
                {
                        if ( Math.abs(end_i - start_i) == 1)
                        {
                                position[end_i][end_j] = position[start_i][start_j];
                                position[start_i][start_j] = Checkers.EMPTY;
                        }
                        else // capture
                        {
                                position[(start_i + end_i)/2][(start_j + end_j)/2] = Checkers.EMPTY;
                                position[end_i][end_j] = position[start_i][start_j];
                                position[start_i][start_j] = Checkers.EMPTY;
                        }

                        if (result == INCOMPLETEMOVE)
                        {
                                // if there are no further captures
                                if (!(canCapture(position,end_i,end_j)))
                                        result = LEGALMOVE;
                        }
                        
                        // check for new king
                        if ( position[end_i][end_j] == Checkers.WHITE && end_j == 7)
                                position[end_i][end_j] = Checkers.WKING;
                        else if ( position[end_i][end_j] == Checkers.BLACK && end_j == 0)
                                position[end_i][end_j] = Checkers.BKING;
        
                }

                return result;
        }

        // IsMoveLegal checks if the move entered is legal.
        // Returns ILLEGALMOVE or LEGALMOVE;
        // have to check with canCapture(int[][],int,int) to see
        //  if there is another capture possible after the first capture
        // Returns INCOMPLETEMOVE if a capture has taken place.
        // Note: it does not check if a 2nd capture is possible!
        static int IsMoveLegal(int[][] position,int start_i,int start_j,int end_i,int end_j,int turn)
        {
			if (! (inRange(start_i,start_j) && inRange(end_i,end_j) ) )
				return ILLEGALMOVE;
            if (position[end_i][end_j] != Checkers.EMPTY)
                return ILLEGALMOVE;

            int piece = position[start_i][start_j];
            if ( Math.abs(start_i - end_i) == 1 )
            {
                // first see if any captures are possible
                switch (piece)
                {
				case Checkers.WHITE:
                case Checkers.WKING:
					for (int i=0;i<8;i++)
						for (int j=0;j<8;j++)
                        {
							if ((position[i][j] == Checkers.WHITE ||
									position[i][j] == Checkers.WKING)
									&& canCapture(position,i,j))
								return ILLEGALMOVE;
                        }
                    break;
                case Checkers.BLACK:
                case Checkers.BKING:
                    for (int i=0;i<8;i++)
                        for (int j=0;j<8;j++)
                        {
                            if ((position[i][j] == Checkers.BLACK ||
								position[i][j] == Checkers.BKING)
								&& canCapture(position,i,j))
                                return ILLEGALMOVE;
					}
                    break;
				} // switch
                                                
                switch (piece)
                {
                case Checkers.WHITE:
					if (end_j - start_j == 1) return LEGALMOVE;
					break;
                case Checkers.BLACK:
					if (end_j - start_j == -1) return LEGALMOVE;
                    break;
				case Checkers.WKING:
                case Checkers.BKING:
                    if ( Math.abs(end_j - start_j) == 1 ) return LEGALMOVE;
                    break;
				} // switch (piece)
                
				return ILLEGALMOVE;

			} // if ( (Math.abs(start_i - end_i) == 1 )

            else if ( Math.abs(start_i - end_i) == 2 )
            {
				int cap_i = (start_i + end_i) / 2;
                int cap_j = (start_j + end_j) / 2;
                int cap_piece = position[cap_i][cap_j];

                if (turn == Checkers.WHITE)
                {
					if (!(cap_piece == Checkers.BLACK ||
						cap_piece == Checkers.BKING))
                        return ILLEGALMOVE;
				}
                else if (!(cap_piece == Checkers.WHITE ||
					cap_piece == Checkers.WKING))
                    return ILLEGALMOVE;

				switch (piece)
                {
                case Checkers.WHITE:
					if (end_j - start_j != 2)
						return ILLEGALMOVE;
					break;
				case Checkers.BLACK:
                    if (end_j - start_j != -2)
						return ILLEGALMOVE;
					break;
				case Checkers.WKING:
                case Checkers.BKING:
					if (Math.abs(end_j - start_j) != 2)
						return ILLEGALMOVE;
				}
                        
                return INCOMPLETEMOVE;
			}
            return ILLEGALMOVE;
        }

        static int IsWalkLegal(int[][] position,int start_i,int start_j,int end_i,int end_j,int turn)
        {
			if (! (inRange(start_i,start_j) && inRange(end_i,end_j) ) )
				return ILLEGALMOVE;
            if (position[end_i][end_j] != Checkers.EMPTY)
                return ILLEGALMOVE;

            int piece = position[start_i][start_j];
            if ( Math.abs(start_i - end_i) == 1 )
            {
                switch (piece)
                {
                case Checkers.WHITE:
					if (end_j - start_j == 1) return LEGALMOVE;
					break;
                case Checkers.BLACK:
					if (end_j - start_j == -1) return LEGALMOVE;
                    break;
				case Checkers.WKING:
                case Checkers.BKING:
                    if ( Math.abs(end_j - start_j) == 1 ) return LEGALMOVE;
                    break;
				} // switch (piece)
                
				return ILLEGALMOVE;

			} // if ( (Math.abs(start_i - end_i) == 1 )

            return ILLEGALMOVE;
        }


		static boolean canCapture(int[][] position, int toMove)
		{
			for (int i=0; i<8; i++)
				for (int j=0; j<8; j++)
			{
				if (color(position[i][j]) == toMove && canCapture(position,i,j))
					return true;
			}
			return false;
		}
  
        // examines a board position to see if the piece indicated at (x,y)
        // can make a(nother) capture
        static boolean canCapture(int[][] position, int i, int j)
        {
                int x,y;
                boolean result = false;

                switch (position[i][j])
                {
                case Checkers.WHITE:
                        if (i+2<8 && j+2<8)
                                if ( (position[i+1][j+1] == Checkers.BLACK ||
                                  position[i+1][j+1] == Checkers.BKING)
                                  &&
                                  (position[i+2][j+2] == Checkers.EMPTY))
                                  return true;
                        if (i-2>-1 && j+2<8)
                                if ( (position[i-1][j+1] == Checkers.BLACK ||
                                position[i-1][j+1] == Checkers.BKING)
                                &&
                                position[i-2][j+2] == Checkers.EMPTY)
                                return true;
                        break;
                case Checkers.BLACK:
                        if (i+2<8 && j-2>-1)
							if ( (position[i+1][j-1] == Checkers.WHITE ||
                                position[i+1][j-1] == Checkers.WKING)
                                &&
                                position[i+2][j-2] == Checkers.EMPTY)
                                return true;
                        if (i-2>-1 && j-2>-1)
                            if ( (position[i-1][j-1] == Checkers.WHITE ||
                                position[i-1][j-1] == Checkers.WKING)
                                &&
                                position[i-2][j-2] == Checkers.EMPTY)
                                return true;
                        break;
                case Checkers.WKING:
                        if (i+2<8)
                        {
                                if (j+2<8)
                                        if ( (position[i+1][j+1] == Checkers.BLACK ||
                                        position[i+1][j+1] == Checkers.BKING )
                                        &&
                                        position[i+2][j+2] == Checkers.EMPTY)
                                        return true;
                                if (j-2>-1)
                                        if ( (position[i+1][j-1] == Checkers.BLACK ||
                                        position[i+1][j-1] == Checkers.BKING )
                                        &&
                                        position[i+2][j-2] == Checkers.EMPTY)
                                        return true;
                        }
                        if (i-2>-1)
                        {
                                if (j+2<8)
                                        if ( (position[i-1][j+1] == Checkers.BLACK ||
                                        position[i-1][j+1] == Checkers.BKING )
                                        &&
                                        position[i-2][j+2] == Checkers.EMPTY)
                                        return true;
                                if (j-2>-1)
                                        if ( (position[i-1][j-1] == Checkers.BLACK ||
                                        position[i-1][j-1] == Checkers.BKING )
                                        &&
                                        position[i-2][j-2] == Checkers.EMPTY)
                                        return true;
                        }
                        break;
                case Checkers.BKING:
                        if (i+2<8)
                        {
                                if (j+2<8)
                                        if ( (position[i+1][j+1] == Checkers.WHITE ||
                                        position[i+1][j+1] == Checkers.WKING )
                                        &&
                                        position[i+2][j+2] == Checkers.EMPTY)
                                        return true;
                                if (j-2>-1)
                                        if ( (position[i+1][j-1] == Checkers.WHITE ||
                                        position[i+1][j-1] == Checkers.WKING )
                                        &&
                                        position[i+2][j-2] == Checkers.EMPTY)
                                        return true;
                        }
                        if (i-2>-1)
                        {
                                if (j+2<8)
                                        if ( (position[i-1][j+1] == Checkers.WHITE ||
                                        position[i-1][j+1] == Checkers.WKING )
                                        &&
                                        position[i-2][j+2] == Checkers.EMPTY)
                                        return true;
                                if (j-2>-1)
                                        if ( (position[i-1][j-1] == Checkers.WHITE  ||
                                        position[i-1][j-1] == Checkers.WKING )
                                        &&
                                        position[i-2][j-2] == Checkers.EMPTY)
                                        return true;
                        }
                        break;
                } // switch
                return false;
        } // canCapture()

        // canWalk() returns true if the piece on (i,j) can make a
        // legal non-capturing move
        static boolean canWalk(int[][] position, int i, int j)
        {
                switch ( position[i][j] )
                {
                case Checkers.WHITE:
                        if ( isEmpty(position,i+1,j+1) || isEmpty(position,i-1,j+1) )
                                return true;
                        break;
                case Checkers.BLACK:
                        if ( isEmpty(position,i+1,j-1) || isEmpty(position,i-1,j-1) )
                                return true;
                        break;
                case Checkers.WKING:
                case Checkers.BKING:
                        if ( isEmpty(position,i+1,j+1) || isEmpty(position,i+1,j-1) 
                                || isEmpty(position,i-1,j+1) || isEmpty(position,i-1,j-1) )
                                return true;
                } // switch
                return false;
        } // canWalk()

        private static boolean isEmpty(int[][] position, int i, int j)
        {
                if (i>-1 && i<8 && j>-1 && j<8)
                        if (position[i][j] == Checkers.EMPTY)
                        return true;
                return false;
        } // isEmpty()

                private static boolean isWhite(int[][] position, int i, int j)
		{
			if (i>-1 && i<8 && j>-1 && j<8)
				if (color(position[i][j]) == Checkers.WHITE)
				return true;
			return false;
		} // isWhite()

                private static boolean isBlack(int[][] position, int i, int j)
		{
			if (i>-1 && i<8 && j>-1 && j<8)
				if (color(position[i][j]) == Checkers.BLACK)
				return true;
			return false;
		} // isBlack()


		// returns the color of a piece
		static int color(int piece)
		{
			switch (piece)
			{
			case Checkers.WHITE:
			case Checkers.WKING:
				return Checkers.WHITE;
			case Checkers.BLACK:
			case Checkers.BKING:
				return Checkers.BLACK;
		 	}
			return Checkers.EMPTY;
		}

		// checkers that i and j are between 0 and 7 inclusive
                private static boolean inRange(int i, int j)
		{
			return (i>-1 && i<8 && j>-1 && j<8);
		}


		
//given a board, generates all the possible moves depending on whose turn
static Vector  generate_moves(int[][] board, int turn) {
 Vector moves_list = new Vector();
 int move;

 for (int i=7; i>=0; i--)
   for (int j=0; j<8; j++)
		if (turn==color(board[i][j])) {
	 if (canCapture(board,turn)) {
		 for (int k=-2; k<=2; k+=4)
			 for (int l=-2; l<=2; l+=4)
					 {
						move=IsMoveLegal(board,i,j,i+k,j+l,turn);
						if (move == INCOMPLETEMOVE)
						{
							int[] int_array = new int[4];
							int_array[0]=i; int_array[1]=j;
							int_array[2]=i+k; int_array[3]=j+l;
								int[][] temp_board = Engine.copy_board(board);
								move = Move.ApplyMove(temp_board,i,j,i+k,j+l);
								if (move == INCOMPLETEMOVE)/*(canCapture(temp_board,i+k,j+l))*/
								{
									forceCaptures(temp_board, turn, int_array,moves_list,10);
								}
								else
								{
									moves_list.addElement(int_array);
								}
							
						} // if (move)
					} // if (inRange)
	 }
	 else {
			for (int k=-1; k<=2; k+=2)
				for (int l=-1; l<=2; l+=2){
					if (inRange(i+k,j+l))
					{
						move=IsWalkLegal(board,i,j,i+k,j+l,turn);
						if (move == LEGALMOVE)
						{
							int[] int_array = new int[4];
							int_array[0]=i; int_array[1]=j;
							int_array[2]=i+k; int_array[3]=j+l;
								//a walk has taken place
								//add the new array to the Vector moves_list
								moves_list.addElement(int_array);
						} // if (move)
					} // if (inRange)
			} //end for k,l
	 }
 }//end if
 return moves_list;

}//generate_moves
	
//"apply move" in the Minimax.  simply moves the board give moves
static void move_board(int[][] board, int[] move)
{
	int startx = move[0];
	int starty = move[1];
	int endx = move[2];
	int endy = move[3];
	while (endx>0 || endy>0)
	{

		ApplyMove(board,startx,starty,endx%10,endy%10);
		startx = endx%10;
		starty = endy%10;
		endx /= 10;
		endy /= 10;
	} 
	
}

//for an initial capture represented by move, sees if there are more captures
//until there is none.  If there are multiple capture configurations,
//add all of them to moves_list 
private static void forceCaptures(int[][] board, int turn, int[] move, Vector moves_list,int inc){
 int newx = move[2], newy = move[3], opponent;

 while (newx>7 || newy>7){
	newx/=10;
	newy/=10;
 }//end while 
 for (int i=-2; i<=2; i+=4)
   for (int j=-2; j<=2; j+=4)
     if (inRange(newx+i,newy+j)) {
		int[][] tempPosition = Engine.copy_board(board);
		int moveResult = ApplyMove(tempPosition,newx,newy,newx+i,newy+j);
		if (moveResult == LEGALMOVE) {
			int[] new_move = new int[4];
			new_move[0] = move[0];
			new_move[1] = move[1];
			new_move[2] = move[2]+(newx+i)*inc;
  			new_move[3] = move[3]+(newy+j)*inc;
			moves_list.addElement(new_move);
		} // if (moveResult == LEGALMOVE)
		else if (moveResult == INCOMPLETEMOVE)
		{
			int[] new_move = new int[4];
			new_move[0] = move[0];
			new_move[1] = move[1];
			new_move[2] = move[2]+(newx+i)*inc;
  			new_move[3] = move[3]+(newy+j)*inc;
			
			forceCaptures(tempPosition, turn, new_move, moves_list, inc*10);
		}
	 } //end if for

}//forceCaptures



} // class Move



















