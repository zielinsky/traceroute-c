#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include "icmp_receive.c"

u_int16_t compute_icmp_checksum (const void *buff, int length)
{
    const u_int16_t* ptr = buff;
    u_int32_t sum = 0;
    assert (length % 2 == 0);
    for (; length > 0; length -= 2)
        sum += *ptr++;
    sum = (sum >> 16U) + (sum & 0xffffU);
    return (u_int16_t)(~(sum + (sum >> 16U)));
}

int set_ttl(int sock_fd, int ttl){
    if (setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != 0) {
        printf("\nSetting socket options to TTL failed !");
        return -1;
    }

    return 0;
}


int send_echo_request(int sock_fd, struct sockaddr_in *dest_addr, uint16_t id, uint16_t seq, double *send_time){
	struct icmphdr header;
    bzero(&header, sizeof(header));

    header.type = ICMP_ECHO;
    header.code = 0;
    header.checksum = 0;
    header.un.echo.id = htons(id);
    header.un.echo.sequence = htons(seq);

    header.checksum = compute_icmp_checksum((uint16_t *)&header, sizeof(header));;


    send_time[seq] = get_time();
    ssize_t bytes_send = sendto(sock_fd,&header, sizeof(header),0,(struct sockaddr *)dest_addr, sizeof(*dest_addr));
    if (bytes_send != sizeof(header)) {
        printf("\n?????????????");
        return -1;
    }

    return 0;
}

struct sockaddr_in string_to_addr(const char *ip_addr){
    struct sockaddr_in addr;
    bzero (&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_aton(ip_addr, (struct in_addr *) &addr.sin_addr.s_addr);

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

    struct sockaddr_in addr = string_to_addr(argv[1]);
    bind(sock_fd, (const struct sockaddr *) &addr, sizeof(addr));


    double *send_time = malloc(4 * sizeof(double));
    int id = getpid();

    int finished = 1;
    for(int ttl = 1; ttl < 64 && (finished == 1); ttl ++){
        set_ttl(sock_fd, ttl);
        printf("%d ", ttl-1);
        send_echo_request(sock_fd, &addr, id, 0, send_time);
        send_echo_request(sock_fd, &addr, id, 1, send_time);
        send_echo_request(sock_fd, &addr, id, 2, send_time);
        send_echo_request(sock_fd, &addr, id, 3, send_time);

        finished = recv_from(sock_fd, argv[1], id, 4, send_time);
    }
    return 0;
}
