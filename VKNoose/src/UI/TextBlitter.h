#pragma once
#include "../Common.h"

namespace TextBlitter {

	inline std::vector<Extent2Di> _charExtents;
	inline std::vector<GPUObjectData2D> _objectData;

	inline std::string _debugTextToBilt = "";

	void Update(float deltaTime);
	void BlitAtPosition(std::string text, int x, int y, bool centered);
	void Type(std::string text, float coolDownTimer = -1, float delayTimer = -1);
	void AddDebugText(std::string text);
	void AskQuestion(std::string question, std::function<void(void)> callback, void* userPtr);
	void ResetDebugText();
	void ResetBlitter();
	bool QuestionIsOpen();
}
