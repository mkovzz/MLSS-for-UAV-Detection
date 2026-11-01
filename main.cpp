#include "RunContoller.h"

int main() {
	RunController::Initialize();
	if (RunController::Instance != nullptr) {
		RunController::Instance->commandLoop();
	}
	RunController::Shutdown();
	return 0;
}
