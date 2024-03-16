
#ifndef TRACEROUTE_ICMP_SEND_H
#define TRACEROUTE_ICMP_SEND_H

#include <netinet/in.h>

// Funkcja odbierająca odpowiedzi ICMP i wypisująca wyniki
int receive_and_print_replies(int sock_fd, const char* expected_address_str, int id, int request_count, const double *request_send_time);
// Wysyła żądanie echo ICMP do określonego adresu
int send_echo_request(int sock_fd, struct sockaddr_in *dest_addr, uint16_t id, uint16_t seq, double *send_time);

// Analizuje adres IP z ciągu znaków i zwraca go jako strukturę sockaddr_in
struct sockaddr_in parse_str_address(const char *ip_addr);

// Wysyła n żądań echo ICMP do określonego adresu z określonym TTL
void send_n_echo_requests(int n, int ttl, int sock_fd, struct sockaddr_in *dest_addr, uint16_t id, double *send_time);

#endif//TRACEROUTE_ICMP_SEND_H
