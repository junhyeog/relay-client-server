#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

using namespace std;

/** Constants
 * BUF_SIZE : TCP payload size
*/
const static int BUF_SIZE = 1 << 16;

/** Global variables
 * bool sender_flag : Am I sender?
 * bool receiver_flag : Am I receiver?
 * char sender_ready_message[25] : sender READY message 
 * char receiver_ready_message[27] : receiver READY message
 * char ready_mes[BUF_SIZE] : buffer for READY message
 * char user_mes[BUF_SIZE]: buffer for user message
 * char relayed_mes[BUF_SIZE] : buffer for relayed message
*/
bool sender_flag = 0;
bool receiver_flag = 0;
char sender_ready_message[25] = "READY: You are a sender.";
char receiver_ready_message[27] = "READY: You are a receiver.";
char ready_mes[BUF_SIZE];
char user_mes[BUF_SIZE];
char relayed_mes[BUF_SIZE];

void usage() {
  printf("syntax : relay-client <ip> <port>\n");
  printf("sample : relay-client 192.168.10.2 1234\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  // For error detection
  int res;

  // Check arguments - syntax : relay-client <ip> <port>
  if (argc != 3) usage();

  // Get IP address & port number from argumetns
  struct in_addr ip;
  int port = atoi(argv[2]);
  res = inet_pton(AF_INET, argv[1], &ip);
  if (res < 0) {
    perror("[-] Failed to get a IP address");
    exit(EXIT_FAILURE);
  } else if (res == 0) {
    perror("[-] Invalid IP address");
    exit(EXIT_FAILURE);
  }
  if (port < 0 || (int)(1 << 16) <= port) {
    perror("[-] Invalid port number");
    exit(EXIT_FAILURE);
  }

  // Create a socket
  // IPv4 Internet protocols and TCP
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("[-] Failed to create a socket");
    exit(EXIT_FAILURE);
  }

  // Build a socket address
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ip.s_addr;
  memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

  // Connect
  res = connect(sockfd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (res < 0) {
    perror("[-] Failed to connect");
    exit(EXIT_FAILURE);
  }
  printf("[+] Connected\n");

  // Receive Ready message
  ssize_t received_len = recv(sockfd, ready_mes, BUF_SIZE - 1, 0);
  ready_mes[received_len] = '\0';
  // Parse READY message (sender or receiver)
  sender_flag = !strncmp(sender_ready_message, ready_mes, strlen(sender_ready_message) + 1);
  receiver_flag = !strncmp(receiver_ready_message, ready_mes, strlen(receiver_ready_message) + 1);
  if (received_len <= 0 || !(sender_flag || receiver_flag)) {
    perror("[-] Failed to receive a READY message");
    close(sockfd);
    exit(EXIT_FAILURE);
  }
  // Print a READY message
  printf("%s\n", ready_mes);

  // Sender: send a user's message
  if (sender_flag) {
    // Get a user's message
    fgets(user_mes, BUF_SIZE, stdin);
    // Send a user's message
    ssize_t sent_len;
    sent_len = send(sockfd, user_mes, strlen(user_mes) - 1, 0);
    close(sockfd);
    if (sent_len <= 0) {
      perror("[-] Failed to send a user's message");
      exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
  }

  // Receiver: receive a relayed message
  if (receiver_flag) {
    ssize_t received_len = recv(sockfd, relayed_mes, BUFSIZ - 1, 0);
    relayed_mes[received_len] = '\0';
    close(sockfd);
    if (received_len <= 0) {
      perror("[-] Failed to recieve a relayed message");
      exit(EXIT_FAILURE);
    }
    printf("%s\n", relayed_mes);
    exit(EXIT_SUCCESS);
  }
}
