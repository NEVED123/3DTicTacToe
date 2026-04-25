from fastapi import FastAPI
from pydantic import BaseModel
import subprocess

class BoardState(BaseModel):
    pieces: list[int]
    isXTurn: bool

app = FastAPI()

@app.post("/")
async def root(state: BoardState):
    x_pieces = 0
    o_pieces = 0
    for i, piece in enumerate(state.pieces):
        bit_mask_value = 2**(31-i)
        if piece == 1:
            x_pieces += bit_mask_value
        if piece == 2:
            o_pieces += bit_mask_value

    result = subprocess.run(["build/3dtictactoe", str(x_pieces), str(o_pieces), str(state.isXTurn)], capture_output=True, text=True)
    x_move, y_move = tuple(result.stdout.split(","))
    return {'x': x_move, 'y': y_move}