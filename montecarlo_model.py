

class MonteCarloModel:
	def init(self, player):
		self.player = player
	def choose_move(self, e):
		moves = e.get_moves(self.player)
		if len(moves)==1:
			return moves[0]
		root = Node()
		root.player = self.player
		root.board = e.board()
		root.children = []
		for move in moves:
			child = Node()
			child.player = opposite(root.player)
			child.board = [x[:] for x in root.board]
			e.play_move(move, child.player, child.board)
			root.children.append(Node())


# board, parent, children, visits, value, policy, 
class Node:
	pass