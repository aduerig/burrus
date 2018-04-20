from copy import deepcopy
ITERATIONS = 20

class NetModel:
    def __init__(self, player):
        self.player = player
    def choose_move(self, e, data, cnn):
        # cnn.evaluate_board(e, e.board) #for fun
        moves = e.get_moves(self.player)
        if len(moves)==1:
            return moves[0]
        root = Node()
        root.player = self.player
        root.board = e.board

        for _c in range(ITERATIONS):
            self.traverse(root, e)


        vals = []
        for child in root.children:
            vals.append(child.value)

        # Store this move's data
        data['boards'].append(deepcopy(e.board))
        formatted_vals = [['' for i in range(8)] for j in range(8)]
        for child in root.children:
            x, y = child.my_move[0], child.my_move[1]
            formatted_vals[x][y] = child.value
        data['search_vals'].append(formatted_vals)

        if self.player == 'B':
            move = root.children[vals.index(max(vals))].my_move
        else:
            move = root.children[vals.index(min(vals))].my_move
        return move


    def traverse(self, curr, e):
        if len(e.get_moves(curr.player, curr.board))==0:
            result = e.winning_player(curr.board)
            if result == 'W':
                return -1
            if result == 'B':
                return 1
            else:
                return 0

        curr.visits+=1
        if not curr.children:
            curr.children = []
            for move in e.get_moves(curr.player, curr.board):
                child = Node()
                child.my_move = move
                child.player = opposite(curr.player)
                child.board = [x[:] for x in curr.board]
                e.play_move(move, child.player, child.board)
                curr.children.append(child)


        #tbd UCT
        visitlist = []
        for child in curr.children:
            visitlist.append(child.visits)
        togo = curr.children[visitlist.index(min(visitlist))]
        res = self.traverse(togo, e)
        curr.value = (curr.value*(curr.visits-1) + res) / float(curr.visits)
        return res




# board, parent, children, visits, value, policy, 
class Node:
    def __init__(self):
        self.visits=0
        self.value = 0 # not used
        self.children = None
#helpers
def opposite(piece):
    if piece == 'W':
        return 'B'
    if piece == 'B':
        return 'W'
    return None