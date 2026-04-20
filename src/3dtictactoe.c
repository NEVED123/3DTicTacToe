#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

void printBoardMask(u_int32_t m) {
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

uint32_t WIN_MASKS[48];

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
} MinimaxResult;

int winner(BoardState *b) {
    for (int i = 0; i < 48; i++) {
        if ((b->X & WIN_MASKS[i]) == WIN_MASKS[i]){
            return INT_MAX;
        }
        if ((b->O & WIN_MASKS[i]) == WIN_MASKS[i]) {
            return INT_MIN;
        }
    }

    return 0;
}

int eval(BoardState *b) {
    for (int i = 0; i < 48; i++) {
        if ((b->X & WIN_MASKS[i]) == WIN_MASKS[i]){
            return INT_MAX;
        }
        if ((b->O & WIN_MASKS[i]) == WIN_MASKS[i]) {
            return INT_MIN;
        }
    }

    // Look for 2 in a rows (I will refer to these as C2 for "connect 2") - for each 2 in a row each player has on the board, this will earn one point.
    // From this mask, since every "node" is connected to one another, we can use n(n-1)/2 to find total number of 2 in a rows.
    // The eval will be then difference between these two. In the future, we could prioritize the ones that actually
    // open up a new immediate threat rather than those that are buried and can't pose any problems.
    // Note that this currently doesn't take into account whose turn it is - i.e, X could be one turn away from winning, but if O has more c2's, then this algorithm
    int total_x_c2 = 0;
    int total_o_c2 = 0;
    uint32_t c2_mask = 0b0110110000110110000000000000; //This is a 2x2x2 cube within the 3x3x3 grid. There are only 8 places for this to fit, hence l = 2, m = 4
    for (int i = 0; i < 2; i += 1) {
        for (int j = 0; j < 2; j += 1) {
            for (int k = 0; k < 2; k += 1) {
                uint32_t x_c2_mask = b->X & c2_mask;
                int num_x_c2_nodes = __builtin_popcount(x_c2_mask); //Super overoptimization, __builtin_popcount directly calls the ASM instruction for counting number of bits set to 1
                total_x_c2 += (num_x_c2_nodes * (num_x_c2_nodes-1)) / 2; 

                uint32_t o_c2_mask = b->O & c2_mask;
                int num_o_c2_nodes = __builtin_popcount(o_c2_mask); 
                total_o_c2 +=  (num_o_c2_nodes * (num_o_c2_nodes-1)) / 2;

                c2_mask >>= 1;
            }
            c2_mask >>= 1;
        }
        c2_mask >>= 3; // This brings us up to the next layer
    }

    return total_x_c2 - total_o_c2;
}

Result move(char j, char k, BoardState *b) {

    Result r;

    int winning_player = winner(b);
    if (winning_player != 0) {
        r.error = ERR_ILLEGAL_MOVE;
        return r;
    }

    int move_mask = 1 << ((2-k) * 3 + (2-j) + 18);
    u_int32_t all_occupied_mask = b->X | b->O;
    for (int i = 0; i < 3; i++) {
        if (!(move_mask & all_occupied_mask)) {
            if (b->isXturn) {
                b->X |= move_mask;
            } else {
                b->O |= move_mask;
            }

            b->isXturn ^= 1;
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
            r.error = ERR_OK;
            return r;
        }
        if (b->O & move_mask) {
            b->O ^= move_mask;
            b->isXturn ^= 1;
            r.error = ERR_OK;
            return r;
        }
        move_mask <<= 9; 
    }

    r.error = ERR_ILLEGAL_MOVE;
    return r;
}

void get_move_order(BoardState *b, MinimaxResult *moves_buffer) {
    // At most, we need to sort 9 moves
    int isXturn = b->isXturn;

    // We must initialize the buffer before attempting to sort again
    for (int i = 0; i < 9; i++) {
        if (isXturn)
            moves_buffer[i].eval = INT_MIN;
        else {
            moves_buffer[i].eval = INT_MAX;
        }
        moves_buffer[i].j = 0;
        moves_buffer[i].k = 0;
    }

    for (int j = 0; j < 3; j += 1) {
        for (int k = 0; k < 3; k += 1) {
            
            Result move_result = move(j,k,b);
            if (move_result.error == ERR_ILLEGAL_MOVE) {
                continue;
            }

            int move_evaluation = eval(b);

            // Use insertion sort to put the move in its correct spot
            // heap sort may be slightly faster here but the difference would be negligable since n is at most 9 and not worth the additional complexity for now.
            MinimaxResult curr_eval = { .eval = move_evaluation, .j = j, .k = k };
            for (int l = 0; l < 9; l += 1) {
                // Sort descending order if it is X move (maximizing)
                // Sort ascending order if it is O move (minimizing)
                if ((isXturn && curr_eval.eval > moves_buffer[l].eval) || (!isXturn && curr_eval.eval < moves_buffer[l].eval)) {
                    MinimaxResult temp_eval = moves_buffer[l];
                    moves_buffer[l] = curr_eval;
                    curr_eval = temp_eval;
                }

                // If we have just replaced an INT_MIN or INT_MAX, it means we are at the end of the list, no need to continue
                // replacing
                if (isXturn && curr_eval.eval == INT_MIN || !isXturn && curr_eval.eval == INT_MAX) {
                    break;
                }
            }

            undo_move(j,k,b);
        }
    }
}

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

    // diagonal through all planes (3)
    WIN_MASKS[w] = 0b100000000000010000000000001;
    w += 1;
    WIN_MASKS[w] = 0b001000000000010000000000100;
    w += 1;
    WIN_MASKS[w] = 0b000000100000010000001000000;
}


