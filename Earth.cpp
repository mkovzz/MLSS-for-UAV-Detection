#include "Earth.h"

Earth& Earth::getInstance() {
	static Earth instance;
	return instance;
}
