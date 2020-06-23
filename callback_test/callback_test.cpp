#include <stdio.h>

typedef int (*callback) (void);

//int printBA();




class A{

public:
	callback print;

};

class B{
public:
	B()
	{
		a = new A();
		a->print = printBA;	
	}
	B* getThisPtr()
	{
		return this;
	}

public:
	callback printB;
	A* a;

};



int printBA()
{
	B* test = getThisPtr();
      test.printB();
     return 8;
}


int print_test()
{
	printf("I am ok!\r\n");
	return 8;
}

int main()
{

	A test;
	test.print = print_test;
	test.print();
	printf("---------------\r\n");
	B testB;
	testB.printB = print_test;
	testB.a->print();
return 0;

}

