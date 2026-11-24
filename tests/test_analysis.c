int helper(int x);
int helper(int x) {
    return x + 1;
}
int work(int n, int total) {
    for (int i = 0; i < n; i++) {
        total += helper(i);
    }
    while (n > 0) {
        n--;
    }
    return total;
}
int main(void) {
    int x = work(3, 0);
    return helper(x) == 7 ? 0 : 1;
}
