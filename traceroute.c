// Patryk Zieliński 330261
#include "icmp_receive.h"
#include "icmp_send.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include "stdbool.h"

struct sockaddr_in parse_str_address(const char *address_str){
  struct sockaddr_in addr;
  bzero (&addr, sizeof(addr));

  addr.sin_family = AF_INET;

  if(inet_aton(address_str, (struct in_addr *) &addr.sin_addr.s_addr) == 0){
    fprintf(stderr, "invalid address: %s\n", address_str);
    exit(EXIT_FAILURE);
  }

  return addr;
}

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

  int id = getpid();
  struct sockaddr_in addr = parse_str_address(argv[1]);
  double *send_time;

  if((send_time = malloc(4 * sizeof(double))) == NULL){
    fprintf(stderr, "malloc error: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  for(int ttl = 1; ttl < 64; ttl++){
    printf("%d ", ttl);
    if(send_n_echo_requests(3, ttl, sock_fd, &addr, id+ttl, send_time) != 0) continue;
    if(receive_and_print_replies(sock_fd, argv[1], id+ttl, 3, send_time, false) == 0) break;
  }

  return 0;
}