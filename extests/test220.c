extern int GET();
extern void * MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

char b='a';
int main() {
    char a=b;
    b=a;
    PRINT(b);
}


#97