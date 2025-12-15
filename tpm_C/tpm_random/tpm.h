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

void ErrorHandling(const char *msg);
int send_all(int sock, const void *buf, size_t len);
int recv_all(int sock, void *buf, size_t len);
void init_tpm(TPM *tpm);
void generate_inputs(int inputs[K][N]);
int sgn(int x);
void calculate_tau(TPM *tpm, int inputs[K][N]);
void update_weights(TPM *tpm, int theta[K][N]);
void print_weights(const TPM *tpm, const char *name);

#endif