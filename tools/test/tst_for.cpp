// gcc -o tst_for tst_for.cpp tst_for.o -lstdc++ -lgfortran

#include <iostream>
#include <cstring>

extern "C" void add_(double *a, double *b, double *res) __attribute__((stdcall));
extern "C" double sub_(double *a, double *b) __attribute__((stdcall));
extern "C" void pr_(char* str) __attribute__((stdcall));

int main()
{
	char pc[] = "1234567890";
	pr_(pc);

	double a, b, res, res2;
	std::cout << "a = "; std::cin >> a;
	std::cout << "b = "; std::cin >> b;

	add_(&a, &b, &res);
	res2 = sub_(&a, &b);

	std::cout << "add: " << res << std::endl;
	std::cout << "sub: " << res2 << std::endl;
	return 0;
}
