#ifndef CONSOLE_HPP
#define CONSOLE_HPP

#include <vector>
#include <queue>
#include <string>
#include <iostream>
#include <fstream>
#include <boost/format.hpp>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>

class Console
{
public:
	Console();
	virtual ~Console();

	void print(boost::format message);
	void print(std::string message);
	void run(std::string command);
	std::string getNextCommand(void);
	void show(void);
	void hide(void);
	void toggle(void);
	void processEvent(SDL_Event &event);
	void render(SDL_Renderer *renderer);
protected:
private:
	std::ofstream m_logfile;
	bool m_showing;
	SDL_Rect m_boundary;
	std::string m_inputBuffer;
	std::vector<std::string> m_lines;
	std::queue<std::string> m_commandQueue;

	SDL_Surface *m_textSurface;
	SDL_Texture *m_textTexture;
	SDL_Renderer *m_renderer;

	bool m_waitForNextFrame;

	void initSurface(void);
};

extern Console g_console;

#endif // CONSOLE_HPP
