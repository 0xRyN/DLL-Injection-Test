// TestConsoleApp1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#pragma optimize( "", off)
int add(int a, int b)
{
	return a + b;
}

class Person {
public:
	virtual void work() {
		std::cout << "I have no work" << std::endl;
	}
};

class ComputerScientist : public Person {
public:
	void work() {
		std::cout << "I have a job, but for how long..." << std::endl;
	}
};

int main()
{
	//int a, b;
	//std::cout << "Enter two numbers: ";
	//std::cin >> a >> b;
	//int sum = 99;
	//sum = add(a, b);
	//std::cout << "Sum of " << a << " and " << b << " is " << sum << std::endl;
	Person* p;
	ComputerScientist sc; // "I have a job, but for how long..."
	int a;
	std::cin >> a;
	p = &sc;
	p->work();
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
