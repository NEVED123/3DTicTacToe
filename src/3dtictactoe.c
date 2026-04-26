#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#define BASE_MAX_WIN_SCORE 100;
#define BASE_MIN_WIN_SCORE -100;

// Structs

typedef enum {
    ERR_OK = 0,
    ERR_ILLEGAL_MOVE
} ErrorCode;

typedef struct {
    int value;
    ErrorCode error;
} Result;

typedef struct {
    unsigned int isXturn : 1; // Boolean, one bit
    uint32_t X;
    uint32_t O;
} BoardState;

typedef struct {
    int eval;
    int j;
    int k;
    char is_sentinal;
} MinimaxResult;

uint32_t WIN_MASKS[49];

// Printers

void printBoardMask(uint32_t m) {
    for (int i = 31; i > -1; i--) {
        int k = m >> i; // Shift right by i positions
        if (i == 26)
            printf("| ");
        if (i < 26 && (i+1) % 9 == 0)
            printf("  ");
        else if (i < 26 && (i+1) % 3 == 0)
            printf(" ");
        if (k & 1)      // Check if the last bit is 1
            printf("1");
        else
            printf("0");
    }
    printf("\n");
}

void printBoardState(BoardState *b) { 

    if (b->isXturn) 
        printf("X");
    else
        printf("O");
    printf(" to play\n\n");

    for (int l = 0; l < 9; l++) {
        int bit_mask = 1 << ((3 * (l + 1)) - 1);
        for (int k = 0; k < 3; k++){
            if (b->X & bit_mask)
                printf("X ");
            else if (b->O & bit_mask)
                printf("O ");
            else
                printf(". ");
            
            if ((k + 1) % 3 == 0)
                printf("\n");

            bit_mask >>= 1;
        }

        if ((l+1) % 3 == 0)
            printf("-------- \n");
    }
}

// Asserts

void assert_legal_position(BoardState *b) {
    // Make sure no collision between X and O bitboard
    if (b->X & b->O) {
        printf("Overlap detected!\n");
        printBoardState(b);
        assert(0);
    }

    // Make sure there are no floating pieces
    for (int k = 0; k < 3; k++) {
        for (int j = 0; j < 3; j++) {
            int move_mask = 1 << ((2-k) * 3 + (2-j) + 18);
            char has_pieces = 0;
            if (b->X & move_mask || b->O & move_mask) {
                has_pieces = 1;
            }

            // i = 1, since we already checked the first layer
            // if has_pieces = 0 and we come across a piece, we have a floater
            for (int i = 1; i < 3; i++) {
                move_mask >>= 9;
                if (!has_pieces && (b->X & move_mask || b->O & move_mask)){
                    printf("Floating piece detected!\n");
                    printBoardState(b);
                    assert(0);
                }
                
                // if has_pieces = 1 and we don't have a piece, we set has_pieces to 0
                if (has_pieces && !(b->X & move_mask || b->O & move_mask)) {
                    has_pieces = 0;
                }
            }
        }
    }

    assert(1);
}

void assert_board_equal(BoardState *a, BoardState *b) {
    if (a->X != b->X || a->O != b->O || a->isXturn != b->isXturn) {
        printf("Board mismatch!\n");
        printf("Expected:\n");
        printBoardState(a);
        printf("Got:\n");
        printBoardState(b);
        assert(0);
    }
}

// Board State Evaluation
// Depth is used to calculate how quickly we won for evaluation. I.E a win at a
// shallower depth is a quicker win. This is so the engine if knows that it is losing by force, so that it can still distinguish between
// holding moves and moves that lose immediately.
int eval(BoardState *b, int depth) {
    for (int i = 0; i < 49; i++) {
        if ((b->X & WIN_MASKS[i]) == WIN_MASKS[i]){
            return (depth+1) * BASE_MAX_WIN_SCORE;
        }
        if ((b->O & WIN_MASKS[i]) == WIN_MASKS[i]) {
            return (depth+1) * BASE_MIN_WIN_SCORE;
        }
    }

    return 0;
}

int has_legal_moves(BoardState *b) {

    if (eval(b, 1) != 0) {
        return 0;
    }

    uint32_t all_occupied_mask = b->X | b->O;
    int layer_full_mask = 0b000000000000000000111111111;

    return (all_occupied_mask & layer_full_mask) != layer_full_mask;
}

