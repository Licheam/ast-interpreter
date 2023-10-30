extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int f(int x) {
  int i=0;
  for (; i<3; i = i+1) {
  }
  return 1;
}

int main() {
  PRINT(f(2));
}

#1