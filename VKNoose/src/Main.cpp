#include "vk_engine.h"
#include <iostream>
#include "Windows.h"

int main()
{
	//system("compile.bat");

	std::cout << "\nWELCOME TO HELL FUCK WHIT\n";
	VulkanEngine engine;

	engine.init();

	engine.run();

	engine.cleanup();

	return 0;
}