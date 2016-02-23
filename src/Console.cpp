#include "Console.hpp"

Console g_console;

Console::Console() :
	m_showing(false),
	m_textSurface(NULL),
	m_textTexture(NULL),
	m_waitForNextFrame(false)
{
	m_logfile.open("output/log.txt");
	m_boundary.x = 0; m_boundary.y = 0; m_boundary.w = 800; m_boundary.h = 300;
	m_textSurface = SDL_CreateRGBSurface(0, m_boundary.w, m_boundary.h, 32,
		0x00ff0000,
		0x0000ff00,
		0x000000ff,
		0xff000000
	);

	SDL_assert(m_textSurface != NULL);
	print("~");
}

Console::~Console()
{
	if (m_textSurface != NULL) {
		SDL_FreeSurface(m_textSurface);
	}
}

void Console::print(boost::format message)
{
	print(message.str());
}

void Console::print(std::string message)
{
	m_lines.push_back(message);
	m_logfile << message << std::endl;
	std::cout << message << std::endl;
}

void Console::run(std::string command)
{
	if (command.find("msg") == 0) {
		print(command.substr(4));
	}
}

void Console::show(void)
{
	m_showing = true;
}

void Console::hide(void)
{
	m_showing = false;
}

void Console::toggle(void)
{
	if (m_showing) hide();
	else show();
}

void Console::processEvent(SDL_Event &event)
{
	// Block the console toggle key from being typed into the text bar.
	if (m_waitForNextFrame) {
		m_waitForNextFrame = false;
		return;
	}
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
			toggle();
			m_waitForNextFrame = true;
			return;
		}
	}

	if (!m_showing) return;

	if (event.type == SDL_TEXTINPUT) {
		m_inputBuffer += event.text.text;
	}
	else if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_RETURN) {
			if (m_inputBuffer.length() > 0) {
				m_lines.push_back(std::string("> ") + std::string(m_inputBuffer));
				run(m_inputBuffer);
				m_commandQueue.push(m_inputBuffer);
				m_inputBuffer = "";
			}
		}
		else if (event.key.keysym.sym == SDLK_BACKSPACE) {
			if (m_inputBuffer.length() > 0) {
				m_inputBuffer = m_inputBuffer.substr(0, std::max(0, (int)m_inputBuffer.length() - 1));
			}
		}
	}

	SDL_FillRect(m_textSurface, NULL, SDL_MapRGBA(m_textSurface->format, 0, 0, 0, 0));

	cairo_surface_t *cairoSurface = cairo_image_surface_create_for_data((unsigned char*)m_textSurface->pixels, CAIRO_FORMAT_ARGB32, m_textSurface->w, m_textSurface->h, m_textSurface->pitch);
	SDL_assert(cairoSurface != NULL);
	SDL_assert(cairo_surface_status(cairoSurface) == CAIRO_STATUS_SUCCESS);
	cairo_t *cr = cairo_create(cairoSurface);
	SDL_assert(cairo_status(cr) == CAIRO_STATUS_SUCCESS);
	cairo_surface_destroy(cairoSurface);

	cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
	cairo_set_font_size(cr, 16);

	double lineSpacing = 16.0;
	int startlineIndex = std::max(0, int(m_lines.size()) - int((double)m_textSurface->h / lineSpacing - 2.0));
	for (int i = startlineIndex; i < m_lines.size(); i ++) {
		int lineNumber = i - startlineIndex;
		cairo_move_to(cr, (double)m_boundary.x, (double)m_boundary.y + lineSpacing * (lineNumber + 1));
		cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
		cairo_show_text(cr, m_lines[i].c_str());
	}

	cairo_identity_matrix(cr);
	cairo_move_to(cr, (double)m_boundary.x, (double)m_boundary.y + (double)m_boundary.h - lineSpacing);
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_show_text(cr, (std::string("> ") + m_inputBuffer).c_str());

	cairo_destroy(cr);

	if (m_textTexture != NULL) {
		SDL_UpdateTexture(m_textTexture, NULL, m_textSurface->pixels, m_textSurface->pitch);
	}
}

std::string Console::getNextCommand(void)
{
	if (m_commandQueue.size() == 0) return "";

	std::string command(m_commandQueue.front());
	m_commandQueue.pop();
	return command;
}

void Console::render(SDL_Renderer *renderer)
{
	if (m_showing) {
		SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
		SDL_SetRenderDrawColor(renderer, 50, 50, 50, 200);
		SDL_RenderFillRect(renderer, &m_boundary);

		if (m_textTexture == NULL) {
			m_textTexture = SDL_CreateTextureFromSurface(renderer, m_textSurface);
		}

		SDL_RenderCopy(renderer, m_textTexture, NULL, &m_boundary);
	}
}
