#pragma once
#include "../Common.h"

namespace TextBlitter {

	inline std::vector<Extent2Di> _charExtents;
	inline std::vector<GPUObjectData> _objectData;

	void Update(float deltaTime);
	void Type(std::string text);
	void AddDebugText(std::string text);
	void AskQuestion(std::string question, std::function<void(void)> callback);
	void ResetDebugText();
	void ResetBlitter();
	bool QuestionIsOpen();
}
