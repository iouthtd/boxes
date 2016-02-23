#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cml/cml.h>
#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include <Box2D/Box2D.h>
#include "ImageCache.hpp"
#include "Console.hpp"

class Object {
public:
	Object(b2Body *body = NULL, std::string image = "") :
		body(body), image(image)
	{
	}

	~Object() {
	}

	b2Body *body;
	std::string image;
};

class Frame
{
public:
	Frame(int width, int height) {
		m_sdlSurface = SDL_CreateRGBSurface(0, width, height, 32,
			0x00ff0000,
			0x0000ff00,
			0x000000ff,
			0xff000000
		);

		cairo_surface_t *cairoSurface = cairo_image_surface_create_for_data((unsigned char*)m_sdlSurface->pixels, CAIRO_FORMAT_ARGB32, width, height, m_sdlSurface->pitch);
		SDL_assert(cairoSurface != NULL);
		SDL_assert(cairo_surface_status(cairoSurface) != CAIRO_STATUS_INVALID_STRIDE);
		m_cairoContext = cairo_create(cairoSurface);
		SDL_assert(cairo_status(m_cairoContext) == CAIRO_STATUS_SUCCESS);
		cairo_surface_destroy(cairoSurface);

		m_texture = NULL;
	}

	~Frame() {
		if (m_texture != NULL) {
			SDL_DestroyTexture(m_texture);
		}

		SDL_FreeSurface(m_sdlSurface);
		cairo_destroy(m_cairoContext);
	}

	void updateTexture(void) {
		SDL_UpdateTexture(m_texture, NULL, m_sdlSurface->pixels, m_sdlSurface->pitch);
	}

	SDL_Surface *surface(void) {
		return m_sdlSurface;
	}

	SDL_Texture *texture(SDL_Renderer *renderer) {
		if (m_texture == NULL) {
			m_texture = SDL_CreateTextureFromSurface(renderer, m_sdlSurface);
		}

		return m_texture;
	}

	cairo_t *cairoContext(void) {
		return m_cairoContext;
	}

protected:
	SDL_Surface *m_sdlSurface;
	SDL_Texture *m_texture;
	cairo_t *m_cairoContext;
};

typedef boost::shared_ptr<Frame> FramePtr;

class Animation
{
	public:
		Animation();
		virtual ~Animation();

		bool load(std::string jsonFile);
		void save(void);
		void pause(void);
		void resume(void);
		void reverse(void);
		void frameStep(int steps);
		int width(void);
		int height(void);
		void framerate(double framerate);
		double framerate(void);
		std::vector<FramePtr> &frames(void);
		FramePtr currentFrame(double currentTime);
		void blendFrames(void);
	protected:
	private:
		bool m_paused;
		bool m_reversed;
		int m_frameWidth;
		int m_frameHeight;
		double m_framerate;
		std::vector<FramePtr> m_frames;
		int m_frameIndex;
		Uint32 m_animationTimeStep;
		Uint32 m_nextAnimationFrame;
		b2World *m_world;
		std::vector<Object> m_objects;
		Object *m_light;
		std::map<std::string, cairo_pattern_t *> m_imagePatterns;

		boost::property_tree::ptree m_animationProperties;

		void blendFramesStriped(int threadIndex, int threadCount, std::vector<double> &frameWeights, std::vector<FramePtr> *output);
		void render(void);
		b2Body *spawnCrate(float x, float y, float density = 1.0f);
		b2Body *spawnBall(float x, float y, float density = 1.0f);
};

#endif // ANIMATION_HPP
