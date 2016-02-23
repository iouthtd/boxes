#include <fstream>
#include <boost/math/special_functions/round.hpp>
#include <boost/filesystem.hpp>
#include <SDL2/SDL.h>
#include "Application.hpp"

int main(int argc, char *argv[])
{
	Application application;

	if (!boost::filesystem::is_directory("output")) {
		boost::filesystem::create_directory("output");
	}

	std::ofstream framerateLog("output/framerate.txt");
	while (!application.wantsToExit()) {
		application.update();

		{
			// Calculate framerate in FPS.
			static int lastUpdate = SDL_GetTicks();
			static int frames = 0;
			++frames;
			int timeDifference = SDL_GetTicks() - lastUpdate;
			if (timeDifference >= 3000) { // Update every three seconds.
				double fps = (double)frames / ((double)timeDifference / 1000.0);
				framerateLog << "FPS: " << fps << std::endl;

				lastUpdate = SDL_GetTicks();
				frames = 0;
			}
		}

		{
			// FPS capping
			static const Uint32 maxFPS = 60;
			static const Uint32 timeStep = boost::math::iround(1000.0 / double(maxFPS));
			static Uint32 nextFrame = SDL_GetTicks() + timeStep;

			Uint32 now = SDL_GetTicks();
			if (now < nextFrame) {
				SDL_Delay(nextFrame - now);
			}
			nextFrame += timeStep;
		}
	}

	return EXIT_SUCCESS;
}
