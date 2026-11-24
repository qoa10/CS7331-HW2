#include <stdio.h>
#include <limits.h>
#include <assert.h>

int mul(int x) { return x * 4; }
int div2(int x) { return x / 2; }
int div4(int x) { return x / 4; }
int div8(int x) { return x / 8; }

unsigned umul(unsigned x) { return x * 4; }
unsigned udiv(unsigned x) { return x / 4; }

long mixed(int x) { return x * 4L; }
int promoted(short x) { return x * 4; }

int side_effect(int x) {
    int y = x++ / 2;
    return y + x;
}

int main(void) {
    assert(mul(7) == 28);
    assert(mul(-7) == -28);
    assert(mul(0) == 0);
    assert(mul(INT_MIN / 4) == INT_MIN);

    assert(div2(7) == 3);
    assert(div2(-7) == -3);
    assert(div2(-1) == 0);
    assert(div2(INT_MIN) == INT_MIN / 2);
    assert(div2(INT_MAX) == INT_MAX / 2);

    assert(div4(-7) == -1);
    assert(div4(-8) == -2);
    assert(div4(7) == 1);

    assert(div8(-15) == -1);
    assert(div8(15) == 1);

    assert(umul(UINT_MAX) == UINT_MAX - 3);
    assert(udiv(UINT_MAX) == UINT_MAX / 4);

    assert(mixed(-7) == -28L);
    assert(promoted(-7) == -28);

    assert(side_effect(7) == 11);
    assert(side_effect(-7) == -9);

    puts("All tests passed.");
    return 0;
}
