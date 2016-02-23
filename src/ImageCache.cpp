#include "ImageCache.hpp"

ImageCache g_imageCache;

const std::string IMAGE_DIR = std::string("img/");

ImageCache::ImageCache()
{
}

ImageCache::~ImageCache()
{
	for (std::map<std::string, cairo_surface_t *>::iterator it = m_images.begin(); it != m_images.end(); ++ it) {
		cairo_surface_destroy(it->second);
	}
}

cairo_surface_t *ImageCache::get(std::string filename)
{
	if (m_images.find(filename) != m_images.end()) {
		return m_images[filename];
	}
	else {
		std::string filepath = IMAGE_DIR + filename;
		cairo_surface_t *image = cairo_image_surface_create_from_png(filepath.c_str());
		cairo_status_t status = cairo_surface_status(image);
		if (status != CAIRO_STATUS_SUCCESS) {
			g_console.print(boost::format("Error loading '%s': %s") % filepath %
				cairo_status_to_string(status));
			image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 64, 64);
			SDL_assert(cairo_surface_status(image) == CAIRO_STATUS_SUCCESS);

			// Placeholder checkers image.
			cairo_t *cr = cairo_create(image);
			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
			cairo_rectangle(cr, 0, 0, 64, 64);
			cairo_fill(cr);
			cairo_set_source_rgba(cr, 1.0, 0.0, 1.0, 1.0);
			cairo_rectangle(cr, 32, 0, 32, 32);
			cairo_rectangle(cr, 0, 32, 32, 32);
			cairo_fill(cr);
			cairo_destroy(cr);
		}

		m_images[filename] = image;
		return image;
	}

}
