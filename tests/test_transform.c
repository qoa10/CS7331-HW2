unsigned multiply(unsigned x) { return x * 4; }
unsigned divide(unsigned y) { return y / 2; }
unsigned reverse(unsigned x) { return 8 * x; }
int signed_multiply(int x) { return x * 4; }
int signed_reverse(int x) { return 8 * x; }
long mixed(int x) { return x * 4L; }
int promoted(short x) { return x * 4; }
int negative(int x) { return x / 2; }
int nonpower(int x) { return x * 3; }
unsigned expression(unsigned x) { return (x + 1) * 4; }
unsigned nested(unsigned x) { return x * 4 * 2; }
unsigned compound(unsigned x) { x *= 4; return x; }
int main(void) {
    return !(multiply(7) == 28 && divide(7) == 3 && reverse(3) == 24 &&
             signed_multiply(7) == 28 && signed_reverse(3) == 24 &&
             mixed(7) == 28L && promoted(7) == 28 && negative(-7) == -3 &&
             nonpower(3) == 9 && expression(3) == 16 && nested(3) == 24 &&
             compound(3) == 12);
}
