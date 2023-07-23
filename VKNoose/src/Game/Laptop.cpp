#include "Laptop.h"
#include "GameData.h"
#include "AssetManager.h"
#include "../Renderer/RasterRenderer.h"
#include "../IO/Input.h"
#include "../Util.h"

bool sortfileByDragState(File a, File b) {
	return (a.dragging < b.dragging);
}

bool sortWindowByFrontMost(Window a, Window b) {
	return (a.isFrontMost < b.isFrontMost);
}

void Window::UpdateSize(float deltaTime) {

	// If already resizing, keep current state
	if (resizing)
		return;

	// Resize avalibility
	resizeAvalibility = ResizeAvalibility::NONE;
	if (visible) {
		int x = Laptop::_cursorX;
		int y = Laptop::_cursorY;


		if (x < GetLeftEdge() + RESIZE_GRACE_DISTANCE && x > GetLeftEdge() - RESIZE_GRACE_DISTANCE) {
			resizeAvalibility = ResizeAvalibility::WEST;
		}
		if (x < GetRightEdge() + RESIZE_GRACE_DISTANCE && x > GetRightEdge() - RESIZE_GRACE_DISTANCE) {
			resizeAvalibility = ResizeAvalibility::EAST;
		}

		if (y < GetTopEdge() + RESIZE_GRACE_DISTANCE && y > GetTopEdge() - RESIZE_GRACE_DISTANCE) {
			if (resizeAvalibility == ResizeAvalibility::WEST)
				resizeAvalibility = ResizeAvalibility::NORTH_WEST;
			else if (resizeAvalibility == ResizeAvalibility::EAST)
				resizeAvalibility = ResizeAvalibility::NORTH_EAST;
			else
				resizeAvalibility = ResizeAvalibility::NORTH;
		}
		if (y < GetBottomEdge() + RESIZE_GRACE_DISTANCE && y > GetBottomEdge() - RESIZE_GRACE_DISTANCE) {
			if (resizeAvalibility == ResizeAvalibility::WEST)
				resizeAvalibility = ResizeAvalibility::SOUTH_WEST;
			else if (resizeAvalibility == ResizeAvalibility::EAST)
				resizeAvalibility = ResizeAvalibility::SOUTH_EAST;
			else
				resizeAvalibility = ResizeAvalibility::SOUTH;
		}
	}


	if (currentWidth == -1)
		currentWidth = windowedWidth;
	if (currentHeight == -1)
		currentHeight = windowedHeight;
		if (currentXPos == -1)
			currentXPos = windowedXPos;
		if (currentYPos == -1)
			currentYPos = windowedYPos;

		if (minimizing) {
			float minHeight = 100;
			float minWidth = 200;
			currentWidth = Util::FInterpTo(currentWidth, minWidth, deltaTime, 60);
			currentHeight = Util::FInterpTo(currentHeight, minWidth, deltaTime, 60);
			//currentXPos = Util::FInterpTo(currentXPos, LAPTOP_DISPLAY_WIDTH / 2, deltaTime, 60);
			currentYPos = Util::FInterpTo(currentYPos, -50, deltaTime, 60);
			if (currentYPos <= 0) {
				visible = false;
			}
		}
		else if (maximizeState == MaximizeState::FULLSCREEN) {
			currentWidth = Util::FInterpTo(currentWidth, LAPTOP_DISPLAY_WIDTH, deltaTime, 60);
			currentHeight = Util::FInterpTo(currentHeight, LAPTOP_DISPLAY_HEIGHT - GetMenuHeight(), deltaTime, 60);
			currentXPos = Util::FInterpTo(currentXPos, LAPTOP_DISPLAY_WIDTH / 2, deltaTime, 60);
			currentYPos = Util::FInterpTo(currentYPos, LAPTOP_DISPLAY_HEIGHT / 2 - (GetMenuHeight() / 2), deltaTime, 60);
		}
		else if (maximizeState == MaximizeState::WINDOWED) {
			currentWidth = Util::FInterpTo(currentWidth, windowedWidth, deltaTime, 60);
			currentHeight = Util::FInterpTo(currentHeight, windowedHeight, deltaTime, 60);
			currentXPos = Util::FInterpTo(currentXPos, windowedXPos, deltaTime, 60);
			currentYPos = Util::FInterpTo(currentYPos, windowedYPos, deltaTime, 60);
		}
}

float Window::GetCurrentWidth() {
	
	if (resizing) {
		if (resizeAvalibility == ResizeAvalibility::WEST || resizeAvalibility == ResizeAvalibility::SOUTH_WEST || resizeAvalibility == ResizeAvalibility::NORTH_WEST) {
			return rightEdgeBeforeResize - GetLeftEdge();
		}
		if (resizeAvalibility == ResizeAvalibility::EAST || resizeAvalibility == ResizeAvalibility::SOUTH_EAST || resizeAvalibility == ResizeAvalibility::NORTH_EAST) {
			return GetRightEdge() - leftEdgeBeforeResize;
		}
	}
	return currentWidth;
}

int Window::GetCurrentHeight() {

	if (resizing) {
		if (resizeAvalibility == ResizeAvalibility::SOUTH || resizeAvalibility == ResizeAvalibility::SOUTH_EAST || resizeAvalibility == ResizeAvalibility::SOUTH_WEST) {
			return topEdgeBeforeResize - GetBottomEdge();
		}
		if (resizeAvalibility == ResizeAvalibility::NORTH || resizeAvalibility == ResizeAvalibility::NORTH_EAST || resizeAvalibility == ResizeAvalibility::NORTH_WEST) {
			return GetTopEdge() - bottomEdgeBeforeResize;
		}
	}
	return currentHeight;
}

