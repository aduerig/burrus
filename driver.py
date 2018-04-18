from optparse import OptionParser
from engine import Engine
from random_model import RandomModel
def main():
	parser = OptionParser()
	parser.add_option("-n", "--num_games", dest="num_games",
	                  help="Number of games", default = 10)
	parser.add_option("--p1", dest="player_1",
					  help="random for now", default="random")
	parser.add_option("--p2", dest="player_2",
					  help="random for now", default="random")
	parser.add_option("-p", "--print", dest="printing", action="store_true",
					  help="print every move", default=False)

	options, args = parser.parse_args()
	models = {"random": RandomModel}


	players =  {'B': models[options.player_1]('B'),
	 			'W': models[options.player_2]('W')}
	e = Engine()
	for _i in range(int(options.num_games)):
		over = False
		e.set_board_to_beginning()
		curr = 'B'
		while True:
			if len(e.get_moves(curr)) == 0:
				if len(e.get_moves(opposite(curr))) == 0:
					break
				curr = opposite(curr)
			to_move = players[curr].choose_move(e)
			e.play_move(to_move, curr)
			curr=opposite(curr)
			if(options.printing == True):
				print(e)

		print('Winning player:'+ e.winning_player())




#helpers
def opposite(piece):
	if piece == 'W':
		return 'B'
	if piece == 'B':
		return 'W'
	return None

if __name__ == "__main__":
    main()