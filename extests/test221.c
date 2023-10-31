extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
   char* a; 
   a = (char *)MALLOC(2);
   *a = 42;
   *(a+1) = 43;
   PRINT((int)*a);
   PRINT((int)*(a+1));
   FREE(a);
   return 0;
}

#4243
