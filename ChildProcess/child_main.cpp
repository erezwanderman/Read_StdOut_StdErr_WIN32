#include <iostream>

int main()
{
	for (int i = 0; i < 2; i++)
	{
		std::cerr << std::string(5'000'000, 'e');
		std::cout << std::string(5'000'000, 'o');
	}
	return 0;
}
