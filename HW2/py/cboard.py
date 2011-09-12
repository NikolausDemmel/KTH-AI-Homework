from constants import *
from cmove import *

class CBoard:
  #constructor
  
  #if pInit is true, it will leave the board in a state that represents
  #the initial board position.
  #otherwise, it is undefined
  def __init__(self,pInit=True,player=None):
    if isinstance(pInit, list):
      self._mCell = list(range(NSQUARES))
      for i in range(NSQUARES):
        self._mCell[i] = pInit[i]
      self._player = player or CELL_OWN
      self._numberOfPieces = self._count_pieces()
    elif isinstance(pInit, CBoard):
      self._mCell = pInit._mCell[:]
      self._player = pInit._player
      self._numberOfPieces = pInit._numberOfPieces
    elif isinstance(pInit, bool):
      self._mCell = list(range(NSQUARES))
      self._player = player or CELL_OWN
      if pInit:
        for i in range(NSQUARES):
          if i<NPLAYERPIECES:
            self._mCell[i] = CELL_OWN
          elif i<NSQUARES-NPLAYERPIECES:
            self._mCell[i] = CELL_EMPTY
          else:
            self._mCell[i] = CELL_OTHER
        self._numberOfPieces = 24
      else:
        self._numberOfPieces = 0
    else:
      raise TypeError("CBoard expects either a bool, another CBoard or a list of cells")
  
  def player(self):
    "Return the whos turn it is, either CELL_OWN or CELL_OTHER"
    return self._player

  def state(self):
    "return tuple that fully captures the boards state. Can be used as keys in dicts."
    return tuple([self._player] + self._mCell)

  def numberOfPieces(self):
    return self._numberOfPieces

  def _count_pieces(self):
    "Count pieces on the board"
    count = 0
    for c in self._mCell:
      if c & CELL_OWN or c & CELL_OTHER:
        count += 1
    return count

  def __eq__(self, other):
    # assume that numberOfPieces is consistent
    if self._player != other._player:
      return False
    for i in range(NSQUARES):
      if self._mCell[i] != other._mCell[i]:
        return False
    return True

  #returns the contents of the cell pPos (where pPos is a number from 0 to 31)
  
  #Cells are numbered as follows:
  #
  #    31    30    29    28
  # 27    26    25    24   
  #    23    22    21    20
  # 19    18    17    16   
  #    15    14    13    12
  # 11    10     9     8   
  #    7      6     5     4
  #  3     2     1     0
  #
  # this function returns a byte representing the contents of the cell.
  #
  # For example, to check if cell 3 contains an own piece, you would check if
  #
  #   (lBoard.atcell(3)&CELL_OWN)
  #
  # to check if it is a piece of the other player,
  #
  #   (lBoard.atcell(3)&CELL_OTHER)
  #
  # and to check if it is aking, you would check if
  #
  #   (lBoard.atcell(3)&CELL_KING)
  #
  def atcell(self,pPos):
    assert pPos<NSQUARES
    return self._mCell[pPos]

  #returns the contents of the cell at the pRth row and pCth column
  #both rows and columns are in the range 0-7.
  #(0,0) is the lower right corner

  #the contents is returned in the same format as atcell
  def at(self,pR,pC):
    if pR<0 or pR>=NROWS or pC<0 or pC>=NCOLS:
      return CELL_INVALID
    if pR&1 == pC&1:
      return CELL_INVALID
    return self._mCell[pR*(NROWS//2)+(pC>>1)]

  def has_adjacent_enemy(self,r,c):
    if (self.at(r+1,c+1) & CELL_OTHER or 
        self.at(r+1,c-1) & CELL_OTHER or
        self.at(r-1,c+1) & CELL_OTHER or
        self.at(r-1,c-1) & CELL_OTHER):
      return True
    return False
    
  def __setcell(self,pR,pC,pValue):
    self._mCell[pR*(NROWS//2)+(pC>>1)]=pValue

  #operator version of the above function. Allows lBoard(pR,pC)
  def __call__(self,pR,pC):
    return self.at(pR,pC)

  #obtains the row of a cell index
  def cell2row(self,pCell):
    return pCell>>2

  #obtains the col of a cell index
  def cell2col(self,pCell):
    lC=(pCell&3)<<1
    if not pCell&4:
        lC+=1
    return lC

  #obtains the cell index from a row and a col
  def row_col2cell(self,pR,pC):
    return (pR*4)+(pC>>1)

  def __try_jump_dir(self,pMoves,pOther,pR,pC,pKing,pBuffer,pR_inc,pC_inc,pDepth=0):
    if (self.at(pR+pR_inc,pC+pC_inc) & pOther) and (self.at(pR+2*pR_inc,pC+2*pC_inc)==CELL_EMPTY):
      lOldValue = self.at(pR+pR_inc,pC+pC_inc)
      self.__setcell(pR+pR_inc,pC+pC_inc,CELL_EMPTY)
      self.__try_jump(pMoves,pOther,pR+2*pR_inc,pC+2*pC_inc,pKing,pBuffer,pDepth+1)
      self.__setcell(pR+pR_inc,pC+pC_inc,lOldValue)
      return True;
    return False

  def __try_jump(self,pMoves,pOther,pR,pC,pKing,pBuffer,pDepth=0):
    pBuffer[pDepth]=self.row_col2cell(pR,pC)
    lFound = False
    # try capturing fw
    if pOther==CELL_OTHER or pKing:
      # try capturing right
      lFound=self.__try_jump_dir(pMoves,pOther,pR,pC,pKing,pBuffer,1,-1,pDepth) or lFound
      #try capturing left
      lFound=self.__try_jump_dir(pMoves,pOther,pR,pC,pKing,pBuffer,1,1,pDepth) or lFound
    # try capturing bw
    if pOther==CELL_OWN or pKing:
      # try capturing right
      lFound=self.__try_jump_dir(pMoves,pOther,pR,pC,pKing,pBuffer,-1,-1,pDepth) or lFound
      #try capturing left
      lFound=self.__try_jump_dir(pMoves,pOther,pR,pC,pKing,pBuffer,-1,1,pDepth) or lFound

    if not lFound and pDepth>0: # XXX check precedence of operators
      pMoves.append(CMove(pDepth,pBuffer))

    return lFound

  def __try_move_dir(self,pMoves,pCell,pR_dest,pC_dest):
    if self.at(pR_dest,pC_dest)==CELL_EMPTY:
      pMoves.append(CMove(MOVE_NORMAL,[pCell,self.row_col2cell(pR_dest,pC_dest)]))


  def __try_move(self,pMoves,pCell,pOther,pKing):
    lR = self.cell2row(pCell)
    lC = self.cell2col(pCell)
    # try moving fw
    if pOther==CELL_OTHER or pKing:
      self.__try_move_dir(pMoves,pCell,lR+1,lC-1)# try moving right
      self.__try_move_dir(pMoves,pCell,lR+1,lC+1)# try moving left
    # try moving bw
    if pOther==CELL_OWN or pKing:
      self.__try_move_dir(pMoves,pCell,lR-1,lC-1)# try moving right
      self.__try_move_dir(pMoves,pCell,lR-1,lC+1)# try moving left

  #returns a list of CMove objects, representing the possible moves
  #from this board position
  def find_possible_moves(self):
    lMoves=[]
    # assert pWho==CELL_OWN or pWho==CELL_OTHER
    lOther = self._player^(CELL_OWN|CELL_OTHER)
    lFound = False
    lPieces = []
    lMoveBuffer = [-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1]

    for i in range(NSQUARES):
      if self.atcell(i) & self._player:
        lIsKing = self.atcell(i) & CELL_KING
        if self.__try_jump(lMoves,lOther,self.cell2row(i),self.cell2col(i),lIsKing,lMoveBuffer):
          lFound = True
        elif not lFound:
          lPieces.append(i)

    if not lFound:
      if self._player == CELL_OWN: # move ordering
        lPieces.reverse()
      for p in lPieces:
        lIsKing = self.atcell(p) & CELL_KING
        self.__try_move(lMoves,p,lOther,lIsKing)
    else:
      order_jumps(lMoves)

    return lMoves

  def _invert_player(self):
    if self._player == CELL_OWN:
      self._player = CELL_OTHER
    else:
      self._player = CELL_OWN

  #transforms this board positions bt making the move pMove
  def do_move(self,pMove):
    if pMove.is_jump():
      self._invert_player()
      self._numberOfPieces -= pMove.number_of_jumps()

      lSR = self.cell2row(pMove[0])
      lSC = self.cell2col(pMove[0])

      for i in range(1,len(pMove)):
        lDR = self.cell2row(pMove[i])
        lDC = self.cell2col(pMove[i])

        self._mCell[pMove[i]] = self._mCell[pMove[i-1]]
        self._mCell[pMove[i-1]] = CELL_EMPTY

        if (lDR == NROWS-1 and self._mCell[pMove[i]] & CELL_OWN) or (lDR == 0 and self._mCell[pMove[i]] & CELL_OTHER):
          self._mCell[pMove[i]] = self._mCell[pMove[i]] | CELL_KING

        lDR_capt = lDR-1 if lDR>lSR else lDR+1
        lDC_capt = lDC-1 if lDC>lSC else lDC+1
        self._mCell[self.row_col2cell(lDR_capt,lDC_capt)] = CELL_EMPTY

        lSR = lDR
        lSC = lDC

    elif pMove.is_normal():
      self._invert_player()

      lDR = self.cell2row(pMove[1])
      self._mCell[pMove[1]] = self._mCell[pMove[0]]
      self._mCell[pMove[0]] = CELL_EMPTY

      if (lDR == NROWS-1 and self._mCell[pMove[1]] & CELL_OWN) or (lDR == 0 and self._mCell[pMove[1]] & CELL_OTHER):
        self._mCell[pMove[1]] = self._mCell[pMove[1]] | CELL_KING

  def copy_and_move(self, pMove):
    "Make a copy of self, apply the move and return the resulting board"
    board = CBoard(self)
    board.do_move(pMove)
    return board

  #pretty-prints the board to standard output
  def print_out(self):
    for r in reversed(range(NROWS)):
      line = ""
      for c in reversed(range(NCOLS)):
        if c&1 == r&1:
          line += "\033[47m  \033[0m"
        elif(self.at(r,c) & CELL_OWN):
          if(self.at(r,c) & CELL_KING):
            line += "\033[37;40m##\033[0m"
          else:
            line += "\033[37;40m()\033[0m"
        elif(self.at(r,c) & CELL_OTHER):
          if(self.at(r,c) & CELL_KING):
            line += "\033[31;40m##\033[0m"
          else:
            line += "\033[31;40m()\033[0m"
        else:
          line += "\033[37;40m  \033[0m"
      # print a second board to right, which includes the cell numbers.
      line += "    "
      for c in reversed(range(NCOLS)):
        if c&1 == r&1:
          line += "\033[47m  \033[0m"
        else:
          line += "\033[37;40m{0:2}\033[0m".format(r*(NROWS//2)+(c>>1))
      print(line)
    print("")

  #pretty-prints the board to standard output (no color version)
  def print_nocolor(self):
    print("----------------")
    for r in reversed(range(NROWS)):
      line = ""
      for c in reversed(range(NCOLS)):
        if c&1 == r&1:
          line += "  "
        elif(self.at(r,c) & CELL_OWN):
          if(self.at(r,c) & CELL_KING):
            line += "WW"
          else:
            line += "ww"
        elif(self.at(r,c) & CELL_OTHER):
          if(self.at(r,c) & CELL_KING):
            line += "RR"
          else:
            line += "rr"
        else:
          line += "  "
      print(line)
    print("----------------")

  def game_over(self, moves = None):
    "Returns True if the game is over. turn should be CELL_OWN or CELL_OTHER."
    if None == moves:
      moves = self.find_possible_moves()
    return not moves

  def evaluate(self, moves = None):
    # ACKNOLEDGEMENT: some ideas for evaluation function taken from:
    # http://www.mec-acm.org/assets/archive/assets/Checkers/Checkers_page.htm
    
    # ignoring draw for now. Should do something clever in endgame anyway...

    is_endgame = self._numberOfPieces <= ENDGAME

    if None == moves:
      moves = self.find_possible_moves()
    if not moves:
      if self._player == CELL_OWN:
        return 0.0 # we lose. 
      else:
        return 1.0 # we win.
    own = 0
    other = 0
    for r in range(NROWS):
      for c in range(NCOLS):
        cell = self.at(r,c)
        if cell & CELL_OWN: 
          if cell & CELL_KING:
            own += KING_SCORE
            if r == 0 or r == 7:
              own += KING_SIDE
            if c == 0 or c == 7:
              own += KING_SIDE
            if is_endgame:
              if self.has_adjacent_enemy(c,r):
                own += ENDGAME_AGGRESSIVE_KING
          else:
            own += PAWN_SCORE
            own += PAWN_POS * r * r
        elif cell & CELL_OTHER:
          if cell & CELL_KING:
            other += KING_SCORE
            if r == 0 or r == 7:
              other += KING_SIDE
            if c == 0 or c == 7:
              other += KING_SIDE
          else:
            other += PAWN_SCORE
            other += PAWN_POS * (7-r) * (7-r)
    return own / (own+other)

PAWN_SCORE=1
KING_SCORE=2
KING_SIDE=-0.1
PAWN_POS=0.01
ENDGAME_AGGRESSIVE_KING=0.2
ENDGAME=8

# add random weight??

def order_jumps(list_of_jump_moves):
  list_of_jump_moves.sort(key=lambda m: m.number_of_jumps())