// Board State Operations

Result move(char j, char k, BoardState *b) {

    Result r;

    int winning_player = eval(b, 1);
    if (winning_player != 0) {
        r.error = ERR_ILLEGAL_MOVE;
        return r;
    }

    int move_mask = 1 << ((2-k) * 3 + (2-j) + 18);
    uint32_t all_occupied_mask = b->X | b->O;
    for (int i = 0; i < 3; i++) {
        if (!(move_mask & all_occupied_mask)) {
            if (b->isXturn) {
                b->X |= move_mask;
            } else {
                b->O |= move_mask;
            }

            b->isXturn ^= 1;
            assert_legal_position(b);
            r.error = ERR_OK;
            return r;
        }
        move_mask >>= 9;
    }
    r.error = ERR_ILLEGAL_MOVE;
    return r;
}

// This method makes you pinky promise that you are respecting game move stack
Result undo_move(char j, char k, BoardState *b) {

    Result r;
    int move_mask = 1 << ((2-k) * 3 + (2-j));
    for (int i = 0; i < 3; i++) {
        if (b->X & move_mask) {
            b->X ^= move_mask;
            b->isXturn ^= 1;
            assert_legal_position(b);
            r.error = ERR_OK;
            
            return r;
        }
        if (b->O & move_mask) {
            b->O ^= move_mask;
            b->isXturn ^= 1;
            assert_legal_position(b);
            r.error = ERR_OK;
            return r;
        }
        move_mask <<= 9; 
    }

    r.error = ERR_ILLEGAL_MOVE;
    return r;
}

// Minimax Functions

void get_move_order(BoardState *b, MinimaxResult *moves_buffer) {
    // At most, we need to sort 9 moves
    int isXturn = b->isXturn;

    for (int i = 1; i < 9; i++) {
        moves_buffer[i] = (MinimaxResult){
            .eval = isXturn ? INT_MIN : INT_MAX,
            .j = 0,
            .k = 0,
            .is_sentinal = 0
        };
    }
    moves_buffer[0].is_sentinal = 1;

    for (int j = 0; j < 3; j += 1) {
        for (int k = 0; k < 3; k += 1) {
            
            Result move_result = move(j,k,b);
            if (move_result.error == ERR_ILLEGAL_MOVE) {
                continue;
            }

            int move_evaluation = eval(b, 1);
            
            // Use insertion sort to put the move in its correct spot
            // heap sort may be slightly faster here but the difference would be negligable since n is at most 9 and not worth the additional complexity for now.
            MinimaxResult curr_move = { .eval = move_evaluation, .j = j, .k = k, .is_sentinal = 0 };
            for (int l = 0; l < 9; l += 1) {
                // Sort descending order if it is X move (maximizing)
                // Sort ascending order if it is O move (minimizing)
                if ((isXturn && curr_move.eval > moves_buffer[l].eval) || (!isXturn && curr_move.eval < moves_buffer[l].eval) || moves_buffer[l].is_sentinal) {
                    MinimaxResult temp_move = moves_buffer[l];
                    moves_buffer[l] = curr_move;
                    curr_move = temp_move;
                }

                // If we hit the sentinal, either
                // 1. We are at the end of the list (l = 8), and have just placed the last legal move of our entire list. In this case we do nothing - technically breaking here isn't necessary since this would only happen if we are at the end
                // 2. We are not at the end of the list (l < 8), and may or may not have more moves incoming. In this case, we should place the sentinal ahead of the current entry and break out of the loop
                if (curr_move.is_sentinal) {
                    if (l < 8) {
                        moves_buffer[l+1] = curr_move;
                    }
                    break;
                }
            }

            undo_move(j,k,b);
        }
    }
}

