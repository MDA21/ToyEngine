#include <GLFW/glfw3.h>
#include <Engine.h>


int main() {
	Engine engineToy;

	engineToy.init();

	engineToy.run();

	engineToy.cleanup();
}

