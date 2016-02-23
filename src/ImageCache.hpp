#ifndef IMAGECACHE_HPP
#define IMAGECACHE_HPP

#include <string>
#include <map>
#include <cairo/cairo.h>
#include <SDL2/SDL.h>
#include "Console.hpp"

class ImageCache
{
public:
	ImageCache();
	virtual ~ImageCache();

	cairo_surface_t *get(std::string filename);
protected:
private:
	std::map<std::string, cairo_surface_t *> m_images;
};

extern ImageCache g_imageCache;

#endif // IMAGECACHE_HPP
