volatile int v = 7;
int counter = 0;

int get_value(void) {
    counter++;
    return -7;
}

int test_volatile(void) { return v / 2; }
int test_call(void) { return get_value() / 2; }
int test_increment(int x) { return x++ / 2; }
int test_unsigned(unsigned x) { return x++ / 2; }
int test_constant(void) { return 16 / 4; }
