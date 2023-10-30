extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   int* a;
   a = (int*)MALLOC(sizeof(int)*8);
   *a = 10;
   *(1+a) = 20;
   PRINT(*a);
   PRINT(*(a+1));
   FREE(a);
}

#1020