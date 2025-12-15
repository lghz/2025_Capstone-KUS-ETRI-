#ifndef TPM_COMMON_H
#define TPM_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#define K 3
#define N 4
#define L 3

#define BUFSIZE 1024

typedef struct {
    int weights[K][N];
    int sigma[K];    
    int tau;
} TPM;

void init_tpm(TPM *tpm);
void generate_inputs(int inputs[K][N]);
int sgn(int x);
void calculate_tau(TPM *tpm, int inputs[K][N]);
void update_weights(TPM *tpm, int theta[K][N]);
void print_weights(const TPM *tpm, const char *name);
long long get_weights_checksum(const TPM *tpm);

// ★ Query 기능 선언
void generate_query_inputs(TPM *tpm, int x[K][N], int H);

#endif