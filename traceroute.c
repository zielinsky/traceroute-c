// Patryk Zieliński 330261
#include "icmp_send.h"
#include "icmp_receive.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>

int main(int argc, char **argv){
  if (argc != 2){
    fprintf(stderr, "correct usage: ./traceroute x.y.z.q \n");
    return EXIT_FAILURE;
  }

  int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock_fd < 0) {
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  int ttl = 1;
  int id = getpid();
  struct sockaddr_in addr = parse_str_address(argv[1]);
  double *send_time = malloc(4 * sizeof(double));

  do{
    printf("%d ", ttl);
    send_n_echo_requests(3, ttl, sock_fd, &addr, id, send_time);
  }while(ttl++ < 64 && receive_and_print_replies(sock_fd, argv[1], id, 3, send_time) == 1);

  return 0;
}