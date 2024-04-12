#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char** argv) {
    int p = fork();

    if (p < 0) {
        printf(1, "fork failed\n");
        exit();
    } else if (p == 0) { // child
        for (int i = 10; i--; ) {
            yield();
            printf(1, "child\n");
            sleep(100);
        }
    } else { // parent
        for (int i = 10; i--; ) {
            yield();
            printf(1, "parent\n");
            sleep(100);
        }
        wait();
    }
    exit();
}
