#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PACKET_SIZE 17  // Fixed size of each packet (4 + 1 + 4 + 4 + 4 = 17 bytes)

void parse_packet(const uint8_t *buffer) {
    // Parse the Symbol (4 bytes, Big Endian, ASCII)
    char symbol[5] = {0};  // 4 bytes for symbol + 1 byte for null terminator
    memcpy(symbol, buffer, 4);
    
    // Parse the Buy/Sell Indicator (1 byte, ASCII)
    char buy_sell_indicator = buffer[4];

    // Parse the Quantity (4 bytes, int32, Big Endian)
    int32_t quantity;
    memcpy(&quantity, buffer + 5, 4);
    quantity = ntohl(quantity);  // Convert to host byte order

    // Parse the Price (4 bytes, int32, Big Endian)
    int32_t price;
    memcpy(&price, buffer + 9, 4);
    price = ntohl(price);  // Convert to host byte order

    // Parse the Packet Sequence (4 bytes, int32, Big Endian)
    int32_t packet_sequence;
    memcpy(&packet_sequence, buffer + 13, 4);
    packet_sequence = ntohl(packet_sequence);  // Convert to host byte order

    // Display the parsed packet information
    printf("Parsed Packet:\n");
    printf("Symbol: %s\n", symbol);
    printf("Buy/Sell: %c\n", buy_sell_indicator);
    printf("Quantity: %d\n", quantity);
    printf("Price: %d\n", price);
    printf("Packet Sequence: %d\n", packet_sequence);

    printf("/n/n");
}

int main() {
    char *ip = (char*)"127.0.0.1";
    int port = 3000;

    int sock;
    struct sockaddr_in addr;
    socklen_t addr_size;
    uint8_t buffer[1024];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-]Socket error");
        exit(1);
    }
    printf("[+]TCP client socket created.\n");

    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[-]Connection error");
        exit(1);
    }
    printf("[+]Connected to the server.\n");

    srand(time(0));  // Seed for random number generation

    while (1) {
        int request_type;
        printf("\nEnter request type:\n1. Stream All Packets (value: 1)\n2. Resend Packet (value: 2)\nChoice: ");
        scanf("%d", &request_type);

        if (request_type == 1) {
            // Stream All Packets request (Call Type 1, value = 1)
            uint8_t request_value = 1;
            send(sock, &request_value, sizeof(request_value), 0);  // Send the value '1' to the server
            printf("[Client]: Sent request to stream all packets (value: %d)\n", request_value);

            // Receive and parse the packets
            int bytes_received;
            while ((bytes_received = recv(sock, buffer, PACKET_SIZE, 0)) > 0) {
                if (bytes_received == PACKET_SIZE) {
                    parse_packet(buffer);  // Parse each packet
                } else {
                    printf("Incomplete packet received. Bytes: %d\n", bytes_received);
                }
            }

            // Server should close connection after streaming all packets
            printf("[Client]: The server closed the connection after sending packets.\n");
            break;  // Exit the loop since the server has closed the connection

        } else if (request_type == 2) {
            // Resend Packet request (Call Type 2, value = 2)
            uint8_t request_value = 2;
            uint8_t resendSeq;  // Resend sequence number (1 byte)
            printf("Enter the sequence number of the packet to resend (1 byte): ");
            scanf("%hhu", &resendSeq);

            // Create the payload (Call Type 2 value + resendSeq)
            uint8_t payload[2] = {request_value, resendSeq};  // 2-byte payload

            send(sock, payload, sizeof(payload), 0);  // Send the request with sequence number
            printf("[Client]: Sent request to resend packet with sequence number %d\n", resendSeq);

            // Receive and parse the packet
            int bytes_received = recv(sock, buffer, PACKET_SIZE, 0);
            if (bytes_received == PACKET_SIZE) {
                parse_packet(buffer);  // Parse the single packet
            } else {
                printf("Incomplete packet received. Bytes: %d\n", bytes_received);
            }

            // Client decides to close the connection
            printf("[Client]: Closing connection after resending packet.\n");
            break;  // Exit the loop after closing the connection

        } else {
            printf("Invalid request type!\n");
            continue;
        }

        sleep(2);  // Optional: Pause before sending another request
    }

    close(sock);
    printf("Disconnected from the server.\n");

    return 0;
}
