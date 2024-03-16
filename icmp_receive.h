// Patryk Zieliński 330261
#ifndef TRACEROUTE_ICMP_RECEIVE_H
#define TRACEROUTE_ICMP_RECEIVE_H
#include <netinet/in.h>
#include <stdbool.h>

// Definicje stałych
#define ECHO_REPLY_TIME_EXCEEDED 11
#define IS_THE_SAME_ADDRESS(addr1, addr2) (strcmp((addr1),(addr2)) == 0)

// Struktura przechowująca informacje o odpowiedzi
typedef struct replyInfo {
    double rtt; // Round Trip Time
    char address[20]; // Adres IP jako ciąg znaków
    bool received; // Flaga oznaczająca, czy odpowiedź została otrzymana
} replyInfo_t;

// Funkcja wypisująca wyniki otrzymanych odpowiedzi
void print_results(replyInfo_t* replies, int request_count);

int receive_and_print_replies(int sock_fd, const char*expected_address_str, int id, int request_count, const double *request_send_time);

#endif//TRACEROUTE_ICMP_RECEIVE_H
