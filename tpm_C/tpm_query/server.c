#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/resource.h>

#include "tpm.h"

#define TARGET_TAU 1

int send_all(int sock, const void *buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, (char*)buf + sent, len - sent, 0);
        if (n <= 0) return -1;
        sent += n;
    }
    return 1;
}

int recv_all(int sock, void *buf, size_t len) {
    size_t recvd = 0;
    while (recvd < len) {
        ssize_t n = recv(sock, (char*)buf + recvd, len - recvd, 0);
        if (n <= 0) return -1;
        recvd += n;
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

void show_result_graph(int sync_iter, int rep_steps, long mem_kb) {
    printf("\n=========== TPM RESULT GRAPH ===========\n\n");

    print_bar_graph("Sync Iterations", sync_iter, sync_iter + 10);
    print_bar_graph("Repulsive Steps", rep_steps, sync_iter);
    print_bar_graph("Memory Usage(KB)", mem_kb, mem_kb + 300);

    printf("\n========================================\n\n");
}

int main(int argc, char** argv) {

    int servSock, clntSock;
    struct sockaddr_in servAddr, clntAddr;
    socklen_t clntLen;

    int port = (argc == 2) ? atoi(argv[1]) : 4000;

    TPM tpm_A;
    int inputs[K][N];
    int theta[K][N];
    int tau_B;

    long memory_used = 0;
    int sync_iterations = 0;
    int repulsive_steps = 0;

    srand(time(NULL));

    init_tpm(&tpm_A);
    print_weights(&tpm_A, "Server Initial");

    servSock = socket(AF_INET, SOCK_STREAM, 0);

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(port);
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(servSock, (struct sockaddr*)&servAddr, sizeof(servAddr));
    listen(servSock, 5);

    printf("Waiting for client...\n");
    clntLen = sizeof(clntAddr);
    clntSock = accept(servSock, (struct sockaddr*)&clntAddr, &clntLen);

    printf("Client connected.\n");
    printf("\nQuery-based Synchronization Start\n");

    int H = 2;
    int iteration = 0;

    while (1) {
        iteration++;
        sync_iterations++;

        printf("\n[Iteration %d]\n", iteration);
        generate_query_inputs(&tpm_A, inputs, H);
        generate_inputs(theta);
        calculate_tau(&tpm_A, inputs);

        printf("  Server Tau: %d (checksum: %lld)\n",
               tpm_A.tau, get_weights_checksum(&tpm_A));

        if (send_all(clntSock, inputs, sizeof(inputs)) <= 0) break;
        if (send_all(clntSock, theta, sizeof(theta)) <= 0) break;
        if (send_all(clntSock, &tpm_A.tau, sizeof(tpm_A.tau)) <= 0) break;
        if (recv_all(clntSock, &tau_B, sizeof(tau_B)) <= 0) break;
        printf("  Client Tau: %d\n", tau_B);

        if (tau_B == tpm_A.tau) {
            printf("  > Taus match! Updating weights...\n");
            update_weights(&tpm_A, theta);
        } else {
            printf("  > Taus mismatch. No update.\n");
            repulsive_steps++;
        }

        TPM tpm_B;
        if (recv_all(clntSock, &tpm_B, sizeof(TPM)) <= 0) break;

        memory_used = get_memory_usage_kb();

        char status[10];
        if (memcmp(tpm_A.weights, tpm_B.weights, sizeof(tpm_A.weights)) == 0) {

            printf("\nSynchronization Success!\n");
            strcpy(status, "SYNC_OK");
            send_all(clntSock, status, sizeof(status));

            show_result_graph(sync_iterations, repulsive_steps, memory_used);

            break;
        } else {
            printf("  > Not Synced Yet.\n");
            strcpy(status, "CONTINUE");
            send_all(clntSock, status, sizeof(status));
        }
    }

    print_weights(&tpm_A, "Server Final");

    close(clntSock);
    close(servSock);
    return 0;
}
