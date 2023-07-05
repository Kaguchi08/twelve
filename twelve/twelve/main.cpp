#include "Game.h"

#ifndef _DEBUG
int main() {
#else
#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#endif
	Game game;

	if (game.Initialize())
	{
		game.RunLoop();
	}

	game.Shutdown();

	return 0;
}
