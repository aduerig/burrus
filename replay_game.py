import numpy as np
import os
import utils
import time

from tkinter import *
from tkinter import messagebox


def draw_lines(canvas, width, height):
    holder = []

    # vertical lines
    curr_x = 0
    inc_x = width / 8
    for i in range(9):
        line = canvas.create_line(curr_x, 0, curr_x, height, fill = 'black')
        holder.append(line)
        curr_x += inc_x

    # horizontal lines
    curr_y = 0
    inc_y = height / 8
    for i in range(9):
        line = canvas.create_line(0, curr_y, width, curr_y, fill = 'black')
        holder.append(line)
        curr_y += inc_y
    return holder


def draw_circle(canvas, width, height, pos):
    if(pos >= 64):
        color = "white"
        pos = pos - 64
    else:
        color = "black"

    x = pos % 8
    y = int(pos / 8)
    
    offset = 5
    square_x_size = (width / 8)
    square_y_size = (height / 8)
    
    circ = canvas.create_oval(x * square_x_size + offset, y * square_y_size + offset, 
                            (x+1) * square_x_size - offset, 
                            (y+1) * square_y_size - offset, 
                            fill = color)
    return circ


def click_callback(event, canvas, game_data, counter, objs, width, height):
    for obj in objs:
        canvas.delete(obj)
    objs.clear()

    temp = 0
    # utils.print_board(game_data[0][counter[0]])
    for circle in game_data[0][counter[0]]:
        # print(game_data[0][counter[0]])
        if(circle == 1):
            objs.append(draw_circle(canvas, width, height, temp))
        temp += 1

    counter[0] += 1
    if(counter[0] == len(game_data[0])):
        print("reached end of games, exiting")
        exit(0)


def close(event):
    exit() # if you want to exit the entire thing


def display_game(games, games_dir):
    top = Tk()
    top.title('Othello game displayer')

    bg_color_board = "salmon4"
    bg_color = "gray25"
    top.configure(bg=bg_color)

    w = 500
    h = 500
    canvas = Canvas(top, bg = bg_color_board, height = h, width = w)
    # canvas.pack_propagate(False)

    game_num = 0
    text_to_disp = "{0}\ngame number: {1}".format(games_dir, game_num)
    text = Label(top, text=text_to_disp, height=2, bg = bg_color, fg="white", font=("Helvetica", 11))
    text.pack()


    # coord = [10, 50, 240, 210]
    # arc = canvas.create_arc(coord, start = 0, extent = 150, fill = "red")
    # line = canvas.create_line(10, 10, 200, 200, fill = 'white')

    lines = draw_lines(canvas, w, h)

    counter = [0]
    objs = []
    click_callback(None, canvas, games, counter, objs, w, h)
    canvas.focus_set()

    canvas.bind("<Button-1>", lambda x: click_callback(x, canvas, games, counter, objs, w, h))
    canvas.bind("<space>", lambda x: click_callback(x, canvas, games, counter, objs, w, h))
    canvas.bind("<Return>", lambda x: click_callback(x, canvas, games, counter, objs, w, h))
    canvas.bind("<Escape>", close)

    canvas.pack()
    top.mainloop()



def load_games(games_dir):
    curr_path = os.path.join(games_dir, 'all_games.game')
    boards, evals, results = utils.read_in_games(curr_path)
    return [boards, evals, results]


def main():    
    games_dir = utils.get_games_dir('recent')
    print('Using model at: ' + games_dir)
    games = load_games(games_dir)

    # for i in reversed(range(len(games[0]))):
    #     utils.print_board(games[0][i])
    #     input("Press enter to see next board state")

    inv_games = [list(reversed(games[0])), list(reversed(games[1])), list(reversed(games[2]))]
    display_game(inv_games, games_dir)


if __name__ == "__main__":
    main()