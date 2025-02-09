// KVM
#include <iostream>
#include <string>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <cstring>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <mutex>
#include <thread>

std::mutex skt_mutex;
int skt_fd;

std::mutex tun_mutex;
int tun_fd;

struct etherip_hdr {
    u_int16_t version : 4;
    u_int16_t reserved : 12;
};

int tun_alloc(char *dev, int flags) {
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        std::cout << "Error opening /dev/net/tun" << std::endl;
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = flags;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        std::cout << "Error calling ioctl" << std::endl;
        close(fd);
        return err;
    }

    strcpy(dev, ifr.ifr_name);
    return fd;
}

int main() {
    std::cout << "Booting EtherIP" << std::endl;

    char tun_name[IFNAMSIZ];
    strcpy(tun_name, "tap0");
    tun_fd = tun_alloc(tun_name, IFF_TAP | IFF_NO_PI);

    skt_fd = socket(AF_INET, SOCK_RAW, 97);

    std::thread t1([]() {
        char buf[1500];
        while (true) {
            int len = read(tun_fd, buf, sizeof(buf));
	        if (len < 0) {
	            std::cout << "Error reading from tun" << std::endl;
	        };

            etherip_hdr hdr;
            hdr.version = 3;
	        hdr.reserved = 0;

	        char send_buf[sizeof(hdr) + len];
	        memcpy(send_buf, &hdr, sizeof(hdr));
	        memcpy(send_buf + sizeof(hdr), buf, len);

            {
                std::lock_guard<std::mutex> lock(skt_mutex);
                send(skt_fd, send_buf, len, 0);
            }
        }
    });

    while (true) {
        char buf[1500];

    }

    close(tun_fd);
    close(skt_fd);
    
    return 0;
}