int minimax(BoardState *b, int depth, int alpha, int beta, MinimaxResult *r, int *num_moves) {

    if (!has_legal_moves(b)) {
        return eval(b, depth);
    }

    MinimaxResult *moves_buffer = malloc(9 * sizeof(MinimaxResult));
    get_move_order(b, moves_buffer);

    char is_maximizing = b->isXturn;
    if (is_maximizing) {
        *num_moves += 1;
        int best_score = INT_MIN;
        for (int l = 0; l < 9; l++) {
            
            MinimaxResult curr_move = moves_buffer[l];

            if (curr_move.is_sentinal) {
                break;
            }

            BoardState before = {
                .X = b->X,
                .O = b->O,
                .isXturn = b->isXturn
            };

            Result move_result = move(curr_move.j,curr_move.k,b);
            if (move_result.error == ERR_ILLEGAL_MOVE) {
                printf("Error: Attempted to make move j = %d, k = %d with on this board state:\n", curr_move.j, curr_move.k);
                printBoardState(b);
                printf("Debug info: move_result = %d, isMaximizing = %d\n", move_result.error, is_maximizing);
                exit(EXIT_FAILURE);
            }

            int score = minimax(b, depth-1, alpha, beta, NULL, num_moves);

            if (score > best_score) {
                best_score = score;    
                if (r != NULL) {
                    r->eval = best_score;
                    r->j = curr_move.j;
                    r->k = curr_move.k;
                }
            }

            Result undo_move_result = undo_move(curr_move.j, curr_move.k, b);
            assert(undo_move_result.error == ERR_OK);

            BoardState after = {
                .X = b->X,
                .O = b->O,
                .isXturn = b->isXturn
            };

            assert_board_equal(&after, &before);
            
            if (best_score > beta) {
                break;
            }

            if (best_score > alpha) {
                alpha = best_score;
            }
        }
        free(moves_buffer);
        return best_score;
    } else {
        int best_score = INT_MAX;
        for (int l = 0; l < 9; l++) {
            *num_moves += 1;
            MinimaxResult curr_move = moves_buffer[l];

            if (curr_move.is_sentinal) {
                break;
            }

            BoardState before = {
                .X = b->X,
                .O = b->O,
                .isXturn = b->isXturn
            };

            Result move_result = move(curr_move.j,curr_move.k,b);
            if (move_result.error == ERR_ILLEGAL_MOVE) {
                printf("Error: Attempted to make move j = %d, k = %d with on this board state:\n", curr_move.j, curr_move.k);
                printBoardState(b);
                printf("Debug info: move_result = %d, isMaximizing = %d\n", move_result.error, is_maximizing);
                exit(EXIT_FAILURE);
            }

            int score = minimax(b, depth-1, alpha, beta, NULL, num_moves);
            if (score < best_score) {
                best_score = score;  
                if (r != NULL) {
                    r->eval = best_score;
                    r->j = curr_move.j;
                    r->k = curr_move.k;
                }
            }

            Result undo_move_result = undo_move(curr_move.j, curr_move.k, b);
            assert(undo_move_result.error == ERR_OK);

            BoardState after = {
                .X = b->X,
                .O = b->O,
                .isXturn = b->isXturn
            };
            
            assert_board_equal(&after, &before);

            if (best_score < alpha) {
                break;
            }

            if (best_score < beta) {
                beta = best_score;
            }
        }
        free(moves_buffer);
        return best_score;
    }
}

// Misc Helpers