int Window::GetTopEdge() {

	if (resizing) {
		if (resizeAvalibility == ResizeAvalibility::SOUTH || resizeAvalibility == ResizeAvalibility::SOUTH_EAST || resizeAvalibility == ResizeAvalibility::SOUTH_WEST) {
			return topEdgeBeforeResize;
		}
		if (resizeAvalibility == ResizeAvalibility::NORTH || resizeAvalibility == ResizeAvalibility::NORTH_EAST || resizeAvalibility == ResizeAvalibility::NORTH_WEST) {
			int edge = Laptop::_cursorY + resizeDragOffsetY;
			edge = std::max(bottomEdgeBeforeResize + MINIMUM_WINDOW_HEIGHT, edge);
			return edge;
		}
	}
	 
	return currentYPos + (GetCurrentHeight() * 0.5f);
}
int Window::GetBottomEdge() {

	if (resizing) {
		if (resizeAvalibility == ResizeAvalibility::SOUTH || resizeAvalibility == ResizeAvalibility::SOUTH_EAST || resizeAvalibility == ResizeAvalibility::NORTH_WEST) {
			int edge = Laptop::_cursorY + resizeDragOffsetY;
			edge = std::min(topEdgeBeforeResize - MINIMUM_WINDOW_HEIGHT, edge);
			return edge;
		}
		if (resizeAvalibility == ResizeAvalibility::NORTH || resizeAvalibility == ResizeAvalibility::NORTH_EAST || resizeAvalibility == ResizeAvalibility::NORTH_WEST) {
			return bottomEdgeBeforeResize;
		}
	}
	return currentYPos - (GetCurrentHeight() / 2);
}
int Window::GetLeftEdge() {

	if (resizing) {
		if (resizeAvalibility == ResizeAvalibility::WEST || resizeAvalibility == ResizeAvalibility::SOUTH_WEST || resizeAvalibility == ResizeAvalibility::NORTH_WEST) {
			int edge = Laptop::_cursorX + resizeDragOffsetX;
			edge = std::min(rightEdgeBeforeResize - MINIMUM_WINDOW_WIDTH, edge);
			return edge;
		}
		if (resizeAvalibility == ResizeAvalibility::EAST || resizeAvalibility == ResizeAvalibility::SOUTH_EAST || resizeAvalibility == ResizeAvalibility::NORTH_EAST) {
			return leftEdgeBeforeResize;
		}
	}
	
	return currentXPos - (GetCurrentWidth() / 2);
}
int Window::GetRightEdge() {

	if (resizing) {
		if (resizeAvalibility == ResizeAvalibility::WEST || resizeAvalibility == ResizeAvalibility::SOUTH_WEST || resizeAvalibility == ResizeAvalibility::NORTH_WEST) {
			return rightEdgeBeforeResize;
		}
		if (resizeAvalibility == ResizeAvalibility::EAST || resizeAvalibility == ResizeAvalibility::SOUTH_EAST || resizeAvalibility == ResizeAvalibility::NORTH_EAST) {

			int edge = Laptop::_cursorX + resizeDragOffsetX;


			edge = std::max(leftEdgeBeforeResize + MINIMUM_WINDOW_WIDTH, edge);
			return edge;
		}
	}
	
	return currentXPos + (GetCurrentWidth() * 0.5f);
}

int Window::GetMenuHeight() {
	return AssetManager::GetTexture("OS_window_mainbar")->_height;
}
int Window::GetRightEdgeOfMenuButtons() {
	return GetLeftEdge() + AssetManager::GetTexture("OS_buttons_default")->_width;
}
int Window::GetCenterYPos()
{
	if (GetCurrentHeight() % 2) {
		return currentYPos + 1;
	}
	else {
		return currentYPos;
	}
}
std::string Window::GetButtonTextureName() {
	if (buttonState == ButtonState::CLOSE_HOVER)
		return "OS_buttons_close_hover";
	else if (buttonState == ButtonState::CLOSE_CLICK)
		return "OS_buttons_close_click";
	else if (buttonState == ButtonState::MIN_HOVER)
		return "OS_buttons_min_hover";
	else if (buttonState == ButtonState::MIN_CLICK)
		return "OS_buttons_min_click";
	else if (buttonState == ButtonState::MAX_HOVER)
		return "OS_buttons_max_hover";
	else if (buttonState == ButtonState::MAX_CLICK)
		return "OS_buttons_max_click";
	else
		return "OS_buttons_default";
}
void Window::Close() {
	visible = false;
	buttonState = ButtonState::DEFAULT;
	isFrontMost = false;
}
void Window::Minimize() {
	minimizing = true;
	buttonState = ButtonState::DEFAULT;
}
void Window::Maximize() {
	if (maximizeState == MaximizeState::FULLSCREEN)
		maximizeState = MaximizeState::WINDOWED;
	else
		maximizeState = MaximizeState::FULLSCREEN;
	buttonState = ButtonState::DEFAULT;
}
void Window::Open(int spawnX, int spawnY) {

	// This is some bug you dont understand. Open() is being called multipe times per frame somehow
	if (!Laptop::_allowAnotherFileToBeOpenedThisFrame)
		return;
	Laptop::_allowAnotherFileToBeOpenedThisFrame = false;

	//std::cout << "open " <<  Laptop::_frameIndex << "\n";

	if (visible && isFrontMost) {
		flashTimer = 3.9f;
		return;
	}

	if (!visible) {
		visible = true;
		buttonState = ButtonState::DEFAULT;
		minimizing = false;
		// Determine target xy pos, by centering it at the file location (spawn location) but not letting it go off screen
		float targetXPos = spawnX;
		float targetYPos = spawnY;
		float graceDistance = 20;
		if (targetXPos < windowedWidth / 2 + graceDistance) {
			targetXPos = windowedWidth / 2 + graceDistance;
		}
		if (targetYPos < windowedHeight / 2 + graceDistance) {
			targetYPos = windowedHeight / 2 + graceDistance;
		}
		if (targetXPos > LAPTOP_DISPLAY_WIDTH - windowedWidth / 2 - graceDistance) {
			targetXPos = LAPTOP_DISPLAY_WIDTH - windowedWidth / 2 - graceDistance;
		}
		if (targetYPos > LAPTOP_DISPLAY_HEIGHT - windowedHeight / 2 - graceDistance) {
			targetYPos = LAPTOP_DISPLAY_HEIGHT - windowedHeight / 2 - graceDistance;
		}
		currentXPos = targetXPos;
		currentYPos = targetYPos;
		windowedXPos = targetXPos;
		windowedYPos = targetYPos;
		currentWidth = windowedWidth * 0.5;
		currentHeight = windowedHeight * 0.5;
	}
	Laptop::ResetFrontMostOnAllWindows();
	isFrontMost = this;
	sort(Laptop::_windows.begin(), Laptop::_windows.end(), sortWindowByFrontMost);
}

