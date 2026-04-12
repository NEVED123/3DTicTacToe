class Board:
    board = [[['.', '.', '.'] for _ in range(3)] for _ in range(3)]

    # Array should always be 3x3x3
    def __str__(self):  
        output = '=' * 10 + '\n' * 2
        layer_names = ["Bottom Layer", "Middle Layer", 'Top Layer']
        for i, layer in list(enumerate(self.board))[::-1]:
            output += layer_names[i] + "\n"
            for slice in layer[::-1]:
                output += " ".join(slice) + '\n'
            output += (('-') * 10) + ('\n' * 2)
    
        return output
    
    def move(self, x, y, c):
        for layer in self.board:
            if layer[x][y] == '.':
                layer[x][y] = c
                return
        # if we make it here, this is not a legal move
        raise Exception("This column is full")
    
    def is_draw(self):
        for layer in self.board:
            for slice in layer:
                if '.' in slice:
                    return False
        return True
        
    # 1 if X won, -1 if O won, 0 if not won, -2 if drawn (board filled up)

    '''
    Types of wins:
    horizontal within the layer (2 directions)
    diagonal within the layer (2 directions)
    '''
    def eval():
        return

    # represents a win state on a regular tic tac toe board
    def _is_layer_win(self):
        for w in ['X', 'O']:
            for layer in self.board:
                # horizontal
                for s in layer:
                    if all(c == w for c in s):
                        return w
                    
                # vertical
                for i in range(len(layer[0])):
                    if all(layer[j][i] == w for j in range(len(layer))):
                        return w
                
                # diagonal
                if all(layer[i][i] == w for i in range(len(layer))):
                    return w

                if all(layer[i][len(layer)-1-i] == w for i in range(len(layer))):
                    return w
        
        return None

board = Board()

board.move(0,0,'O')
board.move(1,1,'O')
board.move(2,0,'O')

print(board)
print(board._is_layer_win())