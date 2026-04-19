import math
import time

class Board:

    def __init__(self, board = None, next_player_c = None):
        if board == None:
            self.board = [[['.', '.', '.'] for _ in range(3)] for _ in range(3)]
        if next_player_c == None:
            self.next_player_c = 'X'

    # Array should always be 3x3x3
    def __str__(self):  
        output = '=' * 10 + '\n' * 2
        layer_names = ["Bottom Layer", "Middle Layer", 'Top Layer']
        for i in range(len(self.board)-1,-1,-1):
            output += layer_names[i] + "\n"
            for j in range(len(self.board)-1,-1,-1):
                output += " ".join(self.board[i][j]) + '\n'
            output += (('-') * 10) + ('\n' * 2)
    
        return output
    
    def get_minimal_board_position(self):
        return str((self.board, self.next_player_c))
    
    def move(self, move):
        j, k = move

        if self.winner() is not None:
            #print(self)
            raise Exception("The game has ended")
        if move not in self.get_legal_moves():
            #print(self)
            raise Exception("Illegal Move: This column is full")
        
        for i in range(len(self.board)):
            if self.board[i][j][k] == '.':
                self.board[i][j][k] = self.next_player_c
                if self.next_player_c == 'X':
                    self.next_player_c = 'O'
                else:
                    self.next_player_c = 'X'
                return
            
    def undo_move(self, move):
        j, k = move
        
        for i in range(len(self.board)-1, -1, -1):
            if self.board[i][j][k] in ('X', 'O'):
                self.board[i][j][k] = '.'
                if self.next_player_c == 'X':
                    self.next_player_c = 'O'
                else:
                    self.next_player_c = 'X'
                return

        raise Exception(f"Tried to undo move {move} that had no letters placed")
            
        
    def get_legal_moves(self):
        if self.winner() is not None:
            return []
        
        moves = []
        for j in range(len(self.board[0])):
            for k in range(len(self.board[0][0])):
                if any(self.board[i][j][k] == '.' for i in range(len(self.board))):
                    moves.append((j,k))

        return moves
            
    def winner(self):
        for i in range(len(self.board)):
            result = self._is_layer_win(self.board[i])
            if result is not None:
                return result
        
        for j in range(len(self.board[0])):
            layer = []
            for i in range(len(self.board)):
                layer.append(self.board[i][j])
            result = self._is_layer_win(layer)
            if result is not None:
                return result

        for k in range(len(self.board[0][0])):
            layer = []
            for i in range(len(self.board)):
                strip = []
                for j in range(len(self.board[0])):
                    strip.append(self.board[i][j][k])
                layer.append(strip)
            result = self._is_layer_win(layer)
            if result is not None:
                return result

        for w in ['X', 'O']:
            if all(self.board[i][i][i] == w for i in range(len(self.board))):
                return w
            
            if all(self.board[len(layer)-1-i][i][i] == w for i in range(len(self.board))):
                return w
            
            if all(self.board[i][i][len(layer)-1-i] == w for i in range(len(self.board))):
                return w
            
        return None
    
    def eval(self):
        if self.winner() == 'X':
            return 1
        elif self.winner() == 'O':
            return -1
        else: 
            return 0

    # represents a win state on a regular tic tac toe board
    def _is_layer_win(self, layer):
        for w in ['X', 'O']:
            # k direction wins
            for j in range(len(layer)):
                if all(layer[j][k] == w for k in range(len(layer[j]))):
                    return w
                
            # j direction wins
            for k in range(len(layer[0])):
                if all(layer[j][k] == w for j in range(len(layer))):
                    return w
            
            # diagonal
            if all(layer[j][j] == w for j in range(len(layer))):
                return w

            if all(layer[j][len(layer)-1-j] == w for j in range(len(layer))):
                return w
            
        return None
    
    def get_copy():
        return
    

def minimax(board, lookup = {}, depth = 5):
    #print(f'Minimax Debug: getting board state as input at depth {depth}:')
    #print(board)
    precomputed = lookup.get(board.get_minimal_board_position())
    if precomputed is not None:
        return precomputed
    
    legal_moves = board.get_legal_moves()
    #print(f'Minimax Debug: Found {legal_moves} legal moves for board state')
    if depth == 0 or len(legal_moves) == 0:
        ##print(f'Minimax Debug: Found 0 legal moves for board state: returning evaluation {board.eval()}')
        return board.eval(), None
    
    is_maximizing = board.next_player_c == 'X'
    if is_maximizing:
        best_score = -math.inf
        best_move = None
        for move in legal_moves:
            #print(f'Minimax Debug: exploring move {move} at depth {depth}, maximizing from board state {board_copy}')
            try:
                board.move(move)
                score, _ = minimax(board, lookup, depth-1)
                if score > best_score:
                    best_score = score
                    best_move = move
                    if best_score == 1: # This indicates that we have found a move that is winning by force, we don't need to look further
                        break
            finally:
                board.undo_move(move)
        return best_score, best_move
    else:
        best_score = math.inf
        best_move = None
        for move in legal_moves:
            try:
                #print(f'Minimax Debug: exploring move {move} at depth {depth} minimizing from board state {board_copy}')
                board.move(move)
                score, _ = minimax(board, lookup, depth-1)
                if score < best_score:
                    best_score = score
                    best_move = move
                    if best_score == -1:  # This indicates that we have found a move that is winning by force, we don't need to look further
                        break
            finally:
                board.undo_move(move)
        return best_score, best_move

def main():
    board = Board()    
    
    # eval, engine_move = minimax(board)
    # print(eval, engine_move)

    print(f"Welcome to 3D Tic Tac Toe. You are {board.next_player_c} and are going first.")
    while board.winner() is None and len(board.get_legal_moves()) > 0:
        lookup = {}
        legal_move = False
        while not legal_move:
            print("Current Board State:\n\n")
            print(board)
            print("\n\n")
            print(board.get_legal_moves())
            k = int(input("X coordinate: "))
            j = int(input("Y coordinate: "))
            try:
                board.move((j,k))
                legal_move = True
            except:
                print("You must make a legal move")

        start_time = time.perf_counter()
        eval, engine_move = minimax(board, lookup)
        end_time = time.perf_counter()
        print(f"Time to find move on depth 5: {end_time-start_time:.6f}")
        if engine_move is not None:
            board.move(engine_move)

    print(f"The winner is {board.winner()}! Final position: \n {board}")
 
if __name__ == "__main__":
    main()