bool Window::CursorIsOverWindow(int cursorX, int cursorY) {
	return (cursorX > GetLeftEdge() && cursorX < GetRightEdge() && cursorY < GetTopEdge() && cursorY > GetBottomEdge());
}

bool Window::CursorIsOverWindowIncludingResizeGraceDistance(int cursorX, int cursorY)
{

	return (cursorX > GetLeftEdge() - RESIZE_GRACE_DISTANCE && cursorX < GetRightEdge() + RESIZE_GRACE_DISTANCE && cursorY < GetTopEdge() + RESIZE_GRACE_DISTANCE && cursorY > GetBottomEdge() - RESIZE_GRACE_DISTANCE);
}

bool Window::CursorIsOverWindowsMenu(int cursorX, int cursorY) {
	return (cursorX > GetRightEdgeOfMenuButtons() && cursorX < GetRightEdge() && cursorY < GetTopEdge() && cursorY > GetTopEdge() - GetMenuHeight());
}

bool Window::CursorIsOverWindowsMenuIncludingButtons(int cursorX, int cursorY) {
	return (cursorX > GetLeftEdge() && cursorX < GetRightEdge() && cursorY < GetTopEdge() && cursorY > GetTopEdge() - GetMenuHeight());
}

void Window::AddFile(File file)
{
	File& newlyAddedfile = files.emplace_back(file);
	newlyAddedfile.parentWindow = this->name;

	//newlyAddedfile.xPos = this->windowedXPos + newlyAddedfile.xPos;
	//newlyAddedfile.yPos = this->windowedYPos + newlyAddedfile.yPos;
	//newlyAddedfile.xPos =  newlyAddedfile.xPos;
	//newlyAddedfile.yPos = newlyAddedfile.yPos;
}

/*bool Window::IsHovered(int cursorX, int cursorY)
{
	// First bail if the mouse isn't even over the window
	if (!CursorIsOverWindow(cursorX, cursorY) || !visible)
		return false;

	// Ok so we're overlapping, now bail with a true early if this is front most window
	if (isFrontMost)
		return true;
	
	// Now check if no other window has the cursor overlapping it and that window is front most
	//for (Window& otherWindow : Laptop::_windows) {
	//	if (otherWindow.isFrontMost && otherWindow.CursorIsOverWindow(cursorX, cursorY))
	//		return false;
	//}

	// If you made it this far then you're good!
	return true;
}*/

bool Window::IsValidFileDropLocation(File* file, int cursorX, int cursorY)
{
	if (CursorIsOverWindowsMenuIncludingButtons(cursorX, cursorY))
		return false;
	
	if (!allowFileDrop)
		return false;

	// prevent folders being moved into themselves
	if (file) {
		if (file->textureName == "OS_icon_folder_photos" && this->name == "window_photos") {
			return false;
		}
		if (file->textureName == "OS_icon_folder_writing" && this->name == "writing") {
			return false;
		}
	}
	// Otherwise good
	return true;
}

bool Laptop::AFileIsBeingDragged() {
	for (File& file : _files) {
		if (file.dragging) {
			return true;
		}
	}
	for (Window& window : _windows) {
		for (File& file : window.files) {
			if (file.dragging) {
				return true;
			}
		}
	}
	return false;
}


int File::GetWidth() {
	return AssetManager::GetTexture(textureName)->_width;
}	
int File::GetHeight() {
	return AssetManager::GetTexture(textureName)->_height;
}
bool File::CursorIsOverlapping(int cursorX, int cursorY) {
	if (cursorX > GetGlobalXPosition() && cursorX < GetGlobalXPosition() + GetWidth() && cursorY > GetGlobalYPosition() && cursorY < GetGlobalYPosition() + GetHeight()) {
		//std::cout << "overlapping\n";
		return true;
	}
	return false;
}

float File::GetGlobalXPosition() {
	Window* parent = (Window*)GetParent();
	if (parent) {
		return parent->GetLeftEdge() + xPos;
	}
	else {
		return xPos;
	}
}

float File::GetGlobalYPosition() {
	Window* parent = (Window*)GetParent();
	if (parent) {
		return parent->GetTopEdge() + yPos;
	}
	else {
		return yPos;
	}
}

void* File::GetParent() {
	if (parentWindow != "") {
		for (Window& window : Laptop::_windows) {
			if (window.name == parentWindow) {
				return &window;
			}
		}
	}
	return nullptr;
}

/*bool File::IsHovered(int cursorX, int cursorY)
{
	return false;
}*/

/*void file::RepositionWithinParentWindowBounds()
{
	Window* parent = (Window*)GetParent();
	float graceDistance = 6.0f;

	if (xPos < 0 + graceDistance)
		xPos = graceDistance;

	if (xPos > parent->GetRightEdge() - graceDistance + GetWidth())
		xPos = parent->GetRightEdge() - graceDistance + GetWidth();
}*/

