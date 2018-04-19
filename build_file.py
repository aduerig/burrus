
boards = []
evals = []
results = []

def bitfield(n):
    return [n >> i & 1 for i in range(63,-1,-1)]
for whocares in range(1000):
    with open("0_1.game", "r") as f:
        move_count = int(f.readline())

        for i in range(move_count):
            board1 = bitfield(int(f.readline()))
            board2 = bitfield(int(f.readline()))
            boards.append(board1+board2)
            arr = []
            for _j in range(64):
                arr.append(float(f.readline()))
            evals.append(arr)
            #skip saved values for now
            f.readline()
            results.append(int(f.readline()))

# for row in boards:
#     for i in range(8):
#         print(row[8*i:8*(i+1)])
#         print
#     print('vs')
#     for i in range(8,16):
#         print(row[8*i:8*(i+1)])
#         print
#     print('++++')
#     print
