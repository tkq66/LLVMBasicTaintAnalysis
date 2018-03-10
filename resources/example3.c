int main () {
    int b, c, sink, source, N = 5;
    int i = 0;
    while (i < N) {
        if (i % 2 == 0)
            b = source;
        else
            c = b;
        i++;
    }
    sink = c;
}