void File::Update(float deltaTime, int cursorX, int cursorY) {

	//if (hasBeenUpdatedThisFrame)
	//	return;
	//hasBeenUpdatedThisFrame = true;

	// double click countdown timer
	doubleClickCountDown -= deltaTime;
	doubleClickCountDown = std::max(doubleClickCountDown, 0.0f);

	// Double click
	if (Input::LeftMousePressed() && CursorIsOverlapping(cursorX, cursorY) && doubleClickCountDown > 0) {

		if (textureName == "OS_icon_chrome") {
			if (!Laptop::_chromeMessageSeen) {
				TextBlitter::Type("Did she unplug the router?", 4.0f, 0.25);
				Laptop::_chromeMessageSeen = true;
			}
		}

		// Open windows
		for (Window& window : Laptop::_windows) {
			if (window.correspondingFile == textureName) {
				window.Open(xPos, yPos);
			}
		}
		selected = false;
		dragging = false;
		return;
	}

	// Begin drag
	if (Input::LeftMousePressed() && CursorIsOverlapping(cursorX, cursorY) && !dragging) {

		Window* parentWindow = (Window*)GetParent();

		bool safe = false;

		// First check a window isn't in front
		bool cursorOverAWindow = false;
		for (Window& window : Laptop::_windows) {
			for (Window& otherWindow : Laptop::_windows) {
				if (&otherWindow != parentWindow && otherWindow.visible && otherWindow.CursorIsOverWindow(cursorX, cursorY))
					cursorOverAWindow = true;
			}
		}

		// allow if is on desktop and no window is in front
		if (parentWindow == nullptr && !cursorOverAWindow) {
			safe = true;
		}

		// allow if parent is a window that is front most
		if (parentWindow && parentWindow->isFrontMost) {
			safe = true;
		}

		// Disallow if parent window is dragging 
		if (parentWindow && parentWindow->dragging) {
			safe = false;
		}

		// If safe, then dragging this file is allowed
		if (safe) {
			dragging = true;
			dragOffsetX = xPos - cursorX;
			dragOffsetY = yPos - cursorY;
			xPosBeforeDrag = xPos;
			yPosBeforeDrag = yPos;

			for (auto& otherfile : Laptop::_files)
				otherfile.selected = false;

			selected = true;
			doubleClickCountDown = 0.25f;
			hasBeenDraggedAtLeastOnePixel = false;
		}
	}

	// Drag
	if (dragging && selected) {
		xPos = cursorX + dragOffsetX;
		yPos = cursorY + dragOffsetY;
		if (xPos != xPosBeforeDrag && yPos != yPosBeforeDrag) {
			hasBeenDraggedAtLeastOnePixel = true;
		}
	}

	// End drag 
	if (!Input::LeftMouseDown() && dragging) {

		dragging = false;
		hasBeenDraggedAtLeastOnePixel = false;

		Window* parent = (Window*)GetParent();
		Window* destinationWindow = Laptop::GetHoveredWindow();

		// User drops on DESKTOP
		if (!destinationWindow) {
			dragOffsetX = cursorX - GetGlobalXPosition();
			dragOffsetY = cursorY - GetGlobalYPosition();
			xPos = cursorX - dragOffsetX;
			yPos = cursorY - dragOffsetY;

			if (parent) {
				parentWindow = "";
				Laptop::_files.push_back(*this);
				// Remove from parent
				if (parent) {
					for (int i = 0; i < parent->files.size(); i++) {
						if (parent->files[i].textureName == textureName) {
							parent->files.erase(parent->files.begin() + i);
							return;
						}
					}
				}
			}
		}
		// User drops in FOLDER
		else {

			// If file drop location is invalid, then return the file to where it came from
			if (!destinationWindow->IsValidFileDropLocation(this, cursorX, cursorY) || !Laptop::IsTheCurrentLocactionAValideFileDropLocation()) {

				xPos = xPosBeforeDrag;
				yPos = yPosBeforeDrag;
				return;
			}

			dragOffsetX = cursorX - GetGlobalXPosition();
			dragOffsetY = cursorY - GetGlobalYPosition();
			xPos = cursorX - destinationWindow->GetLeftEdge() - dragOffsetX;
			yPos = cursorY - destinationWindow->GetTopEdge() - dragOffsetY;

			if (parent != destinationWindow) {
				parentWindow = destinationWindow->name;
				destinationWindow->files.push_back(*this);
				//RepositionWithinParentWindowBounds();

				// Remove from parent
				if (parent) {
					for (int i = 0; i < parent->files.size(); i++) {
						if (parent->files[i].textureName == textureName) {
							parent->files.erase(parent->files.begin() + i);
							return;
						}
					}
				}
				// Remove from desktop
				else {
					// Remove the instance of this in the desktop files vector
					for (int i = 0; i < Laptop::_files.size(); i++) {
						if (Laptop::_files[i].textureName == textureName) {
							Laptop::_files.erase(Laptop::_files.begin() + i);
							return;
						}
					}
				}
			}
		}
	}
		

	// Prevent them getting lost
	/*xPos = std::min(xPos, (float)LAPTOP_DISPLAY_WIDTH - GetWidth());
	yPos = std::min(yPos, (float)LAPTOP_DISPLAY_HEIGHT - GetHeight());
	xPos = std::max(xPos, 0.0f);
	yPos = std::max(yPos, 0.0f);*/

	// Unselect file
	if (Input::LeftMousePressed()) {
		if (!CursorIsOverlapping(cursorX, cursorY))
			selected = false;
	}
}

struct ClickableRegion {
	int xMin;
	int xMax;
	int yMin;
	int yMax;

	bool MouseHover(int cursorX, int cursorY) {
		return (cursorX > xMin && cursorX < xMax && cursorY > yMin && cursorY < yMax);
	}
};





void Laptop::Init()
{
	_files.clear();
	_windows.clear();
	_showChrome = false;

	File a = { 57, 330, "OS_icon_folder_photos"};
	File b = { 125, 312, "OS_icon_folder_writing" };
	File c = { 59, 247, "OS_icon_chrome" };
	_files.push_back(a);
	_files.push_back(b);
	_files.push_back(c);

	Window window = { 640 * 0.5, 430 * 0.5, 400, 200, "window_photos", "OS_icon_folder_photos", MaximizeState::WINDOWED, true, "OS_window_mainbar_text_photos"};
	Window window2 = { 400, 200, 300, 150, "writing", "OS_icon_folder_writing", MaximizeState::WINDOWED, true, "OS_window_mainbar_text_writing"};
	Window window3 = { 640 * 0.5, 430 * 0.5, 640 * 0.75, 430 * 0.75, "chrome", "OS_icon_chrome", MaximizeState::WINDOWED, false, ""};
	Window painting = { 640 * 0.5, 430 * 0.5, 640 * 0.75, 430 * 0.75, "painting_picture", "OS_icon_painting", MaximizeState::WINDOWED, false, "OS_window_mainbar_text_painting", "OS_picture_painting"};

	File photo = { 100, -100, "OS_icon_painting"};
	window.AddFile(photo);

	_windows.push_back(window);
	_windows.push_back(window2);
	_windows.push_back(window3);
	_windows.push_back(painting);

	_chromeMessageSeen = false;
}

