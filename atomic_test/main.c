#include <stdio.h>

extern int __atomic_dec(volatile int *ptr);
extern int __atomic_inc(volatile int *ptr);



int main()
{
	int a = 0;
	__atomic_inc(&a);
	printf("a=%d\r\n",a);
	__atomic_inc(&a);
	printf("a=%d\r\n",a);
	__atomic_inc(&a);
	printf("a=%d\r\n",a);
	__atomic_inc(&a);
	printf("a=%d\r\n",a);
	__atomic_dec(&a);
	printf("a=%d\r\n",a);
	__atomic_dec(&a);
	printf("a=%d\r\n",a);
	__atomic_dec(&a);
	printf("a=%d\r\n",a);
}
