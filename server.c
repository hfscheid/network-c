#include <stdio.h>
#include "network.c"

struct MESSAGE {
    int val;
};

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("srv call must include two port numbers\n");
        return -1;
    }

    int rec_port = atoi(argv[1]);
    int send_port = atoi(argv[2]);
    struct network nw = new_network(rec_port, send_port);
    struct MESSAGE* m = network_get(&nw);
    m->val = 10;
    printf("val: %d\n", m->val);

//    network_start(&nw);
//    network_end(&nw);

    return 0;
}
