#include "tpm.h"

int sgn(int x) {
    return (x >= 0) ? 1 : -1;
}

void init_tpm(TPM *tpm) {
    for (int k = 0; k < K; k++) {
        for (int n = 0; n < N; n++) {
            int w = 0;
            while (w == 0)
                w = (rand() % (2 * L + 1)) - L;
            tpm->weights[k][n] = w;
        }
    }
}

void generate_inputs(int inputs[K][N]) {
    for (int k = 0; k < K; k++) {
        for (int n = 0; n < N; n++) {
            inputs[k][n] = (rand() % 2) ? 1 : -1;
        }
    }
}

void calculate_tau(TPM *tpm, int inputs[K][N]) {
    tpm->tau = 1;
    for (int k = 0; k < K; k++) {
        int sum = 0;
        for (int n = 0; n < N; n++)
            sum += tpm->weights[k][n] * inputs[k][n];

        tpm->sigma[k] = sgn(sum);
        tpm->tau *= tpm->sigma[k];
    }
}

void update_weights(TPM *tpm, int theta[K][N]) {
    for (int k = 0; k < K; k++) {
        if (tpm->sigma[k] == tpm->tau) {
            for (int n = 0; n < N; n++) {
                tpm->weights[k][n] += theta[k][n];
                if (tpm->weights[k][n] > L) tpm->weights[k][n] = L;
                if (tpm->weights[k][n] < -L) tpm->weights[k][n] = -L;
            }
        }
    }
}

void print_weights(const TPM *tpm, const char *name) {
    printf("%s's Weights \n", name);
    for (int k = 0; k < K; k++) {
        printf("k=%d: [", k);
        for (int n = 0; n < N; n++)
            printf("%3d", tpm->weights[k][n]);
        printf(" ]\n");
    }
    printf("\n");
}

long long get_weights_checksum(const TPM *tpm) {
    long long sum = 0;
    for (int k = 0; k < K; k++)
        for (int n = 0; n < N; n++)
            sum += (k + 1) * (n + 1) * tpm->weights[k][n];
    return sum;
}

void generate_query_inputs(TPM *tpm, int x[K][N], int H)
{
    for (int k = 0; k < K; k++)
        for (int n = 0; n < N; n++)
            x[k][n] = (rand() % 2) ? 1 : -1;

    int target_k = rand() % K;
    int max_iter = 200;

    while (max_iter--) {
        int h = 0;
        for (int n = 0; n < N; n++)
            h += tpm->weights[target_k][n] * x[target_k][n];

        if (abs(abs(h) - H) <= 1)
            return;

        int n = rand() % N;
        x[target_k][n] = -x[target_k][n];
    }

    for (int k = 0; k < K; k++)
        for (int n = 0; n < N; n++)
            x[k][n] = (rand() % 2) ? 1 : -1;
}