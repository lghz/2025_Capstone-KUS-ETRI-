#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "tpm.h"

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

int main() {

    char ip[64];
    int port;

    printf("Server IP: ");
    scanf("%63s", ip);
    printf("Port: ");
    scanf("%d", &port);

    TPM tpm_B;
    int inputs[K][N];
    int theta[K][N];
    int tau_A;
    char status[10];

    init_tpm(&tpm_B);
    print_weights(&tpm_B, "Client Initial");

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &servAddr.sin_addr);

    connect(sock, (struct sockaddr*)&servAddr, sizeof(servAddr));

    printf("\nQuery-based Synchronization Start\n");

    int iteration = 0;

    while (1) {
        iteration++;
        printf("\n[Iteration %d]\n", iteration);

        if (recv_all(sock, inputs, sizeof(inputs)) <= 0) break;
        if (recv_all(sock, theta, sizeof(theta)) <= 0) break;
        if (recv_all(sock, &tau_A, sizeof(tau_A)) <= 0) break;

        printf("  Received Tau: %d\n", tau_A);

        calculate_tau(&tpm_B, inputs);
        printf("  Client Tau: %d (checksum: %lld)\n",
            tpm_B.tau, get_weights_checksum(&tpm_B));

        if (send_all(sock, &tpm_B.tau, sizeof(tpm_B.tau)) <= 0) break;

        if (tpm_B.tau == tau_A) {
            printf("  > Match! Updating weights...\n");
            update_weights(&tpm_B, theta);
        } else {
            printf("  > Mismatch. No update.\n");
        }

        if (send_all(sock, &tpm_B, sizeof(TPM)) <= 0) break;

        if (recv_all(sock, status, sizeof(status)) <= 0) break;

        if (!strcmp(status, "SYNC_OK")) {
            printf("\nSynchronization Achieved!\n");
            break;
        } else {
            printf("  > Not Synced Yet.\n");
        }
    }

    print_weights(&tpm_B, "Client Final");

    close(sock);
    return 0;
}
