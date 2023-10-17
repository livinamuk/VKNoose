#pragma once
#include "keycodes.h"

namespace Input {

	void SetMousePos(int x, int y);
	void Update();
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
	void ForceSetStoredMousePosition(int x, int y);

	inline int _mouseWheelValue = 0;
	inline int _sensitivity = 100;
}