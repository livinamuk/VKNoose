#include "Input.h"
#include "../vk_engine.h"
#include <GLFW/glfw3.h>

bool _keyPressed[372];
bool _keyDown[372];
bool _keyDownLastFrame[372];
double _mouseX = 0;
double _mouseY = 0;
double _mouseOffsetX;
double _mouseOffsetY;
bool _mouseWheelUp = false;
bool _mouseWheelDown = false;
bool _leftMouseDown = false;
bool _rightMouseDown = false;
bool _leftMousePressed = false;
bool _rightMousePressed = false;
bool _leftMouseDownLastFrame = false;
bool _rightMouseDownLastFrame = false;
bool _preventRightMouseHoldTillNextClick = false;

void Input::Update() {

    glfwPollEvents();
    GLFWwindow* window = Vulkan::GetWindow();

    // Wheel
    _mouseWheelUp = false;
    _mouseWheelDown = false;

    if (_mouseWheelValue < 0)
        _mouseWheelDown = true;
    if (_mouseWheelValue > 0)
        _mouseWheelUp = true;

    _mouseWheelValue = 0;

    // Keyboard
    for (int i = 30; i < 350; i++) {
        // down
        if (glfwGetKey(window, i) == GLFW_PRESS)
            _keyDown[i] = true;
           else
               _keyDown[i] = false;

        // press
        if (_keyDown[i] && !_keyDownLastFrame[i])
            _keyPressed[i] = true;
        else
            _keyPressed[i] = false;
        _keyDownLastFrame[i] = _keyDown[i];
    }

    // Mouse
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    _mouseOffsetX = x - _mouseX;
    _mouseOffsetY = y - _mouseY;
    _mouseX = x;
    _mouseY = y;

    // Left mouse down/pressed
    _leftMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (_leftMouseDown == GLFW_PRESS && !_leftMouseDownLastFrame)
        _leftMousePressed = true;
    else
        _leftMousePressed = false;
    _leftMouseDownLastFrame = _leftMouseDown;

    // Right mouse down/pressed
    _rightMouseDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (_rightMouseDown == GLFW_PRESS && !_rightMouseDownLastFrame)
        _rightMousePressed = true;
    else
        _rightMousePressed = false;
    _rightMouseDownLastFrame = _rightMouseDown;

    if (_rightMousePressed)
        _preventRightMouseHoldTillNextClick = false;
}

bool Input::KeyPressed(unsigned int keycode) {
	return _keyPressed[keycode];
}

bool Input::KeyDown(unsigned int keycode) {
	return _keyDown[keycode];
}

float Input::GetMouseOffsetX() {
    return (float)_mouseOffsetX;
}

float Input::GetMouseOffsetY() {
    return (float)_mouseOffsetY;
}

bool Input::LeftMouseDown() {
    return _leftMouseDown;
}

bool Input::RightMouseDown() {
    return _rightMouseDown && !_preventRightMouseHoldTillNextClick;
}

bool Input::LeftMousePressed() {
    return _leftMousePressed;
}

bool Input::RightMousePressed() {
    return _rightMousePressed;
}

bool Input::MouseWheelDown() {
    return _mouseWheelDown;
}

bool Input::MouseWheelUp() {
    return _mouseWheelUp;
}

void Input::PreventRightMouseHold() {
    _preventRightMouseHoldTillNextClick = true;
}

void Input::ForceSetStoredMousePosition(int x, int y) {
    _mouseX = x;
    _mouseY = y;
}

void Input::SetMousePos(int x, int y) {
    _mouseX = x;
    _mouseY = y;
    glfwSetCursorPos(Vulkan::GetWindow(), x, y);
}