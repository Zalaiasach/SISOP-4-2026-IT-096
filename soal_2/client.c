#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char message[1024];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(9000);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) return -1;
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) return -1;

    while(1) {
        printf("db > ");
        if (fgets(message, 1024, stdin) == NULL) break;
        message[strcspn(message, "\n")] = 0;
        
        if (strcmp(message, "exit") == 0) break;

        send(sock, message, strlen(message), 0);
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            printf("%s\n", buffer);
        }
        memset(buffer, 0, 1024);
    }
    close(sock);
    return 0;
}