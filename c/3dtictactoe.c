#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>

uint32_t WIN_MASKS[48];

typedef enum {
    ERR_OK = 0,
    ERR_ILLEGAL_MOVE
} ErrorCode;

typedef struct {
    int value;
    ErrorCode error;
} Result;

struct BoardState {
    unsigned int isXturn : 1; // Boolean, one bit
    uint32_t X;
    uint32_t O;
};

struct MinimaxResult {
    int eval;
    int j;
    int k;
};

int winner(struct BoardState *b) {
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

void printWinMask(u_int32_t m) {
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

void printBoardState(struct BoardState *b) { 

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

Result move(char j, char k, struct BoardState *b) {

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
Result undo_move(char j, char k, struct BoardState *b) {

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

int has_legal_moves(struct BoardState *b) {

    if (winner(b) != 0) {
        return 0;
    }

    u_int32_t all_occupied_mask = b->X | b->O;
    int layer_full_mask = 0b111111111;

    return (all_occupied_mask & layer_full_mask) != layer_full_mask;
}



int minimax(struct BoardState *b, int depth, struct MinimaxResult *r) {
    if (depth == 0 || !has_legal_moves(b)) {
        return winner(b);
    }

    char is_maximizing = b->isXturn;
    if (is_maximizing) {
        int best_score = INT_MIN;
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                Result move_result = move(j,k,b);
                if (move_result.error == ERR_ILLEGAL_MOVE) {
                    continue;
                }
                int score = minimax(b, depth-1, NULL);
                if (score > best_score) {
                    best_score = score;    
                    if (r != NULL) {
                        r->eval = best_score;
                        r->j = j;
                        r->k = k;
                    }
                }

                Result undo_move_result = undo_move(j,k,b);
                if (undo_move_result.error != ERR_OK) {
                    printf("Error: Attempted to undo move j = %d, k = %d with nothing to undo\n on this board state:\n", j, k);
                    printBoardState(b);
                    printf("Debug info: depth = %d, undo_move_result = %d, isMaximizing = %d", depth, undo_move_result.error, is_maximizing);
                    exit(EXIT_FAILURE);
                }
            }
        }
        return best_score;
    } else {
        int best_score = INT_MAX;
        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < 3; k++) {
                Result move_result = move(j,k,b);
                if (move_result.error == ERR_ILLEGAL_MOVE) {
                    continue;
                }
                int score = minimax(b, depth-1, NULL);
                if (score < best_score) {
                    best_score = score;  
                    if (r != NULL) {
                        r->eval = best_score;
                        r->j = j;
                        r->k = k;
                    }
                }
                Result undo_move_result = undo_move(j,k,b);
                if (undo_move_result.error != ERR_OK) {
                    printf("Error: Attempted to undo move j = %d, k = %d with nothing to undo\n on this board state:\n", j, k);
                    printBoardState(b);
                    printf("Debug info: depth = %d, undo_move_result = %d, isMaximizing = %d", depth, undo_move_result.error, is_maximizing);
                    exit(EXIT_FAILURE);
                }
            }
        }
        return best_score;
    }
}

int main() {
    generate_win_masks();

    struct BoardState b = {
        .isXturn = 1,
        .X = 0,
        .O = 0
    };

    struct MinimaxResult r = {0,0,0};

    int depth = 9;

    printf("Welcome to 3D Tic Tac Toe. You are X and going first");

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
}
