import random
import time
from constants import *

class CPlayer:
  # this function is called when waiting for the other player to move
  
  # When called, pBoard will contain the current state of the board.
  # If you return false from it, it won't be called until the next move.
  # Otherwise, the client will check if the other player moved already,
  # and call this function again if it didn't
  def idle(self,pBoard):
    return False

  # you can perform initialization tasks from this function
  
  # This function is called before the game starts
  # the pFirst parameter will be True if you are the player who is going
  # to move first.
  # 
  # pDue will contain the time (in seconds since the epoch, floating point
  # value) before which you must have completed initialization
  # For example, to check if we have less than 100 ms to return, we can check if
  # pDue-time.time()<0.1.
  def initialize(self,pFirst,pDue):
    random.seed()

  # this is the function from which you play!

  # pBoard will contain the current board position
  # pDue will, like in initialize, represent the time before which you must
  # have made your move
  #
  # you must return a CMove containing your move.
  def play(self,pBoard,pDue):
    pBoard.print_out() 
    print("value: %f, turn: %s" % (pBoard.evaluate(CELL_OWN),"we" if pBoard.player() == CELL_OWN else "they"))

    self.max_depth = 5
    
    move = self.a_b_search(pBoard)
    return move
    #return random.choice(pBoard.find_possible_moves(CELL_OWN))

  def cutoff_test(self, board, depth, moves = None):
    if board.game_over(moves):
      return True
    if depth >= self.max_depth:
      return True
    return False
  
  def a_b_search(self,board):
    # assume there is at least 1 possible move and it is our turn
    print("starting a b serach. Max depth: %d" % self.max_depth)
    self.number_of_boards = 0
    a = NEGINF
    moves = board.find_possible_moves()
    v,m = NEGINF, None
    for mcurr in moves:
      vcurr = self.min_value(board.copy_and_move(mcurr), a, INF, 0)
      if vcurr > v:
        v = vcurr
        m = mcurr
      a = max(a,v)
    print("finished a b search. Number of boards: %d. Best move: '%s', value: %f" % (self.number_of_boards, m.to_string(), v))
    return m
  
  def max_value(self, board, a, b, depth):
    moves = board.find_possible_moves()
    if self.cutoff_test(board, depth, moves):
      return board.evaluate(moves)
    else:
      v = NEGINF
      for m in moves:
        v = max(v, self.min_value(board.copy_and_move(m), a, b, depth+1))
        self.number_of_boards += 1
        if v >= b:
          return v
        a = max(a,v)
      return v

  def min_value(self, board, a, b, depth):
    moves = board.find_possible_moves()
    if self.cutoff_test(board, depth, moves):
      return board.evaluate(moves)
    else:
      v = INF
      for m in moves:
        v = min(v, self.max_value(board.copy_and_move(m), a, b, depth+1))
        self.number_of_boards += 1
        if v <= a:
          return v
        b = min(b,v)
      return v

      
