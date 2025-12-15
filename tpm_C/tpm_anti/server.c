#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/resource.h>
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

long get_memory_usage_kb() {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024;
}

void print_bar_graph(const char* label, long value, long maxValue) {
    printf("%-20s | ", label);
    int bar_length = (int)((50.0 * value) / maxValue);
    for (int i = 0; i < bar_length; i++) printf("█");
    for (int i = bar_length; i < 50; i++) printf(" ");
    printf(" %ld\n", value);
}

void show_result_graph(int sync_iter, int rep, long mem) {
    printf("\n=========== TPM RESULT GRAPH ===========\n\n");
    print_bar_graph("Sync Iterations", sync_iter, sync_iter + 10);
    print_bar_graph("Repulsive Steps", rep, sync_iter);
    print_bar_graph("Memory Usage(KB)", mem, mem + 200);
    printf("\n========================================\n\n");
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
    for (int k = 0; k < K; k++) {
        for (int n = 0; n < N; n++) {
            inputs[k][n] = (rand() % 2) * 2 - 1;
        }
    }
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
                tpm->weights[k][n] -= theta[k][n];
                if (tpm->weights[k][n] > L) tpm->weights[k][n] = L;
                if (tpm->weights[k][n] < -L) tpm->weights[k][n] = -L;
            }
        }
    }
}

void print_weights(const TPM *tpm, const char *name) {
    printf("--- %s's Weights (Key) ---\n", name);
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

int main(int argc, char **argv) {
    int servSock = -1, clntSock = -1;
    struct sockaddr_in servAddr, clntAddr;
    char message[BUFSIZE];
    socklen_t clntAddrLen;
    int nRcv;
    int port = 0;

    TPM tpm_A;
    int inputs[K][N];
    int theta[K][N];
    int tau_B;
    TPM tpm_B_weights;

    int sync_iterations = 0;
    int repulsive_steps = 0;
    long memory_used = 0;

    srand((unsigned int)time(NULL));

    if (argc == 2) {
        port = atoi(argv[1]);
    } else {
        printf("Port Number : ");
        if (scanf("%d", &port) != 1) {
            fprintf(stderr, "Invalid port\n");
            return 1;
        }
        getchar();
    }

    init_tpm(&tpm_A);
    printf("[Server] Initialization complete.\n");
    print_weights(&tpm_A, "Server Initial");

    servSock = socket(AF_INET, SOCK_STREAM, 0);
    if (servSock < 0) ErrorHandling("socket");

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(port);

    if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        ErrorHandling("bind");

    if (listen(servSock, 5) < 0)
        ErrorHandling("listen");

    printf("Waiting for connection on port %d...\n", port);

    clntAddrLen = sizeof(clntAddr);
    clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
    if (clntSock < 0)
        ErrorHandling("accept");

    printf("Client %s connected!\n", inet_ntoa(clntAddr.sin_addr));

    printf("\n Synchronization Start \n");
    int iteration = 0;
    while (1) {
        iteration++;
        sync_iterations++;

        printf("\n[Iteration %d]\n", iteration);

        generate_inputs(inputs);
        generate_inputs(theta);

        calculate_tau(&tpm_A, inputs);
        printf("  Server Tau: %d(status: %lld)\n", tpm_A.tau, get_weights_checksum(&tpm_A));

        if (send_all(clntSock, inputs, sizeof(inputs)) <= 0) break;
        if (send_all(clntSock, theta, sizeof(theta)) <= 0) break;
        if (send_all(clntSock, &tpm_A.tau, sizeof(tpm_A.tau)) <= 0) break;

        if (recv_all(clntSock, &tau_B, sizeof(tau_B)) <= 0) break;
        printf("  Client Tau: %d\n", tau_B);

        if (tpm_A.tau == tau_B) {
            printf("  > Taus match! Updating weights...\n");
            update_weights(&tpm_A, theta);
        } else {
            printf("  > Taus mismatch. No update.\n");
            repulsive_steps++;
        }

        if (recv_all(clntSock, &tpm_B_weights, sizeof(TPM)) <= 0) break;

        memory_used = get_memory_usage_kb();

        char sync_status[10] = {0};
        if (memcmp(tpm_A.weights, tpm_B_weights.weights, sizeof(tpm_A.weights)) == 0) {
            printf("\n Synchronization Achieved! (iter: %d) \n", iteration);
            strncpy(sync_status, "SYNC_OK", sizeof(sync_status) - 1);

            if (send_all(clntSock, sync_status, sizeof(sync_status)) <= 0) break;

            show_result_graph(sync_iterations, repulsive_steps, memory_used);

            break;
        } else {
            printf("  > Weights not synced yet.\n");
            strncpy(sync_status, "CONTINUE", sizeof(sync_status) - 1);
            if (send_all(clntSock, sync_status, sizeof(sync_status)) <= 0) break;
        }
    }

    print_weights(&tpm_A, "Server Synced");

    printf("\n Chat Started (\n");

    while (1) {
        printf("Waiting for client's message...\n");
        nRcv = recv(clntSock, message, BUFSIZE - 1, 0);
        if (nRcv <= 0) {
            printf("Client closed connection.\n");
            break;
        }
        message[nRcv] = '\0';

        if (strcmp(message, "exit") == 0) {
            printf("Client requested exit.\n");
            break;
        }
        printf("Received: %s\n", message);

        printf("Send (type message, 'exit' to quit): ");
        if (fgets(message, BUFSIZE, stdin) == NULL) {
            strncpy(message, "exit", BUFSIZE);
        }
        message[strcspn(message, "\n")] = '\0';

        if (send(clntSock, message, (int)strlen(message), 0) < 0) {
            perror("send");
            break;
        }

        if (strcmp(message, "exit") == 0) {
            printf("Server exit requested.\n");
            break;
        }
    }

    close(clntSock);
    close(servSock);
    printf("Server finished.\n");
    return 0;
}