extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int b=10;
int f(int x) {
  return x;
}
int main() {
    PRINT(f(b));
}

#10