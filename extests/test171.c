extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);


int main() {
   int* a;
   int b;
   b = 10;
   
   a = (int*)MALLOC(4);
   *a = b;
   PRINT(*a);
}

#10