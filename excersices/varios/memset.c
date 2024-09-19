#include<stdio.h>
#include<string.h>

struct suCucha {
    int id;
    float plata;
};

int main() {
    int a = 5;
    char b = 'x';
    double c = 6.12341324;
    struct suCucha d = {255, 12.3432};

    printf("int %d\nchar %c\ndouble %f\nstruct int %d\nstruct float %f\n", a, b, c, d.id, d.plata);

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    memset(&c, 0, sizeof c);
    memset(&d, 0, sizeof d);
   
    puts("");
    printf("int %d\nchar %c\ndouble %f\nstruct int %d\nstruct float %f\n", a, b, c, d.id, d.plata);
    printf("tamano de la struct %ld\ntamano de int %ld\n", sizeof d);
}
