#include <arpa/inet.h>
#include <semaphore.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <thread>

using namespace std;

/*
get_port
send_ready(sender, receiver)
relay(sender, receiver)
terminate(sender, receiver)

*/
sem_t m;
int sender_sockfd = -1;
int receiver_sockfd = -1;
bool wait_flag = 1;
char buffer[BUFSIZ];
char* sender_ready_message = "READY: You are a sender.";
char* receiver_ready_message = "READY: You are a receiver.";

void usage() {
  printf("syntax : echo-server <port> [-e[-b]]\n");
  printf("\t-e : echo\n");
  printf("\t-b : broadcast\n");
  printf("sample : echo-server 1234 -e -b\n");
  return;
}

void* sender_thread(void* sockfd, void* buffer, void* wait_flag) {
  int sender_sockfd = *(int*)sockfd;
  char* buf = (char*)buffer;
  while (*(bool*)wait_flag) continue;
  ssize_t response_len, sent_len;
  sent_len = send(sender_sockfd, sender_ready_message, strlen(sender_ready_message), 0);
  if (sent_len == 0 || sent_len == -1) {
    perror("Failed to send Sender READY Message");
  }
  while (true) {
    //? Receive message
    response_len = recv(sender_sockfd, buf, BUFSIZ - 1, 0);
    if (response_len == 0 || response_len == -1) {
      perror("Failed to receive");
      break;
    }
    buf[response_len] = '\0';
  }
  //* Remove client
  close(sender_sockfd);
  printf("Sender(%d) disconnected\n", sender_sockfd);
  return nullptr;
}

void* receiver_thread(void* sockfd, void* buffer) {
  ssize_t sent_len;
  sent_len = send(receiver_sockfd, receiver_ready_message, strlen(receiver_ready_message), 0);
  if (sent_len == 0 || sent_len == -1) {
    perror("Failed to send Receiver READY Message");
  }
  int receiver_sockfd = *(int*)sockfd;
  char* buf = (char*)buffer;
  sent_len = send(receiver_sockfd, buf, strlen(buf), 0);
  if (sent_len == 0 || sent_len == -1) {
    perror("Failed to send");
  }
  //* Remove client
  close(receiver_sockfd);
  printf("Receiver(%d) disconnected\n", receiver_sockfd);
  return nullptr;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    usage();
    return -1;
  }

  int res;  //* for error check

  //* Get port number
  int port = atoi(argv[1]);

  //* Create a socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("Failed to create a socket");
    return -1;
  }

  //* Set socket options
  //   int optval = 1;
  //   res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
  //   if (res == -1) {
  //     perror("Failed to set socket options");
  //     return -1;
  //   }

  //* Build a socket address
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

  //* Bind
  res = bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(struct sockaddr));
  if (res == -1) {
    perror("Failed to bind");
    return -1;
  }

  //* Listen
  res = listen(sockfd, SOMAXCONN);
  if (res == -1) {
    perror("Failed to listen");
    return -1;
  }

  while (true) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sockfd = accept(sockfd, reinterpret_cast<struct sockaddr*>(&client_addr), &client_len);
    if (client_sockfd == -1) {
      perror("Failed to accept");
      break;
    }
    printf("[+] Client connected\n");

    //? Sender setting
    if (sender_sockfd == -1) {
      sender_sockfd = client_sockfd;
      thread sender_t(sender_thread, reinterpret_cast<void*>(&sender_sockfd), reinterpret_cast<void*>(buffer), reinterpret_cast<void*>(&wait_flag));
      sender_t.detach();
    }

    //? Receiver setting
    if (receiver_sockfd == -1) {
      receiver_sockfd = client_sockfd;
      wait_flag = 0;
      thread receiver_t(receiver_thread, reinterpret_cast<void*>(&receiver_sockfd), reinterpret_cast<void*>(buffer));
      receiver_t.detach();
    }
  }

  close(sockfd);
  sem_destroy(&m);
  return 0;
}