extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int b=10;
int f() {
  return b;
}
int main() {
    PRINT(f());
}

#10