void Laptop::Update(float deltaTime)
{
	_frameIndex++;

	// Audio
	static bool mousePressed = false;
	if (Input::LeftMousePressed()) {
		Audio::PlayAudio("MouseClickIn.wav", 0.14f);
		mousePressed = true;
	}
	if (!Input::LeftMouseDown() && mousePressed) {
		Audio::PlayAudio("MouseClickOut.wav", 0.15f);
		mousePressed = false;
	}

	float mouseSensitivity = 50.5f;
	float xoffset = Input::GetMouseOffsetY() * mouseSensitivity* deltaTime;
	float yoffset = Input::GetMouseOffsetX() * mouseSensitivity* deltaTime;
	_cursorXFloat += yoffset;
	_cursorYFloat -= xoffset;
	_cursorXFloat = std::min(_cursorXFloat, (float)LAPTOP_DISPLAY_WIDTH);
	_cursorYFloat = std::min(_cursorYFloat, (float)LAPTOP_DISPLAY_HEIGHT);
	_cursorXFloat = std::max(_cursorXFloat, 0.0f);
	_cursorYFloat = std::max(_cursorYFloat, 0.0f);

	_cursorX = _cursorXFloat;
	_cursorY = _cursorYFloat;


	//_cursorX = (int)_cursorX;
	//_cursorY = (int)_cursorY;

	static float lastCursorX = _cursorX;

	float cursorOffsetX = _cursorX - lastCursorX;
	lastCursorX = _cursorX;

	//if (cursorOffsetX != 0)
		//std::cout << _cursorX << " " << cursorOffsetX << "\n";




	// Windows
	for (Window& window : _windows) {

		window.UpdateSize(deltaTime);

		// Decrement  timer
		window.flashTimer = std::max(window.flashTimer - deltaTime * 6, 0.0f); 
		window.mainbarDoubleClickCountDown = std::max(window.mainbarDoubleClickCountDown - deltaTime, 0.0f);

		if (!window.visible)
			continue;

		// Cursor over menu?
		bool cursorIsOverWindowsMenu = window.CursorIsOverWindowsMenu(_cursorX, _cursorY);

		// Clicks the menu bar
		if (Input::LeftMousePressed() && window.CursorIsOverWindowsMenu(_cursorX, _cursorY)) {

			// Maxiimize
			if (window.mainbarDoubleClickCountDown > 0) {
				window.Maximize();
			}
			window.mainbarDoubleClickCountDown = 0.25f;
		}

		// Begin drag
		if (Input::LeftMousePressed() && cursorIsOverWindowsMenu && window.maximizeState == MaximizeState::WINDOWED && window.isFrontMost) {
			window.dragging = true;
			window.dragOffsetX = window.currentXPos - _cursorX;
			window.dragOffsetY = window.currentYPos - _cursorY;
		}
		// Drag
		if (window.dragging) {
			window.currentXPos = _cursorX + window.dragOffsetX;
			window.currentYPos = _cursorY + window.dragOffsetY;
		}
		// End drag
		if (!Input::LeftMouseDown() && window.dragging) {
			window.dragging = false;
			window.windowedXPos = window.currentXPos;
			window.windowedYPos = window.currentYPos;
		}

		// Begin resize
		if (Input::LeftMousePressed() && window.resizeAvalibility != ResizeAvalibility::NONE) {
			window.xPosBeforeResize = window.currentXPos;
			window.yPosBeforeResize = window.currentYPos;
			window.widthBeforeResize = window.currentWidth;
			window.heightBeforeResize = window.currentHeight;
			window.leftEdgeBeforeResize = window.GetLeftEdge();
			window.rightEdgeBeforeResize = window.GetRightEdge();
			window.topEdgeBeforeResize = window.GetTopEdge();
			window.bottomEdgeBeforeResize = window.GetBottomEdge();

			if (window.resizeAvalibility == ResizeAvalibility::WEST || ResizeAvalibility::NORTH_WEST || ResizeAvalibility::SOUTH_WEST) {
				window.resizeDragOffsetX = window.GetLeftEdge() - _cursorX;
			}
			if (window.resizeAvalibility == ResizeAvalibility::EAST || ResizeAvalibility::NORTH_EAST || ResizeAvalibility::SOUTH_EAST) {
				window.resizeDragOffsetX = window.GetRightEdge() - _cursorX;
			}
			if (window.resizeAvalibility == ResizeAvalibility::SOUTH || ResizeAvalibility::SOUTH_WEST || ResizeAvalibility::SOUTH_EAST) {
				window.resizeDragOffsetY = window.GetBottomEdge() - _cursorY;
			}
			if (window.resizeAvalibility == ResizeAvalibility::NORTH || ResizeAvalibility::NORTH_WEST || ResizeAvalibility::NORTH_EAST) {
				window.resizeDragOffsetY = window.GetTopEdge() - _cursorY;
			}


			window.resizing = true;
			//std::cout << "Clicked for resize\n";
		}
		// Resize
		if (window.resizing) {
			window.dragging = false;
		}

		// End resize
		if (!Input::LeftMouseDown() && window.resizing) {

			if (window.resizeAvalibility == ResizeAvalibility::WEST || ResizeAvalibility::NORTH_WEST || ResizeAvalibility::SOUTH_WEST) {
				window.currentXPos = window.GetLeftEdge() + (window.GetCurrentWidth()) * 0.5;
				window.windowedXPos = window.GetLeftEdge() + (window.GetCurrentWidth()) * 0.5;
				if ((int)window.GetCurrentWidth() % 2) {
					window.currentXPos++;
					window.windowedXPos++;
				}
				window.currentWidth = window.GetCurrentWidth();
				window.windowedWidth = window.GetCurrentWidth();
			}
			if (window.resizeAvalibility == ResizeAvalibility::EAST || ResizeAvalibility::NORTH_EAST || ResizeAvalibility::SOUTH_EAST) {
				window.currentXPos = window.GetRightEdge() - (window.GetCurrentWidth()) * 0.5;
				window.windowedXPos = window.GetRightEdge() - (window.GetCurrentWidth()) * 0.5;
				if ((int)window.GetCurrentWidth() % 2) {
					window.currentXPos++;
					window.windowedXPos++;
				}
				window.currentWidth = window.GetCurrentWidth();
				window.windowedWidth = window.GetCurrentWidth();
			}

			if (window.resizeAvalibility == ResizeAvalibility::SOUTH || ResizeAvalibility::SOUTH_EAST || ResizeAvalibility::SOUTH_WEST) {
				window.currentYPos = window.GetBottomEdge() + (window.GetCurrentHeight()) * 0.5;
				window.windowedYPos = window.GetBottomEdge() + (window.GetCurrentHeight()) * 0.5;
				if ((int)window.GetCurrentHeight() % 2) {
					window.currentYPos++;
					window.windowedYPos++;
				}
				window.currentHeight = window.GetCurrentHeight();
				window.windowedHeight = window.GetCurrentHeight();
			}
			if (window.resizeAvalibility == ResizeAvalibility::NORTH || ResizeAvalibility::NORTH_EAST || ResizeAvalibility::NORTH_WEST) {
				window.currentYPos = window.GetTopEdge() - (window.GetCurrentHeight()) * 0.5;
				window.windowedYPos = window.GetTopEdge() - (window.GetCurrentHeight()) * 0.5;
				if ((int)window.GetCurrentHeight() % 2) {
					window.currentYPos++;
					window.windowedYPos++;
				}
				window.currentHeight = window.GetCurrentHeight();
				window.windowedHeight = window.GetCurrentHeight();
			}

			
			window.resizeAvalibility = ResizeAvalibility::NONE;
			window.resizing = false;
		}

		// Button state
		bool closeHover = (_cursorX > window.GetLeftEdge() + 4 && _cursorX < window.GetLeftEdge() + 13 && _cursorY < window.GetTopEdge() - 4 && _cursorY > window.GetTopEdge() - 14);
		bool minHover = (_cursorX > window.GetLeftEdge() + 14 && _cursorX < window.GetLeftEdge() + 22 && _cursorY < window.GetTopEdge() - 4 && _cursorY > window.GetTopEdge() - 14);
		bool maxHover = (_cursorX > window.GetLeftEdge() + 23 && _cursorX < window.GetLeftEdge() + 32 && _cursorY < window.GetTopEdge() - 4 && _cursorY > window.GetTopEdge() - 14);

		

		// Released click
		if (!Input::LeftMouseDown() && closeHover && window.buttonState == ButtonState::CLOSE_CLICK)
			window.Close();
		else if (!Input::LeftMouseDown() && minHover && window.buttonState == ButtonState::MIN_CLICK)
			window.Minimize();
		else if (!Input::LeftMouseDown() && maxHover && window.buttonState == ButtonState::MAX_CLICK)
			window.Maximize();

		// Hover
		else if (!Input::LeftMouseDown()) {
			if (closeHover)
				window.buttonState = ButtonState::CLOSE_HOVER;
			else if (minHover)
				window.buttonState = ButtonState::MIN_HOVER;
			else if (maxHover)
				window.buttonState = ButtonState::MAX_HOVER;
		}

		// Click
		else if (Input::LeftMouseDown()) {

			// Check no other window is infront
			bool cursorOverFrontMostWindow = false;
			for (Window& otherWindow : _windows) {
				if (&otherWindow == &window)
					continue;
				if (otherWindow.isFrontMost && otherWindow.CursorIsOverWindow(_cursorX, _cursorY))
					cursorOverFrontMostWindow = true;
			}
			// If not, then safe to begin the click
			if (!cursorOverFrontMostWindow) {
				if (closeHover)
					window.buttonState = ButtonState::CLOSE_CLICK;
				else if (minHover)
					window.buttonState = ButtonState::MIN_CLICK;
				else if (maxHover)
					window.buttonState = ButtonState::MAX_CLICK;
			}
		}

		// Default
		if (!Input::LeftMouseDown() && !closeHover && !minHover && !maxHover)
			window.buttonState = ButtonState::DEFAULT;
		

		// Select this window
		if (!window.isFrontMost && &window == Laptop::GetHoveredWindow()) {

			bool cursorOverWindow = window.CursorIsOverWindow(_cursorX, _cursorY);
			
			// First check you aren't actually hovering over the front most window, in front of this one
			bool cursorOverFrontMostWindow = false;
			for (Window& otherWindow : _windows) {
				if (otherWindow.isFrontMost && otherWindow.CursorIsOverWindow(_cursorX, _cursorY))
						cursorOverFrontMostWindow = true;
			}
			// all good? then select it
			if (Input::LeftMousePressed() && cursorOverWindow && !cursorOverFrontMostWindow) {
				ResetFrontMostOnAllWindows();
				window.isFrontMost = true;
				
				// Begin drag if required
				if (Input::LeftMousePressed() && cursorIsOverWindowsMenu && window.maximizeState == MaximizeState::WINDOWED) {
					window.dragging = true;
					window.dragOffsetX = window.currentXPos - _cursorX;
					window.dragOffsetY = window.currentYPos - _cursorY;
				}

				sort(_windows.begin(), _windows.end(), sortWindowByFrontMost);
				break;
			}
		}
	}

	// Update files
	for (auto& file : _files) {
		file.Update(deltaTime, _cursorX, _cursorY);
	}

	// Update files within folders
	for (Window& window : _windows) {
		for (auto& nestedfile : window.files) {
			nestedfile.Update(deltaTime, _cursorX, _cursorY);
		}
	}

	_allowAnotherFileToBeOpenedThisFrame = true;
}

