#include "Game2.h"

#ifndef _DEBUG
int main()
{
#else
#include <Windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#endif
	Game2 game;

	if (game.Initialize())
	{
		game.RunLoop();
	}

	game.Terminate();

	return 0;
}
