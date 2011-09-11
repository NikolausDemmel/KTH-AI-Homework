#!/usr/bin/python
from cplayer import CPlayer
from csocket import CSocket
from cboard import CBoard
from cmove import CMove
from constants import *
import time

import sys

if len(sys.argv)<3:
  print('usage: '+sys.argv[0]+" host port [gamekey]\n");
  
lPlayer=CPlayer()
lSocket=CSocket(sys.argv[1],int(sys.argv[2]))

if len(sys.argv)==3:
  lStandalone=True
  lSocket.write_line("MODE STANDALONE")
else:
  lStandalone=False
  lSocket.write_line("MODE GAME "+sys.argv[3])

lLine=lSocket.read_line(True)

lTokens=lLine.split()

if lStandalone:
    lTime=time.time()+19.0
else:
    lTime=int(lTokens[0])/1000000.0
lFirst=bool(int(lTokens[1]))

lPlayer.initialize(lFirst,lTime)

lSocket.write_line("INIT")

lBoard=CBoard()

# lBoard = CBoard([0,0,0,1,
#                  0,0,0,1,
#                  0,0,1,1,
#                  1,1,0,0,
#                  2,0,1,1,
#                  0,0,0,0,
#                  2,2,2,2,
#                  0,0,0,0])

# lBoard.print_out()

# for m in lBoard.find_possible_moves(CELL_OWN):
#   print(m.to_string())

# sys.exit(0)


while True:
  lBlock=False
  while True:
    lLine=lSocket.read_line(lBlock);
    if lLine:
      break
    if not lPlayer.idle(lBoard):
      lBlock=True

  lTokens=lLine.split(None,1)
  
  if lStandalone:
      lTime=time.time()+9.0
  else:
      lTime=int(lTokens[0])/1000000.0
  lMove=CMove.from_string(lTokens[1])
  
  if lMove.is_EOG():
    if len(lMove)>0:
      if lMove[0]==1:
        print('YOU WIN')
      elif lMove[0]==2:
        print('YOU LOSE')
      elif lMove[0]==3:
        print('DRAW')
      else:
        print('INVALID GAME')
    sys.exit(0)
  lBoard.do_move(lMove)
  lMove=lPlayer.play(lBoard,lTime)
  lSocket.write_line(lMove.to_string())
  if lMove.is_EOG():
    sys.exit(0)
  lBoard.do_move(lMove)