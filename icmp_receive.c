#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>

double get_time()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec + t.tv_nsec*1e-9;
}

void print_as_bytes (unsigned char* buff, ssize_t length)
{
    for (ssize_t i = 0; i < length; i++, buff++)
        printf ("%.2x ", *buff);
}

int recv_from(int sock_fd){
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];

    struct pollfd ps;
    ps.fd = sock_fd;
    ps.events = POLLIN;
    ps.revents = 0;
    int ready;

    double t = get_time();;
    while ((get_time() - t) <= 1.0) {
        printf("time taken %.6lf\n", get_time() - t);
        ready = poll (&ps, 1, 1000);

        while(ready-- && ps.revents == POLLIN){
            ssize_t packet_len = recvfrom (sock_fd, buffer, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr*)&sender, &sender_len);
            if (packet_len < 0) {
                fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
                return EXIT_FAILURE;
            }

            char sender_ip_str[20];
            inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, sizeof(sender_ip_str));
            printf ("Received IP packet with ICMP content from: %s\n", sender_ip_str);

            struct ip *ip = (struct ip *)buffer;
            ssize_t	ip_header_len = 4 * (ssize_t)(ip->ip_hl);
            struct icmphdr *icmp_header = (struct icmphdr *)(buffer + ip_header_len);
            if (icmp_header->type == 11){
                icmp_header = (struct icmphdr *)(buffer + ip_header_len + 28);
            }

            struct in_addr src = ip->ip_src;
            struct in_addr dst = ip->ip_dst;

            printf("%s\n",(char*)inet_ntoa(dst));
            printf("%s\n",(char*)inet_ntoa(src));
            printf("%d\n", ntohs(icmp_header->un.echo.id));
            printf("%d\n", ntohs(icmp_header->un.echo.sequence));

            printf("IP header: ");
            print_as_bytes(buffer, ip_header_len);
            printf("\n");

            printf("IP data:   ");
            print_as_bytes(buffer + ip_header_len, packet_len - ip_header_len);
            printf("\n\n");
        }
    }

}

int main()
{
	int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock_fd < 0) {
		fprintf(stderr, "socket error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

    recv_from(sock_fd);

}
