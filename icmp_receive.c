#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <stdbool.h>

#define ECHO_REPLY_TIME_EXCEEDED 11
#define IS_THE_SAME_ADDRESS(addr1, addr2) (strcmp((addr1),(addr2)) == 0)

double get_time()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (double)t.tv_sec + (double)t.tv_nsec*1e-9;
}

typedef struct replyInfo {
    double rtt;
    char address[20];
    bool received;
} replyInfo_t;

void print_results(replyInfo_t* replies, int request_count){
    bool is_unique;
    for(int i = 0; i < request_count; i++){
        if(!replies[i].received)
            continue;

        is_unique = true;
        for(int j = 0; j < i; j++){
            if(IS_THE_SAME_ADDRESS(replies[i].address, replies[j].address)){
                is_unique = false;
                break;
            }
        }
        if(is_unique) printf("%s ", replies[i].address);

    }

    for(int i = 0; i < request_count; i++){
        if (replies[i].received) printf("%.3fms ", replies[i].rtt);
        else printf("* ");

    }
    printf("\n");
}

int receive_and_print_replies(int sock_fd, const char*expected_address_str, int id, int request_count, const double *request_send_time){
    int received_reply_count = 0, return_value = 1, ready;

    replyInfo_t replies[request_count];
    bzero(replies, request_count * sizeof(replyInfo_t));

    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];

    struct pollfd ps;
    ps.fd = sock_fd;
    ps.events = POLLIN;
    ps.revents = 0;

    char sender_address_str[20];
    char initial_address_str[20];

    double start_time = get_time();
    while ((get_time() - start_time) <= 1.0 && received_reply_count < request_count) {
        ready = poll (&ps, 1, 1000);

        while(ready-- && ps.revents == POLLIN){
            ssize_t packet_len = recvfrom (sock_fd, buffer, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr*)&sender, &sender_len);
            if (packet_len < 0) {
                fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
                continue;
            }

            inet_ntop(AF_INET, &(sender.sin_addr), sender_address_str, sizeof(sender_address_str));

            struct ip *ip = (struct ip *)buffer;
            ssize_t	ip_header_len = 4 * (ssize_t)(ip->ip_hl);

            struct icmphdr *icmp_header = (struct icmphdr *)(buffer + ip_header_len);

            if (icmp_header->type == ECHO_REPLY_TIME_EXCEEDED) {
                ip = (struct ip *) (buffer + ip_header_len + 8);
                icmp_header = (struct icmphdr *) (buffer + ip_header_len + 28);

                struct in_addr dst = ip->ip_dst;
                inet_ntop(AF_INET, &dst, initial_address_str, sizeof(initial_address_str));
            }else strcpy(initial_address_str, sender_address_str);


            int ip_id = ntohs(icmp_header->un.echo.id);
            int ip_seq = ntohs(icmp_header->un.echo.sequence);

            if(ip_id == id && IS_THE_SAME_ADDRESS(initial_address_str, expected_address_str)){
                if(IS_THE_SAME_ADDRESS(sender_address_str, expected_address_str)) return_value = 0;

                strcpy(replies[ip_seq].address, sender_address_str);
                replies[ip_seq].rtt = (get_time() - request_send_time[ip_seq]) * 1000;
                replies[ip_seq].received = true;
                received_reply_count++;
            }
        }
    }

    print_results(replies, request_count);
    return return_value;
}
