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
 * int sender_sockfd : sender connection socket
 * int receiver_sockfd : receiver connection socket 
 * char sender_ready_message[25] : sender READY message 
 * char receiver_ready_message[27] : receiver READY message
*/
int sender_sockfd = -1;
int receiver_sockfd = -1;
char sender_ready_message[25] = "READY: You are a sender.";
char receiver_ready_message[27] = "READY: You are a receiver.";

void usage() {
  printf("syntax : relay-server <port>\n");
  printf("sample : relay-server 1234\n");
  exit(EXIT_FAILURE);
}

int send_mes(int sockfd, char* message) {
  ssize_t sent_len;
  sent_len = send(sockfd, message, strlen(message), 0);
  if (sent_len <= 0) {
    perror("[-] Failed to a send message");
    return -1;
  }
  return sent_len;
}

int relay_mes(int sender_sockfd, int receiver_sockfd) {
  // Receive a user's message from sender
  char buf[BUF_SIZE];
  memset(buf, 0, sizeof(buf));
  ssize_t received_len = recv(sender_sockfd, buf, BUF_SIZE - 1, 0);
  buf[received_len] = '\0';
  // Send a user's message to receiver
  int res = send_mes(receiver_sockfd, buf);
  if (res < 0) {
    perror("[-] Failed to a send a user's message to receiver");
    return -1;
  }
  return res;
}

int main(int argc, char* argv[]) {
  // For error detection
  int res;

  // Check arguments - syntax : relay-server <port>
  if (argc != 2) usage();

  // Get a port number from argumetn
  int port = atoi(argv[1]);
  if (port < 0 || (int)(1 << 16) <= port) {
    perror("[-] Invalid port number");
    exit(EXIT_FAILURE);
  }

  // Create a welcome socket
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
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

  // Bind
  res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
  if (res < 0) {
    perror("[-] Failed to bind");
    exit(EXIT_FAILURE);
  }

  // Listen
  res = listen(sockfd, SOMAXCONN);
  if (res < 0) {
    perror("[-] Failed to listen");
    exit(EXIT_FAILURE);
  }

  while (true) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if (client_sockfd < 0) {
      perror("[-] Failed to accept");
      break;
    }
    printf("[+] Client(%d) connected\n", client_sockfd);

    // There is no sender connection
    if (sender_sockfd == -1) {
      sender_sockfd = client_sockfd;
    }
    // There is no receiver connection
    else if (receiver_sockfd == -1) {
      receiver_sockfd = client_sockfd;
      res = send_mes(sender_sockfd, sender_ready_message);
      if (res < 0) {
        perror("[-] Failed to send a sender READY message");
        break;
      }
      res = send_mes(receiver_sockfd, receiver_ready_message);
      if (res < 0) {
        perror("[-] Failed to send a receiver READY message");
        break;
      }
      res = relay_mes(sender_sockfd, receiver_sockfd);
      if (res < 0) {
        perror("[-] Failed to relay a user's message");
        break;
      }
      printf("[+] a message relayed\n");
      break;
    }
  }
  // Close welcome socket
  close(sockfd);
  // Close sender connection socket
  if (sender_sockfd != -1) close(sender_sockfd);
  // Close receiver connection socket
  if (receiver_sockfd != -1) close(receiver_sockfd);
  return 0;
}
