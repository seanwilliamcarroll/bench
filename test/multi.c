#include <math.h>

/* ~50% of runtime: integer accumulation */
void work_a() {
    volatile long x = 0;
    for (long i = 0; i < 150000000L; i++) x += i;
}

/* ~30% of runtime: modulo arithmetic */
void work_b() {
    volatile long x = 1;
    for (long i = 1; i < 80000000L; i++) x += i % 17;
}

/* ~20% of runtime: floating point */
void work_c() {
    volatile double x = 1.0;
    for (long i = 1; i < 30000000L; i++) x += sqrt((double)i);
}

int main() {
    work_a();
    work_b();
    work_c();
    return 0;
}
