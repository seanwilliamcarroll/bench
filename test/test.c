void inner() {
  volatile long x = 0;
  for (long i = 0; i < 100000000L; i++)
    x += i;
}

void outer() { inner(); }

int main() {
  outer();
  return 0;
}
