extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int b=5;
int f(int x) {
  if(x>0)
    return f(x-1)+1;
  else
    return 0;
}
int main() {
  PRINT(b);
  PRINT(f(b));
}

#55