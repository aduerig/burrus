

class Engine:
	def __init__(self,):
		self.board = [[None for i in range(8)] for i in range(8)]

	def set_board_to_beginning(self, board = None):
		if board == None:
			board = self.board
		board = [[None for i in range(8)] for i in range(8)]

		board[3][3] = 'W'
		board[3][4] = 'B'
		board[4][4] = 'W'
		board[4][3] = 'B'

	def get_moves(self, player, board = None):
		if board == None:
			board = self.board
		moves = []
		for x in range(len(board)):
			for y in range(len(board)):
				if self.is_loc_move(x,y,player, board):
					moves.append((x,y))
		return moves

	def is_loc_move(self, x, y,player, board=None):
		if board == None:
			board = self.board
		moves = []
		if board[x][y] is None:
			for path in [(0,1),(1,0),(1,1),(1,-1),(0,-1),(-1,0),(-1,-1),(-1,1)]:
				i=2
				if x+path[0] < 8 and x+path[0] >=0 and y+path[1] < 8 and y+path[1] >= 0 and opposite(board[x+path[0]][y+path[1]]) == player:
					while x+i*path[0] < 8 and x+i*path[0] >=0 and y+i*path[1] < 8 and y+i*path[1] >= 0 and opposite(board[x+i*path[0]][y+i*path[1]]) == player:
						i+=1
					if x+i*path[0] < 8 and x+i*path[0] >=0 and y+i*path[1] < 8 and y+i*path[1] >= 0 and board[x+i*path[0]][y+i*path[1]] == player:
						return True
		return False

	#doesn't check if move is legal
	def play_move(self, move,player, board=None):
		if board == None:
			board = self.board
		x,y = move
		board[x][y] = player
		for path in [(0,1),(1,0),(1,1),(1,-1),(0,-1),(-1,0),(-1,-1),(-1,1)]:
			i=2
			if x+path[0] < 8 and x+path[0] >=0 and y+path[1] < 8 and y+path[1] >= 0 and opposite(board[x+path[0]][y+path[1]]) == player:
				while x+i*path[0] < 8 and x+i*path[0] >=0 and y+i*path[1] < 8 and y+i*path[1] >= 0 and opposite(board[x+i*path[0]][y+i*path[1]]) == player:
					i+=1
				if x+i*path[0] < 8 and x+i*path[0] >=0 and y+i*path[1] < 8 and y+i*path[1] >= 0 and board[x+i*path[0]][y+i*path[1]] == player:
					for i_iter in range(1, i):
						board[x+i_iter*path[0]][y+i_iter*path[1]] = player
	
	def __str__(self):
		str = ''
		for row in self.board:
			for piece in row:
				if piece is None:
					str += '*'
				else:
					str += piece
			str += '\n'
		return str

	def winning_player(self, board = None):
		if board == None:
			board = self.board
		w = 0
		b = 0
		for row in board:
			for piece in row:
				if piece == 'W':
					w += 1
				if piece == 'B':
					b += 1
		if w>b:
			return 'W'
		if b>w:
			return 'B'
		return 'TIE'


#helpers
def opposite(piece):
	if piece == 'W':
		return 'B'
	if piece == 'B':
		return 'W'
	return None