// Indexing is treating the mask as though it were a i*j*k array.
// l is just when i'm not iterating cleanly through i,j,k.
void generate_win_masks() {
    int w = 0;

    // k direction wins (9)
    int k_win_mask = 0b111;
    for (int l = 0; l < 9; l++) {
        WIN_MASKS[w] = k_win_mask;
        w += 1;
        k_win_mask <<= 3;
    }

    // j direction wins (9)
    int j_win_mask = 0b001001001;
    for (int l = 0; l < 3; l++) {
        for (int m = 0; m < 3; m++) {
            WIN_MASKS[w] = j_win_mask;
            w += 1;
            j_win_mask <<= 1;
        }
        j_win_mask <<= 6;
    }

    // i direction wins (9)
    int i_win_mask = 0b000000001000000001000000001;
    for (int l = 0; l < 9; l++) {
        WIN_MASKS[w] = i_win_mask;
        w += 1;
        i_win_mask <<= 1;
    }

    // diagonal on i plane (6)
    int i_diag_up_mask = 0b100010001;
    int i_diag_down_mask = 0b001010100;
    for (int l = 0; l < 3; l++) {
        WIN_MASKS[w] = i_diag_up_mask;
        w += 1;
        WIN_MASKS[w] = i_diag_down_mask;
        w += 1;
        i_diag_up_mask <<= 9;
        i_diag_down_mask <<= 9;
    }

    // diagonal on j plane (6)
    int j_diag_up_mask = 0b000000100000000010000000001;
    int j_diag_down_mask = 0b000000001000000010000000100;
    for (int l = 0; l < 3; l++) {
        WIN_MASKS[w] = j_diag_up_mask;
        w += 1;
        WIN_MASKS[w] = j_diag_down_mask;
        w += 1;
        j_diag_up_mask <<= 3;
        j_diag_down_mask <<= 3;
    }

    // diagonal on the k plane (6)
    int k_diag_up_mask = 0b001000000000001000000000001;
    int k_diag_down_mask = 0b000000001000001000001000000;
    for (int l = 0; l < 3; l++) {
        WIN_MASKS[w] = k_diag_up_mask;
        w += 1;
        WIN_MASKS[w] = k_diag_down_mask;
        w += 1;
        k_diag_up_mask <<= 1;
        k_diag_down_mask <<= 1;
    }

    // diagonal through all planes (4)
    WIN_MASKS[w] = 0b100000000000010000000000001;
    w += 1;
    WIN_MASKS[w] = 0b001000000000010000000000100;
    w += 1;
    WIN_MASKS[w] = 0b000000100000010000001000000;
    w += 1;
    WIN_MASKS[w] = 0b000000001000010000100000000;
}

// Runnables

int ThreeDTicTacToeTUI() {

    printf("Welcome to 3D Tic Tac Toe. Would you like to go first (X) or second (O)? X coordinate is 0 to 2 from left to right, and Y coordinate is 0 to 2 from bottom to top\n");

    char user_turn;

    while (1) {
        printf("Choose X or O: ");
        user_turn = getchar();

        // consume leftover characters (like newline)
        while (getchar() != '\n');

        if (user_turn == 'X' || user_turn == 'O') {
            break;
        }

        printf("You must choose \"X\" or \"O\"\n");
    }

    BoardState b = {
        .isXturn = 1,
        .X = 0,
        .O = 0
    };

    MinimaxResult r = {0,0,0};

    int depth = 27;

    int num_moves = 0;
    
    if (user_turn == 'O') {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        minimax(&b, depth, INT_MIN, INT_MAX, &r, &num_moves);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("Time to find move: %f seconds\n", elapsed);
        printf("Number of moves explored: %d\n", num_moves);
        printf("Eval: %d\n", r.eval);
        num_moves = 0;
        move(r.j, r.k, &b);
    }

    while (has_legal_moves(&b)) {
        char user_made_legal_move = 0;
        while (!user_made_legal_move) {
            printBoardState(&b);
            printf("\n");
            int j;
            printf("X coordinate: ");
            scanf("%d", &j);

            int k;
            printf("Y coordinate: ");
            scanf("%d", &k);

            Result move_result = move(j, k, &b);
            if (move_result.error == ERR_OK) {
                user_made_legal_move = 1;
            }  else {
                printf("You must make a legal move \n\n");
            }
        }

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        minimax(&b, depth, INT_MIN, INT_MAX, &r, &num_moves);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("Time to find move: %f seconds\n", elapsed);
        printf("Number of moves explored: %d\n", num_moves);
        printf("Eval: %d\n", r.eval);
        num_moves = 0;
        move(r.j, r.k, &b);
    }

    printf("Game ended: \n\n");
    printBoardState(&b);

    return 0;
}

int main(int argc, char *argv[]) {
    generate_win_masks();

    if (TUI) {
        return ThreeDTicTacToeTUI();
    }

    uint32_t x = atoi(argv[1]);
    uint32_t o = atoi(argv[2]);
    uint32_t isXturn = atoi(argv[3]);

    BoardState b = {
        .isXturn = isXturn,
        .X = x,
        .O = o
    };

    int depth = 27;
    int num_moves = 0;

    MinimaxResult r = {0,0,0};
    minimax(&b, depth, INT_MIN, INT_MAX, &r, &num_moves);

    printf("%d, %d", r.j, r.k);
    return 0;
}
