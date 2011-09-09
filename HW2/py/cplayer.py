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
    #use the commented version if your system supports ANSI colors (linux does)
    #pBoard.print_out() 
    pBoard.print_nocolor()
    
    #Here you should write your clever algorithms to get the best next move.
    #This skeleton returns instead a random movement from the legal ones.
             
    return random.choice(pBoard.find_possible_moves(CELL_OWN))
