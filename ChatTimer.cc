#include "ChatTimer.h"
#include "Model.h"

ChatTimer::ChatTimer(Model *model) : Runnable(model) {

}

void ChatTimer::run() {
	model->handleChatTimeout();
}
