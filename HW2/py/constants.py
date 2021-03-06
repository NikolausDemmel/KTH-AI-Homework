CELL_EMPTY=0			#< the cell is empty
CELL_OWN=(1<<0)		#< the cell belongs to us (the one playing)
CELL_OTHER=(1<<1)		#< the cell belongs to the other player
CELL_KING=(1<<2)		#< the cell is a king
CELL_INVALID=(1<<3)		#< the cell is invalid
NPLAYERPIECES = 12
NSQUARES = 32
NROWS = 8
NCOLS = 8
BUFSIZE = 4096

# infinities
# used only for comparison with values in range [0.0, 1.0]
INF = 2.0
NEGINF = -1.0
