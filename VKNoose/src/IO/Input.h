#pragma once
#include <GLFW/glfw3.h>
#include "keycodes.h"

namespace Input 
{
	void Init(int windowWidth, int windowHeight);
	void Update(GLFWwindow* window);
	bool KeyPressed(unsigned int keycode);
	bool KeyDown(unsigned int keycode);
	float GetMouseOffsetX();
	float GetMouseOffsetY();
	bool LeftMouseDown();
	bool RightMouseDown();
	bool LeftMousePressed();
	bool RightMousePressed();
	bool MouseWheelUp();
	bool MouseWheelDown();
	void PreventRightMouseHold();
	inline int _mouseWheelValue = 0;
	inline int _sensitivity = 100;
}