# 3D Tic Tac Toe Engine

Engine which solves 3D tic tac toe.

## Backstory

Some friends and I were visiting [Michigan Central Station](https://en.wikipedia.org/wiki/Michigan_Central_Station) and they had a lovely 3D tic tac toe game that played like a 3 dimensional version of connect 4, but it is rather connect 3. You drop an X or an O and it falls to the bottom, and the first to connect horizontally, vertically, or diagonally anywhere in the grid wins. Being the nerds we are, we talked about how cool it would be to create an engine for this game. Here we are.

## How to play in the terminal

Assuming you have git installed on your computer:

```bash
git clone https://github.com/NEVED123/3DTicTacToe.git
cd 3DTicTacToe
make run_tui
```

## How to start up the server

```bash
... Same steps as above to pull package
make run_server

curl --json '{"pieces": [0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0], "isXTurn": true}' localhost:8000 # {"x":"1","y":" 1"}
```