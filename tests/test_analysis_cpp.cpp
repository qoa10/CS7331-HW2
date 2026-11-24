#include <iostream>

int helper(int x);

int helper(int x) {
    return x + 1;
}

double helper(double x) {
    return x + 1.0;
}

int recursive(int n) {
    if (n <= 1)
        return 1;
    return n * recursive(n - 1);
}

int loops(int n) {
    int sum = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++)
            sum++;
    }
    while (n > 0)
        n--;
    do {
        sum++;
    } while (sum < 10);
    return sum;
}

int range_loop() {
    int arr[] = {1, 2, 3};
    int sum = 0;
    for (int x : arr)
        sum += x;
    return sum;
}

int main() {
    helper(1);
    helper(2);
    helper(1.5);
    recursive(4);
    loops(3);
    range_loop();
    return 0;
}
