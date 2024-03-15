#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include <stdio.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


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
    if (setsockopt(sock_fd, SOL_IP, IP_TTL, &ttl, sizeof(ttl)) != 0) {
        printf("\nSetting socket options to TTL failed !");
        return -1;
    }

    return 0;
}


int send_echo_request(int sock_fd, int ttl, struct sockaddr_in *dest_addr, uint16_t id, uint16_t seq){
	struct icmphdr header;
    bzero(&header, sizeof(header));

    set_ttl(sock_fd, ttl);

    header.type = ICMP_ECHO;
    header.code = 0;
    header.checksum = 0;
    header.un.echo.id = htons(id);
    header.un.echo.sequence = htons(seq);

    header.checksum = compute_icmp_checksum((uint16_t *)&header, sizeof(header));;


    ssize_t bytes_send = sendto(sock_fd,&header, sizeof(header),0,(struct sockaddr *)dest_addr, sizeof(*dest_addr));
    if (bytes_send != sizeof(header)) {
        printf("\n?????????????");
        return -1;
    }

    return 0;
}

struct sockaddr_in string_to_addr(const char *ip_addr){
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;

    addr.sin_port = htons(3490);
    inet_aton(ip_addr, (struct in_addr *) &addr.sin_addr.s_addr);

    return addr;
}

int main(){
    int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_fd < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct sockaddr_in addr = string_to_addr("8.8.8.8");
    bind(sock_fd, (const struct sockaddr *) &addr, sizeof(addr));

    send_echo_request(sock_fd, 64, &addr, 1, 1);
    return 0;
}