bool Laptop::CursorIsHand()
{
	for (auto& file : _files) {
		if (file.dragging)
			return true;
	}
	for (auto& window : _windows) {
		for (auto& file : window.files) {
			if (file.dragging)
				return true;
		}
	}
	return false;
}

void Laptop::PrepareUIForRaster()
{
	//if (GetHoveredWindow()) {
	//	std::cout << GetHoveredWindow()->name << "\n";
	//}

	sort(_files.begin(), _files.end(), sortfileByDragState);

	// Draw files (except selected file, it needs to be drawn in front)
	for (const auto& file : _files) {

		if (file.dragging && file.hasBeenDraggedAtLeastOnePixel) {
			// don't draw
		}
		else {
			std::string texture = file.textureName;
			if (file.selected)
				texture += "_selected";
			RasterRenderer::DrawQuad(texture, file.xPos, file.yPos, RasterRenderer::Destination::LAPTOP_DISPLAY);
		}
	}

	// Draw windows
	for (auto& window : _windows) {

		if (!window.visible)
			continue;

		// Background
		std::string backgroundTexture = "OS_window_bg";
		if (!window.isFrontMost)
			backgroundTexture = "OS_window_bg_dark";
		if (window.name == "chrome")
			backgroundTexture = "OS_window_bg_light";

		if (window.backgroundTexture != "")
			backgroundTexture = window.backgroundTexture;

		if (!window.resizing) {
			RasterRenderer::DrawQuad(backgroundTexture, window.currentXPos, window.GetCenterYPos(), RasterRenderer::Destination::LAPTOP_DISPLAY, true, window.GetCurrentWidth(), window.GetCurrentHeight());
		}
		else {
			
			RasterRenderer::DrawQuad(backgroundTexture, window.GetLeftEdge(), window.GetBottomEdge(), RasterRenderer::Destination::LAPTOP_DISPLAY, false, window.GetCurrentWidth(), window.GetCurrentHeight());
		}

		// Menu bar
		std::string mainbarTexture = "OS_window_mainbar";
		if ((int)window.flashTimer % 2) {
			mainbarTexture = "OS_window_mainbar_light";
		}

		RasterRenderer::DrawQuad(mainbarTexture, window.GetLeftEdge(), window.GetTopEdge() - window.GetMenuHeight(), RasterRenderer::Destination::LAPTOP_DISPLAY, false, window.GetCurrentWidth(), window.GetMenuHeight());
		

		// Chrome extras
		if (window.name == "chrome" && !window.minimizing) {
			float xmargin = 100;
			float ymargin = 50;
			float x = window.GetLeftEdge() + xmargin;
			float y = window.GetTopEdge() - AssetManager::GetTexture("OS_window_mainbar_text_chrome_right")->_height - ymargin - AssetManager::GetTexture("OS_chrome_no_internet")->_height;
			RasterRenderer::DrawQuad("OS_chrome_no_internet", x, y, RasterRenderer::Destination::LAPTOP_DISPLAY);
			RasterRenderer::DrawQuad("OS_window_mainbar_text_chrome_right", window.GetRightEdge() - AssetManager::GetTexture("OS_window_mainbar_text_chrome_right")->_width, window.GetTopEdge() - AssetManager::GetTexture("OS_window_mainbar_text_chrome_right")->_height, RasterRenderer::Destination::LAPTOP_DISPLAY);
			RasterRenderer::DrawQuad("OS_window_mainbar_text_chrome_left", window.GetLeftEdge() , window.GetTopEdge() - AssetManager::GetTexture("OS_window_mainbar_text_chrome_left")->_height, RasterRenderer::Destination::LAPTOP_DISPLAY);
		}
		// Menu bar text
		if (!window.minimizing) {
			if (window.mainbarTextTexture != "") {
				RasterRenderer::DrawQuad(window.mainbarTextTexture, window.GetLeftEdge(), window.GetTopEdge() - AssetManager::GetTexture(window.mainbarTextTexture)->_height, RasterRenderer::Destination::LAPTOP_DISPLAY);
			}
		}
		// Buttons
		RasterRenderer::DrawQuad(window.GetButtonTextureName(), window.GetLeftEdge(), window.GetTopEdge() - window.GetMenuHeight(), RasterRenderer::Destination::LAPTOP_DISPLAY);
		
		// Nested files within windows
		if (window.visible) {
			sort(window.files.begin(), window.files.end(), sortfileByDragState);
			for (auto& nestedfile : window.files) {
				std::string textureName = nestedfile.textureName;

				if (nestedfile.dragging && nestedfile.hasBeenDraggedAtLeastOnePixel) {
					// don't draw
				}
				else {
					if (nestedfile.selected)
						textureName += "_selected";
					int x = nestedfile.GetGlobalXPosition();
					int y = nestedfile.GetGlobalYPosition();
					int laptopScreenHeight = 430;
					RasterRenderer::DrawQuad(textureName, x, y, RasterRenderer::Destination::LAPTOP_DISPLAY, false, -1, -1, window.GetLeftEdge(), window.GetRightEdge(), window.GetBottomEdge(), window.GetTopEdge() - window.GetMenuHeight());
				}
			}
		}
	}

	// Draw dragged file
	TextBlitter::AddDebugText("Desktop");
	for (int i = 0; i < _files.size(); i++) {
	//	TextBlitter::AddDebugText(" " + std::to_string(i) + ": " + _files[i].textureName + " " + "");
	}

//	TextBlitter::AddDebugText(std::to_string(_cursorXFloat) + ", " + std::to_string(_cursorYFloat));
//	TextBlitter::AddDebugText(std::to_string(_cursorX) + ", " + std::to_string(_cursorY));

	/*for (int i = 0; i < _windows.size(); i++) {
		Window& window = _windows[i];
		TextBlitter::AddDebugText( std::to_string(i) + ": " + std::to_string(window.currentXPos) + " " + std::to_string(window.GetCurrentWidth()) + "  " + std::to_string(window.xPosBeforeResize) + " " + std::to_string(window.widthBeforeResize) + "   " + std::to_string((int)window.resizeDragOffsetX) + "   " + std::to_string(window.leftEdgeBeforeResize) + "   " + std::to_string(window.GetLeftEdge()));
	}*/
	Window& win = _windows[_windows.size()-1];
	/*TextBlitter::AddDebugText("Current width:        " + std::to_string(win.currentWidth));
	TextBlitter::AddDebugText("Current height:        " + std::to_string(win.currentHeight));
	TextBlitter::AddDebugText("GetCurrentWidth:    " + std::to_string(win.GetCurrentWidth()));
	TextBlitter::AddDebugText("GetCurrentHeight:   " + std::to_string(win.GetCurrentHeight()));
	TextBlitter::AddDebugText("GetBottomEdge:   " + std::to_string(win.GetBottomEdge()));
	TextBlitter::AddDebugText("bottomEdgeBeforeResize: " + std::to_string(win.bottomEdgeBeforeResize));
	TextBlitter::AddDebugText("heightBeforeResize: " + std::to_string(win.heightBeforeResize));
	TextBlitter::AddDebugText("currentXPos:          " + std::to_string(win.currentXPos));
	TextBlitter::AddDebugText("currentYPos:          " + std::to_string(win.currentYPos));
	TextBlitter::AddDebugText("GetCenterYPos:          " + std::to_string(win.GetCenterYPos()));
	TextBlitter::AddDebugText("xPosBeforeResize:     " + std::to_string(win.xPosBeforeResize));
	TextBlitter::AddDebugText("widthBeforeResize:    " + std::to_string(win.widthBeforeResize));
	TextBlitter::AddDebugText("resizeDragOffsetX:    " + std::to_string(win.resizeDragOffsetX));
	TextBlitter::AddDebugText("leftEdgeBeforeResize: " + std::to_string(win.leftEdgeBeforeResize));
	TextBlitter::AddDebugText("GetLeftEdge:        " + std::to_string(win.GetLeftEdge()));
	*/
	for (const auto& file : _files) {
		if (file.dragging && file.hasBeenDraggedAtLeastOnePixel)
			RasterRenderer::DrawQuad(file.textureName + "_selected", file.xPos, file.yPos, RasterRenderer::Destination::LAPTOP_DISPLAY);
	}

	for (auto& window : _windows) {
		
		//TextBlitter::AddDebugText(window.name + " " + std::to_string(window.flashTimer));
		/*	for (int i = 0; i < window.files.size(); i++) {
			TextBlitter::AddDebugText(" " + std::to_string(i) + ": " + window.files[i].textureName + " " + std::to_string(window.files[i].IsHovered(_cursorX, _cursorY)));
		}
		*/
		if (!window.visible)
			continue;

		for (auto& nestedfile : window.files) {
			if (nestedfile.dragging && nestedfile.hasBeenDraggedAtLeastOnePixel)
				RasterRenderer::DrawQuad(nestedfile.textureName + "_selected", nestedfile.GetGlobalXPosition(), nestedfile.GetGlobalYPosition(), RasterRenderer::Destination::LAPTOP_DISPLAY);
		}
	}

	// Draw cursor
	std::string cursorName = "OS_cursor";
	if (CursorIsHand())
		cursorName = "OS_cursor_hand";

	// show the "disallow" cursor
	for (File& file : _files) {
		if (file.dragging) {
			if (Laptop::GetHoveredWindow() && !Laptop::GetHoveredWindow()->IsValidFileDropLocation(&file, _cursorX, _cursorY)) {
				cursorName = "OS_cursor_disallowed";
			}
		}
	}
	// show the "disallow" cursor
	for (Window& window : _windows) {
		for (File& file : window.files) {
			if (file.dragging) {
				if (Laptop::GetHoveredWindow() && !Laptop::GetHoveredWindow()->IsValidFileDropLocation(&file, _cursorX, _cursorY)) {
					cursorName = "OS_cursor_disallowed";
				}
			}
		}
	}


	int offsetX = 0;
	int offsetY = 0;
	if (GetHoveredWindowIncludingResizeGraceDistance() && !AFileIsBeingDragged()) {
		Window* window = GetHoveredWindowIncludingResizeGraceDistance();
		if (window->resizeAvalibility == ResizeAvalibility::EAST) {
			cursorName = "OS_cursor_resize_east";
			offsetX = -12;
			offsetY = 4;
	}
		if (window->resizeAvalibility == ResizeAvalibility::WEST) {
			cursorName = "OS_cursor_resize_west";
			offsetX = -1;
			offsetY = 4;
		}
		if (window->resizeAvalibility == ResizeAvalibility::NORTH)
			cursorName = "OS_cursor_resize_north";
		if (window->resizeAvalibility == ResizeAvalibility::SOUTH)
			cursorName = "OS_cursor_resize_south";
		if (window->resizeAvalibility == ResizeAvalibility::NORTH_EAST)
			cursorName = "OS_cursor_resize_north_east";
		if (window->resizeAvalibility == ResizeAvalibility::SOUTH_EAST)
			cursorName = "OS_cursor_resize_south_east";
		if (window->resizeAvalibility == ResizeAvalibility::NORTH_WEST)
			cursorName = "OS_cursor_resize_north_west";
		if (window->resizeAvalibility == ResizeAvalibility::SOUTH_WEST)
			cursorName = "OS_cursor_resize_south_west";
	}

	int x = _cursorX + offsetX;
	int y = _cursorY - AssetManager::GetTexture(cursorName)->_height + offsetY;
	RasterRenderer::DrawQuad(cursorName, x, y, RasterRenderer::Destination::LAPTOP_DISPLAY);
}

