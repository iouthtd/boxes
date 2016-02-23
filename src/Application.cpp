#include "Application.hpp"

Application::Application() :
	m_wantsToExit(false), m_window(NULL), m_renderer(NULL)
{
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL_Init error", SDL_GetError(), NULL);
		std::cerr << "SDL_Init error: " << SDL_GetError() << std::endl;
	}

	m_window = SDL_CreateWindow("RenderBoxes", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, 0);
	m_renderer = SDL_CreateRenderer(m_window, -1, 0);
	SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
}

Application::~Application()
{
	SDL_DestroyRenderer(m_renderer);
	SDL_DestroyWindow(m_window);
	SDL_Quit();
}

void Application::update(void)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_QUIT) {
			m_wantsToExit = true;
		}

		g_console.processEvent(event);

		std::string cmd;
		do {
			cmd = g_console.getNextCommand();

			if (cmd == "blend") {
				m_animation.blendFrames();
				g_console.print("Done.");
			}

			if (cmd.find("framerate") == 0) {
				std::string argument;
				if (cmd.length() > 9) argument = cmd.substr(10);
				int framerate = -1;
				try {
					framerate = std::min(std::max(1, boost::lexical_cast<int>(argument)), 1000);
				}
				catch (boost::bad_lexical_cast) {
					if (argument.length() > 0) {
						g_console.print((boost::format("Invalid framerate '%s'") % argument).str());
					}
				}

				if (framerate != -1) {
					m_animation.framerate((double)framerate);
					g_console.print(boost::format("Set framerate to %ifps") % framerate);
				}
				else {
					g_console.print(boost::format("Current framerate is %i") % (int)m_animation.framerate());
				}
			}

			if (cmd.find("help") == 0) {
				g_console.print("Available commands:");
				g_console.print("blend framerate load pause resume reverse save quit");
			}

			if (cmd.find("load") == 0) {
				std::string argument;
				if (cmd.length() > 9) argument = cmd.substr(5);
				std::string filename = argument;
				if (m_animation.load(filename)) {
					g_console.print(boost::format("Successfully loaded %s") % filename);
				}
				else {
					g_console.print(boost::format("Error loading %s") % filename);
				}
			}

			if (cmd.find("pause") == 0) {
				m_animation.pause();
			}

			if (cmd.find("resume") == 0) {
				m_animation.resume();
			}

			if (cmd.find("reverse") == 0) {
				m_animation.reverse();
			}

			if (cmd == "save") {
				m_animation.save();
				g_console.print("Saved.");
			}

			if (cmd == "quit") {
				m_wantsToExit = true;
			}
		}
		while (cmd.length() > 0);

		if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.sym == SDLK_PLUS) m_animation.frameStep(1);
			if (event.key.keysym.sym == SDLK_MINUS) m_animation.frameStep(-1);
		}
	}

	SDL_SetRenderDrawColor(m_renderer, 0, 77, 0, 255);
	SDL_RenderClear(m_renderer);

	FramePtr frame = m_animation.currentFrame((double)SDL_GetTicks());
	if (frame.use_count() > 0) {
		SDL_Texture *frameTexture = frame->texture(m_renderer);
		SDL_Rect r;
		r.x = m_windowWidth / 2 - m_animation.width() / 2; r.y = m_windowHeight / 2 - m_animation.height() / 2;
		r.w = m_animation.width(); r.h = m_animation.height();
		SDL_RenderCopy(m_renderer, frameTexture, NULL, &r);
	}
	g_console.render(m_renderer);
	SDL_RenderPresent(m_renderer);
}
