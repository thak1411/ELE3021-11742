#include "types.h"
#include "stat.h"
#include "user.h"

#define RN_STUDENT_ID 2020021949

void level_cnt_init(int* level_cnt) {
    level_cnt[0] = level_cnt[1] = level_cnt[2] = level_cnt[3] = level_cnt[4] = 0;
}

void add_level_cnt(int* level_cnt) {
    int lev;

    lev = getlev();
    if (lev == 99) ++level_cnt[4];
    else ++level_cnt[lev];
}

void print_level_cnt(int* level_cnt, const char* pname) {
    printf(1, " [ process %d (%s) ]\n", getpid(), pname);
    printf(1, "\t- L0:\t\t%d\n", level_cnt[0]);
    printf(1, "\t- L1:\t\t%d\n", level_cnt[1]);
    printf(1, "\t- L2:\t\t%d\n", level_cnt[2]);
    printf(1, "\t- L3:\t\t%d\n", level_cnt[3]);
    printf(1, "\t- MONOPOLIZED:\t%d\n\n", level_cnt[4]);
}

void rn_test1() {
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test1 [L0 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 1000; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 500; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 100; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    w_list[0] = p3;
    w_list[1] = p2;
    w_list[2] = p1;
    w_cnt = 0;
    for (; w_cnt < 3; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test1 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test1 finished successfully\n\n\n");
}

void rn_test2() {
    int p, p1, p2, p3, p4, w_cnt, w_list[4], level_cnt[5];

    printf(1, "rn_test2 [L1~L2 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 300; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 100; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    p4 = fork();
    if (p4 == 0) {
        for (int cnt = 50; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p4");
        exit();
    }

    if (p4 % 2 == 1) {
        w_list[0] = p4;
        w_list[1] = p2;
        w_list[2] = p3;
        w_list[3] = p1;
    } else {
        w_list[0] = p3;
        w_list[1] = p1;
        w_list[2] = p4;
        w_list[3] = p2;
    }
    w_cnt = 0;
    for (; w_cnt < 4; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test2 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test2 finished successfully\n\n\n");
}

void rn_test3() {
    int p, p1, p2, p3, p4, w_cnt, w_list[4], level_cnt[5];

    printf(1, "rn_test3 [L3 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 1) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    p4 = fork();
    if (p4 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 7) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p4");
        exit();
    }

    w_list[0] = p2;
    w_list[1] = p4;
    w_list[2] = p3;
    w_list[3] = p1;
    w_cnt = 0;
    for (; w_cnt < 4; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test3 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test3 finished successfully\n\n\n");
}

void rn_test4() {
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test4 [MoQ Only - wait 5sec]\n");

    level_cnt_init(level_cnt);
    
    p1 = fork();
    if (p1 == 0) {
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 100; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    if (setmonopoly(p1, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p1]\n");
    if (setmonopoly(p2, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p2]\n");
    if (setmonopoly(p3, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p3]\n");

    rn_sleep(5000);
    printf(1, "monopolize!\n");
    monopolize();

    w_list[0] = p1;
    w_list[1] = p2;
    w_list[2] = p3;
    w_cnt = 0;
    for (; w_cnt < 3; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test4 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test4 finished successfully\n\n\n");
}

void rn_test5() {
    int p, p1, p2, p3, p4, p5, p6, w_cnt, w_list[6], level_cnt[5];

    printf(1, "rn_test5 [L0, L1, L2, L3]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    p4 = fork();
    if (p4 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p4");
        exit();
    }

    p5 = fork();
    if (p5 == 0) {
        for (int cnt = 1000; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p5");
        exit();
    }

    p6 = fork();
    if (p6 == 0) {
        for (int cnt = 500; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p6");
        exit();
    }

    w_list[0] = p6;
    w_list[1] = p5;
    if (p3 % 2 == 1) {
        w_list[2] = p3;
        w_list[3] = p4;
    } else {
        w_list[2] = p4;
        w_list[3] = p3;
    }
    w_list[4] = p1;
    w_list[5] = p2;
    w_cnt = 0;
    for (; w_cnt < 6; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test5 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test5 finished successfully\n\n\n");
}

void rn_test6() {
    int p, p1, p2, p3, p4, p5, p6, p7, p8, w_cnt, w_list[8], level_cnt[5];

    printf(1, "rn_test6 [L0, L1, L2, L3, MoQ]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    p4 = fork();
    if (p4 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p4");
        exit();
    }

    p5 = fork();
    if (p5 == 0) {
        for (int cnt = 1000; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p5");
        exit();
    }

    p6 = fork();
    if (p6 == 0) {
        for (int cnt = 500; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p6");
        exit();
    }

    p7 = fork();
    if (p7 == 0) {
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p7");
        exit();
    }

    p8 = fork();
    if (p8 == 0) {
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p8");
        exit();
    }

    if (setmonopoly(p7, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p7]\n");
    if (setmonopoly(p8, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p8]\n");

    w_list[0] = p6;
    w_list[1] = p5;
    if (p3 % 2 == 1) {
        w_list[2] = p3;
        w_list[5] = p4;
    } else {
        w_list[2] = p4;
        w_list[5] = p3;
    }
    w_list[3] = p7;
    w_list[4] = p8;
    w_list[6] = p1;
    w_list[7] = p2;
    w_cnt = 0;
    for (; w_cnt < 8; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test6 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
        if (w_cnt == 3) {
            printf(1, "monopolize!\n");
            monopolize();
        }
    }

    printf(1, "rn_test6 finished successfully\n\n\n");
}

void rn_test7() {
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test7 [MoQ Only - Sleep Test]\n");

    level_cnt_init(level_cnt);
    
    p1 = fork();
    if (p1 == 0) {
        sleep(111);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 100; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        sleep(111);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        sleep(111);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    if (setmonopoly(p1, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p1]\n");
    if (setmonopoly(p2, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p2]\n");
    if (setmonopoly(p3, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p3]\n");

    rn_sleep(5000);
    printf(1, "monopolize!\n");
    monopolize();

    w_list[0] = p1;
    w_list[1] = p2;
    w_list[2] = p3;
    w_cnt = 0;
    for (; w_cnt < 3; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test7 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test7 finished successfully\n\n\n");
}

void rn_test8() {
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test8 [L0 Only - All Sleep]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 200; cnt--; ) {
            sleep(2);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 100; cnt--; ) {
            sleep(2);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 50; cnt--; ) {
            sleep(2);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    w_list[0] = p3;
    w_list[1] = p2;
    w_list[2] = p1;
    w_cnt = 0;
    for (; w_cnt < 3; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test8 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test8 finished successfully\n\n\n");
}

void rn_test9() {
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test9 [L0 Only - Sleep & Kill]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 200; cnt--; ) {
            sleep(2);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 100; cnt--; ) {
            sleep(2);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 50; cnt--; ) {
            sleep(2);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    w_list[0] = p3;
    w_list[1] = p1;
    w_list[2] = p2;
    w_cnt = 0;
    for (; w_cnt < 3; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test9 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
        if (w_cnt == 1) {
            printf(1, "kill process %d\n", p1);
            kill(p1);
        }
    }

    printf(1, "rn_test9 finished successfully\n\n\n");
}

void rn_test10() {
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test10 [MoQ Only - Kill Self]\n");

    level_cnt_init(level_cnt);
    
    p1 = fork();
    if (p1 == 0) {
        sleep(111);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 100; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p1");
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        sleep(111);
        printf(1, "monopolized process %d\n", getpid());
        kill(getpid());
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p2");
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        sleep(111);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt, "p3");
        exit();
    }

    if (setmonopoly(p1, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p1]\n");
    if (setmonopoly(p2, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p2]\n");
    if (setmonopoly(p3, RN_STUDENT_ID) < 0) printf(1, "setmonopoly failed [p3]\n");

    rn_sleep(5000);
    printf(1, "monopolize!\n");
    monopolize();

    w_list[0] = p1;
    w_list[1] = p2;
    w_list[2] = p3;
    w_cnt = 0;
    for (; w_cnt < 3; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test10 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test10 finished successfully\n\n\n");
}

int main(int argc, char** argv) {
    int i;

    rn_test1();
    rn_test2();
    rn_test3();
    rn_test4();
    for (i = 0; i < 400; ++i) {
        rn_sleep(3);
        yield();
    }
    rn_test5();
    rn_test6();
    rn_test7();
    for (i = 0; i < 400; ++i) {
        rn_sleep(3);
        yield();
    }
    rn_test8();
    rn_test9();
    rn_test10();

    exit();
}
