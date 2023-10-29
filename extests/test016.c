extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int b=1000000;
int main() {
  PRINT(b);
  if(b>5) {
    b = 5;
  } else if (b>3) {
    b = 3;
  } else {
    b = 1;
  }
  PRINT(b);
}

#5