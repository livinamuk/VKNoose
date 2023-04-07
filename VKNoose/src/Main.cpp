#include "vk_engine.h"
#include <iostream>
#include "Windows.h"

int main()
{
	std::cout << "\nWELCOME TO HELL FUCK WHIT\n";
	VulkanEngine engine;

	engine.init();


	//engine.load_content();

	engine.run();

	//std::cout << "Cleaning up...\n";
	engine.cleanup();
	//std::cout << "Goodbye!\n";

	return 0;
}