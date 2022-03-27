#include <ctype.h>

int get_digit(int n)
{
  unsigned short d = 0;
  while(n != 0){
    n = n / 10;
    d++;
  }
  if(d == 0){
    d = 1;
  }
  return d;
}

int strcicmp(char const *a, char const *b)
{
  for (;; a++, b++) {
    int d = tolower((unsigned char)*a) - tolower((unsigned char)*b);
    if (d != 0 || !*a)
      return d;
  }
}

int getstrlength(char *s)
{
  int count = 0;

  while (s[count] != '\0')
    count++;

  return count;
}