void Laptop::ResetFrontMostOnAllWindows()
{
	for (Window& window : _windows)
		window.isFrontMost = false;
}

/*void Laptop::UnselectAllFiles()
{
	for (auto& file : _files) {
		file.selected = false;
		file.dragging = false;
	}

	for (Window& window : _windows) {
		for (auto& file : window.files) {
			file.selected = false;
			file.dragging = false;
		}
	}
}*/

Window* Laptop::GetHoveredWindow()
{
	Window* closestFound = nullptr;
	for (int i = 0; i < _windows.size(); i++) {
		Window* window = &_windows[i];
		if (window->CursorIsOverWindow(_cursorX, _cursorY) && window->visible)
			closestFound = window;
	}
	return closestFound;
}

Window* Laptop::GetHoveredWindowIncludingResizeGraceDistance()
{
	Window* closestFound = nullptr;
	for (int i = 0; i < _windows.size(); i++) {
		Window* window = &_windows[i];
		if (window->CursorIsOverWindowIncludingResizeGraceDistance(_cursorX, _cursorY) && window->visible)
			closestFound = window;
	}
	return closestFound;
}

bool Laptop::IsTheCurrentLocactionAValideFileDropLocation()
{
	Window* hoveredWindow = GetHoveredWindow();
	if (hoveredWindow && hoveredWindow->IsValidFileDropLocation(nullptr, _cursorX, _cursorY)) {
		return true;
	}
	return false;
}
