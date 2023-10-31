extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int f(int);

int main() {
    int x=10;
    PRINT(f(x));
    return 0;
}

int f(int x) {
    if (x == 0) return 0;
    return f(x-1)+1;
}

#10