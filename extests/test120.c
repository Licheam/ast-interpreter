extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
  int a[4];
  a[1]=1;
  PRINT(a[1]);
}

#1