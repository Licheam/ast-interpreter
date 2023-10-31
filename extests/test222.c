extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   char* a; 
   char* b;
   a = (char *)MALLOC(4);
   b = (char *)MALLOC(2);
   *a = 42;
   *b = 43;
   PRINT((int)*a);
   PRINT((int)*b);
   FREE(a);
   return 0;
}

#4243
