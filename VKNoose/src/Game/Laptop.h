#pragma once

#include <Noose/Constants.h>

#include <vector>
#include <string>

#define MINIMUM_WINDOW_WIDTH 100
#define MINIMUM_WINDOW_HEIGHT 50
#define RESIZE_GRACE_DISTANCE 2

enum class ButtonState { DEFAULT, CLOSE_HOVER, CLOSE_CLICK, MIN_HOVER, MIN_CLICK, MAX_HOVER, MAX_CLICK };
enum class MaximizeState { WINDOWED, FULLSCREEN };
enum ResizeAvalibility { NONE = 0, NORTH, WEST, EAST, SOUTH, NORTH_WEST, NORTH_EAST, SOUTH_WEST, SOUTH_EAST };

struct File {
	float xPos = 0;
	float yPos = 0;
	std::string textureName = "";
	std::string parentWindow = "";
	bool dragging = false;
	bool selected = false;
	int dragOffsetX = 0;
	int dragOffsetY = 0;
	float doubleClickCountDown = 0;
	float xPosBeforeDrag = 0;
	float yPosBeforeDrag = 0;
	bool hasBeenDraggedAtLeastOnePixel = false;
	bool hasBeenUpdatedThisFrame = false;
	// mess, fix
	//float localXPos = 0;
	//float localYPos = 0;

	int GetWidth();
	int GetHeight();
	bool CursorIsOverlapping(int cursorX, int cursorY);
	void Update(float deltaTime, int cursorX, int cursorY);
	float GetGlobalXPosition();
	float GetGlobalYPosition();
	void* GetParent();
	//bool IsHovered(int cursorX, int cursorY);
	//void RepositionWithinParentWindowBounds();
};
struct Window {
	int windowedXPos = 0;
	int windowedYPos = 0;
	int windowedWidth = 0;
	float windowedHeight = 0;
	std::string name = "";
	std::string correspondingFile = "";
	MaximizeState maximizeState = MaximizeState::WINDOWED;
	bool allowFileDrop = true;
	std::string mainbarTextTexture = "";
	std::string backgroundTexture = "";
	bool minimizing = false;
	int currentWidth = -1;
	int currentHeight = -1;
	int currentXPos = -1;
	int currentYPos = -1;
	bool dragging = false;
	bool resizing = false;
	int dragOffsetX = 0;
	int dragOffsetY = 0;
	ButtonState buttonState = ButtonState::DEFAULT;
	bool visible = false;
	bool isFrontMost = false;
	std::vector<File> files;
	float flashTimer = 0;
	float mainbarDoubleClickCountDown = 0;
	ResizeAvalibility resizeAvalibility;
	int xPosBeforeResize = 0;
	int yPosBeforeResize = 0;
	int widthBeforeResize = 0;
	int heightBeforeResize = 0;
	int leftEdgeBeforeResize = 0;
	int rightEdgeBeforeResize = 0;
	int topEdgeBeforeResize = 0;
	int bottomEdgeBeforeResize = 0;
	int resizeDragOffsetX = 0;
	int resizeDragOffsetY = 0;
	//float xPosWhileResizing = 0;
	//float yPosWhileResizing = 0;


	void UpdateSize(float deltaTime);
	float GetCurrentWidth();
	int GetCurrentHeight();
	int GetTopEdge();
	int GetBottomEdge();
	int GetLeftEdge();
	int GetRightEdge();
	int GetMenuHeight();
	int GetRightEdgeOfMenuButtons();
	int GetCenterYPos();
	std::string GetButtonTextureName();
	void Close();
	void Minimize();
	void Maximize();
	void Open(int spawnX, int spawnY);
	bool CursorIsOverWindow(int cursorX, int cursorY);
	bool CursorIsOverWindowIncludingResizeGraceDistance(int cursorX, int cursorY);
	bool CursorIsOverWindowsMenu(int cursorX, int cursorY);
	bool CursorIsOverWindowsMenuIncludingButtons(int cursorX, int cursorY);
	void AddFile(File folder);
	//bool IsHovered(int cursorX, int cursorY);
	bool IsValidFileDropLocation(File* icon, int cursorX, int cursorY);
};



namespace Laptop {

	void Init();
	void Update(float deltaTime);
	bool CursorIsHand();
	void PrepareUIForRaster();
	void ResetFrontMostOnAllWindows();
	//void UnselectAllFiles();
	Window* GetHoveredWindow();
	Window* GetHoveredWindowIncludingResizeGraceDistance();
	bool IsTheCurrentLocactionAValideFileDropLocation();
	bool AFileIsBeingDragged();

	inline float _cursorXFloat = LAPTOP_DISPLAY_WIDTH * 0.5f;
	inline float _cursorYFloat = LAPTOP_DISPLAY_HEIGHT * 0.5f;
	inline int _cursorX = 0;
	inline int _cursorY = 0;

	inline std::vector<File> _files;
	inline std::vector<Window> _windows;
	inline bool _showChrome = false;
	inline float _chromeScale = 0.5f;
	inline bool _chromeMessageSeen = false;
	inline int _frameIndex = 0;
	inline bool _allowAnotherFileToBeOpenedThisFrame = false;
}