void printBoardState(BoardState *b) { 

    int winning_player = winner(b);
    if (winning_player == INT_MAX) {
        printf("X has won the game!\n\n");
    }
    else if (winning_player == INT_MIN) {
        printf("O has won the game!\n\n");
    } else {
        if (b->isXturn) 
            printf("X");
        else
            printf("O");
        printf(" to play\n\n");
    }

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



int has_legal_moves(BoardState *b) {

    if (winner(b) != 0) {
        return 0;
    }

    u_int32_t all_occupied_mask = b->X | b->O;
    int layer_full_mask = 0b111111111;

    return (all_occupied_mask & layer_full_mask) != layer_full_mask;
}

int minimax_internal(BoardState *b, int depth, MinimaxResult *r, MinimaxResult *moves_buffer) {
    if (depth == 0 || !has_legal_moves(b)) {
        return winner(b);
    }

    char is_maximizing = b->isXturn;
    if (is_maximizing) {
        int best_score = INT_MIN;
        get_move_order(b, moves_buffer);
        for (int l = 0; l < 9; l++) {
            MinimaxResult curr_move = moves_buffer[l];
            Result move_result = move(curr_move.j,curr_move.k,b);
            if (move_result.error == ERR_ILLEGAL_MOVE) {
                continue;
            }
            int score = minimax_internal(b, depth-1, NULL, moves_buffer);
            if (score > best_score) {
                best_score = score;    
                if (r != NULL) {
                    r->eval = best_score;
                    r->j = curr_move.j;
                    r->k = curr_move.k;
                }
            }

            Result undo_move_result = undo_move(curr_move.j,curr_move.k,b);
            if (undo_move_result.error != ERR_OK) {
                printf("Error: Attempted to undo move j = %d, k = %d with nothing to undo\n on this board state:\n", curr_move.j, curr_move.k);
                printBoardState(b);
                printf("Debug info: depth = %d, undo_move_result = %d, isMaximizing = %d", depth, undo_move_result.error, is_maximizing);
                exit(EXIT_FAILURE);
            }

            if (best_score == INT_MAX) {
                break; // We have found a move that is winning by force, no need to explore other branches
            }            
        }
        return best_score;
    } else {
        int best_score = INT_MAX;
        get_move_order(b, moves_buffer);
        for (int l = 0; l < 9; l++) {
            MinimaxResult curr_move = moves_buffer[l];
            Result move_result = move(curr_move.j,curr_move.k,b);
            if (move_result.error == ERR_ILLEGAL_MOVE) {
                continue;
            }
            int score = minimax_internal(b, depth-1, NULL, moves_buffer);
            if (score < best_score) {
                best_score = score;  
                if (r != NULL) {
                    r->eval = best_score;
                    r->j = curr_move.j;
                    r->k = curr_move.k;
                }
            }
            Result undo_move_result = undo_move(curr_move.j,curr_move.k,b);
            if (undo_move_result.error != ERR_OK) {
                printf("Error: Attempted to undo move j = %d, k = %d with nothing to undo\n on this board state:\n", curr_move.j, curr_move.k);
                printBoardState(b);
                printf("Debug info: depth = %d, undo_move_result = %d, isMaximizing = %d", depth, undo_move_result.error, is_maximizing);
                exit(EXIT_FAILURE);
            }

            if (best_score == INT_MIN) {
                break; // We have found a move that is winning by force, no need to explore other branches
            }
        }
        return best_score;
    }
}

int minimax(BoardState *b, int depth, MinimaxResult *r) {
    MinimaxResult *moves_buffer = malloc(9 * sizeof(MinimaxResult));
    int eval = minimax_internal(b, depth, r, moves_buffer);
    free(moves_buffer);
    return eval;
}

int main() {
    generate_win_masks();

    BoardState b = {
        .isXturn = 1,
        .X = 0,
        .O = 0
    };

    MinimaxResult r = {0,0,0};

    // move(0,0,&b);
    // move(1,1,&b);
    // move(2,0,&b);
    // move(2,2,&b);

    // printBoardState(&b);

    // printf("%d\n", eval(&b));

    // get_move_order(&b, moves_buffer);

    // for (int l = 0; l < 9; l += 1) {
    //     MinimaxResult move = moves_buffer[l];
    //     printf("Move %d: eval = %d, j = %d, k = %d\n", (l + 1), move.eval, move.j, move.k);   
    // }

    int depth = 8;
    printf("\n\nWhat depth would you like the engine to search to? Note that anything beyond depth 8 will take at least 30 seconds per move: \n\n");
    scanf("%d", &depth);

    printf("Welcome to 3D Tic Tac Toe. You are X and going first. X coordinate is 0 to 2 from left to right, and Y coordinate is 0 to 2 from bottom to top\n\n");

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
                printf("You must made a legal move \n\n");
            }
        }

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        minimax(&b, depth, &r);
        clock_gettime(CLOCK_MONOTONIC, &end);
        double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
        printf("Time to find move on depth %d: %f seconds\n", depth, elapsed);
        
        move(r.j, r.k, &b);
    }

    printf("Game ended: \n\n");
    printBoardState(&b);

    return 0;
}
