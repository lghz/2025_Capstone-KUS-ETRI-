#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "tpm.h"

void ErrorHandling(const char *msg) {
    perror(msg);
    exit(1);
}

int send_all(int sock, const void *buf, size_t len) {
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t n = send(sock, (const char *)buf + total_sent, len - total_sent, 0);
        if (n < 0) {
            perror("send_all");
            return -1;
        }
        if (n == 0) return 0;
        total_sent += n;
    }
    return 1;
}

int recv_all(int sock, void *buf, size_t len) {
    size_t total_rcvd = 0;
    while (total_rcvd < len) {
        ssize_t n = recv(sock, (char *)buf + total_rcvd, len - total_rcvd, 0);
        if (n < 0) {
            perror("recv_all");
            return -1;
        }
        if (n == 0) return 0;
        total_rcvd += n;
    }
    return 1;
}

void init_tpm(TPM *tpm) {
    for (int k = 0; k < K; k++) {
        for (int n = 0; n < N; n++) {
            int w = 0;
            while (w == 0) {
                w = (rand() % (2 * L + 1)) - L;
            }
            tpm->weights[k][n] = w;
        }
    }
}

void generate_inputs(int inputs[K][N]) {
}

int sgn(int x) {
    return (x >= 0) ? 1 : -1;
}

void calculate_tau(TPM *tpm, int inputs[K][N]) {
    tpm->tau = 1;
    for (int k = 0; k < K; k++) {
        int sum = 0;
        for (int n = 0; n < N; n++) {
            sum += tpm->weights[k][n] * inputs[k][n];
        }
        tpm->sigma[k] = sgn(sum);
        tpm->tau *= tpm->sigma[k];
    }
}

void update_weights(TPM *tpm, int theta[K][N]) {
    for (int k = 0; k < K; k++) {
        if (tpm->sigma[k] == tpm->tau) {
            for (int n = 0; n < N; n++) {
                tpm->weights[k][n] += theta[k][n];;
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
        for (int n = 0; n < N; n++) {
            printf("%3d", tpm->weights[k][n]);
        }
        printf(" ]\n");
    }
    printf("\n");
}

long long get_weights_checksum(const TPM *tpm) {
    long long checksum = 0;
    for (int k = 0; k < K; k++) {
        for (int n = 0; n < N; n++) {
            checksum += (k + 1) * (n + 1) * tpm->weights[k][n];
        }
    }
    return checksum;
}

int main() {
    int sock;
    struct sockaddr_in servAddr;
    char server_ip[64];
    int server_port;
    char message[BUFSIZE];
    int nRcv;

    TPM tpm_B;
    int inputs[K][N];
    int theta[K][N];
    int tau_A;
    char sync_status[10];

    srand((unsigned int)time(NULL) + getpid());

    printf("Server Address: ");
    if (scanf("%63s", server_ip) != 1) return 1;
    printf("Port Number: ");
    if (scanf("%d", &server_port) != 1) return 1;
    getchar(); 

    init_tpm(&tpm_B);
    printf("[Client] Initialization complete\n");
    print_weights(&tpm_B, "Client Initial");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) ErrorHandling("socket");

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &servAddr.sin_addr) <= 0)
        ErrorHandling("inet_pton");

    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        ErrorHandling("connect");

    printf("Connected to %s:%d\n", server_ip, server_port);

    printf("\n Key Synchronization Start\n");
    int iteration = 0;
    while (1) {
        iteration++;
        printf("\n[Iteration %d]\n", iteration);
        
        if (recv_all(sock, inputs, sizeof(inputs)) <= 0) break;
        if (recv_all(sock, theta, sizeof(theta)) <= 0) break;
        if (recv_all(sock, &tau_A, sizeof(tau_A)) <= 0) break;
        printf("  Received Tau: %d\n", tau_A);

        calculate_tau(&tpm_B, inputs);
        printf("  Client Tau: %d (status: %lld)\n", tpm_B.tau, get_weights_checksum(&tpm_B));

        if (send_all(sock, &tpm_B.tau, sizeof(tpm_B.tau)) <= 0) break;

        if (tau_A == tpm_B.tau) {
            printf("  > Taus match! Updating weights...\n");

            update_weights(&tpm_B, theta);
        } else {
            printf("  > Taus mismatch. No update.\n");
        }

        if (send_all(sock, &tpm_B, sizeof(TPM)) <= 0) break;

        if (recv_all(sock, sync_status, sizeof(sync_status)) <= 0) break;

        if (strncmp(sync_status, "SYNC_OK", 7) == 0) {
            printf("\nSynchronization Achieved! (Iter: %d) \n", iteration);
            break;
        } else {
             printf("  > Weights not synced yet.\n");
        }
    }

    print_weights(&tpm_B, "Client Synced");

    printf("\nChat Started\n");

    while (1) {
        printf("message: ");
        if (fgets(message, BUFSIZE, stdin) == NULL) break;
        message[strcspn(message, "\n")] = '\0';

        if (send(sock, message, (int)strlen(message), 0) < 0) {
            perror("send");
            break;
        }
        if (strcmp(message, "exit") == 0) break;

        nRcv = recv(sock, message, BUFSIZE - 1, 0);
        if (nRcv <= 0) {
            printf("Server closed connection.\n");
            break;
        }
        message[nRcv] = '\0';
        
        if (strcmp(message, "exit") == 0) {
            printf("Server requested exit.\n");
            break;
        }
        printf("Received: %s\n", message);
    }

    close(sock);
    return 0;
}
