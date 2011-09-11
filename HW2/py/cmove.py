import re
from constants import *


MOVE_JUMP = 1
MOVE_NORMAL = 0
MOVE_EOG = -1
MOVE_BOG = -2
MOVE_NULL = -3

#encapsulates a move

#in general, you should regard this as an opaque structure that you obtain
#from FindPossibleMoves, and provide to DoMove.

#The functions IsNormal() and Is Jump() can however be useful.

#The rest of the interface you can probably ignore it.
class CMove:
  #initializes the move from a type and the associated data
  
  #you shouldn't need to use this function unless you are doing
  #low-level modifications
  def __init__(self,pType,pData):
    self.__mType = pType
    if pType==MOVE_NORMAL:
      self.__mData = pData[:2]
    elif pType>=MOVE_JUMP:
      self.__mData = pData[:pType+1]
    elif pType==MOVE_EOG:
      self.__mData = pData[:1]
    else:
      self.__mData = []

  #converts a string received from the server into a move

  #you shouldn't need to use this function unless you are doing
  #low-level modifications
  @staticmethod
  def from_string(pStr):
    lNum = [int(a) for a in pStr.split()]

    lType = lNum[0]
    lLen = 0 # could get it from the string
    if lType == MOVE_NORMAL:
      lLen = 2
    elif lType > 0:
      lLen = lType + 1
    elif lType == MOVE_EOG:
      lLen = 1

    if lLen > 12 or lType < -3:
      return CMove(MOVE_NULL,[])

    lData = []

    for i in range(lLen):
      lCell = lNum[i+1]
      if lCell<0 or lCell>NSQUARES:
        lType=MOVE_NULL
        break
      lData.append(lCell)
    return CMove(lType,lData)

  #returns true if this is an invalid move object
  def is_null(self):
    return self.__mType == MOVE_NULL

  #returns true if this move is the beginning of game mark 
  def is_BOG(self):
    return self.__mType == MOVE_BOG

  #returns true if this move is the end game mark 
  def is_EOG(self):
    return self.__mType == MOVE_EOG

  #returns true if this move is a jump
  def is_jump(self):
    return self.__mType > 0

  def number_of_jumps(self):
    return self.__mType

  #returns true if this move is a normal move
  def is_normal(self):
    return self.__mType == MOVE_NORMAL

  def __len__(self):
    return len(self.__mData)

  def __getitem__(self,pN):
    return self.__mData[pN]

  #returns a string that can be passed to the server
  def to_string(self):
    return ' '.join(map(lambda x:str(x),[self.__mType]+self.__mData))

  def __eq__(self,pRH):
    if self.__mType != pRH.__mType:
      return False
    if len(self.__mData)!=len(pRH.__mData):
      return False

    for i in range(len(self.__mData)):
      if self.__mData[i] != pRH.__mData[i]:
        return False

    return True
