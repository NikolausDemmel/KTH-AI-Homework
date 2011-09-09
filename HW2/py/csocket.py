import socket,copy
from constants import *

class CSocket:
  def __init__(self,pHost,pPort):
    self.__mS = None
    self.__mBuffer = bytes()
    self.__mCheckLen = 0

    for res in socket.getaddrinfo(pHost,pPort,socket.AF_UNSPEC,socket.SOCK_STREAM,0,socket.AI_NUMERICSERV):
      (af, socktype, proto, canonname, sa) = res
      try:
        self.__mS = socket.socket(af, socktype, proto)
      except socket.error as msg:
        self.__mS = None
        continue
      try:
        self.__mS.connect(sa)
      except socket.error as msg:
        self.__mS.close()
        self.__mS = None
        continue
      break
    if self.__mS is None:
      raise RunTimeError('could not open socket')

  def __del__(self):
    if self.__mS is None:
      return
    self.__mS.close()
    self.__mFD = None
    self.__mCheckLen = 0

  def write_line(self,pStr):
    if self.__mS is None:
      raise RunTimeError("Trying to write in an uninitialized socket")
    lStr = pStr.encode()
    if not lStr or lStr[-1]!=b'\n':
      lStr+=b'\n'
    lWritten = 0
    while lWritten < len(lStr):
      self.__mS.setblocking(0)
      sent = self.__mS.send(lStr[lWritten:])
      if sent==0:
        raise RunTimeError("socket connection broken")
      lWritten += sent

  def read_line(self,pBlock):
    if self.__mS is None:
      raise RunTimeError("Trying to write in an uninitialized socket")
    line=self.__check_line()
    if line!=None:
      return line
    while True:
      if len(self.__mBuffer) == BUFSIZE:
        raise RunTimeError("Buffer full in CSocket::buffer()")
      try:
        self.__mS.setblocking(1 if pBlock else 0)
        received = self.__mS.recv(BUFSIZE-len(self.__mBuffer))
        if not received:
          raise RunTimeError("socket connection broken")
        self.__mBuffer += received
      except socket.error as aaa:
        if pBlock:
          throw(aaa)

      line=self.__check_line()
      if line!=None:
        return line

      if pBlock:
        continue

      return None

  def __check_line(self):
    while self.__mCheckLen<len(self.__mBuffer):    
      if self.__mBuffer[self.__mCheckLen]==b'\n'[0]:
        line=self.__mBuffer[:self.__mCheckLen].rstrip(b'\r')
        self.__mBuffer=self.__mBuffer[self.__mCheckLen+1:]
        self.__mCheckLen=0;
        return line.decode()
      self.__mCheckLen+=1
    return None

