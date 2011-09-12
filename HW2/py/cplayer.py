import random
import time
from constants import *
from itertools import count
import signal

def signal_handler(signum, frame):
  raise Exception("Timeout")

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
    print("value: %f, turn: %s" % (pBoard.evaluate(),"we" if pBoard.player() == CELL_OWN else "they"))

    self.max_depth = 1
    move = self.a_b_search(pBoard)

    signal.signal(signal.SIGALRM, signal_handler)
    signal.setitimer(signal.ITIMER_REAL, pDue-time.time())

    ultimate_max_depth = 500
    continue_search = True

    try:
      for self.max_depth in range(2,ultimate_max_depth):
        move, continue_search = self.a_b_search(pBoard)
        if not continue_search:
          break
      signal.setitimer(signal.ITIMER_REAL,0)
    except Exception as e:
      print("Interrupted: " + str(e))

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
    if len(moves) == 1:
      return moves[0], False
    v,m = NEGINF, None
    for mcurr in moves:
      vcurr = self.min_value(board.copy_and_move(mcurr), a, INF, 0)
      print("move %s has value %f" % (mcurr.to_string(),vcurr))
      if vcurr > v:
        v = vcurr
        m = [mcurr]
      elif v == vcurr:
        m.append(mcurr)
      a = max(a,v)
    print("finished a b search. Number of boards: %d. Best moves: '%s', value: %f" % (self.number_of_boards, list(map(lambda x: x.to_string(), m)), v))
    move = random.choice(m)
    print("choosing move " + move.to_string())
    return move, True
  
  def max_value(self, board, a, b, depth):
    moves = board.find_possible_moves()

    if len(moves) == 1:
      board.do_move(moves[0])
      return self.min_value(board, a, b, depth)

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

    if len(moves) == 1:
      board.do_move(moves[0])
      return self.max_value(board, a, b, depth)

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

      
