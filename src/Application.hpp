#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <SDL2/SDL.h>
#include "Animation.hpp"
#include "Console.hpp"

class Application
{
public:
	Application();
	virtual ~Application();
	SDL_Window *window(void) { return m_window; };
	SDL_Renderer *renderer(void) { return m_renderer; };
	void update(void);
	bool wantsToExit(void) { return m_wantsToExit; };
protected:
private:
	bool m_wantsToExit;
	SDL_Window *m_window;
	SDL_Renderer *m_renderer;
	int m_windowWidth;
	int m_windowHeight;
	Animation m_animation;
};

#endif // APPLICATION_HPP
