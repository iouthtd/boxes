#include "Animation.hpp"

Animation::Animation() :
	m_world(NULL), m_light(NULL)
{
	m_paused = false;
	m_reversed = false;
	m_framerate = 320.0;
	m_frameIndex = 0;
	m_animationTimeStep = 0;
	m_nextAnimationFrame = 0;
}

Animation::~Animation()
{
	if (m_world != NULL) delete m_world;

	for (std::map<std::string, cairo_pattern_t *>::iterator it = m_imagePatterns.begin(); it != m_imagePatterns.end(); ++ it) {
		cairo_pattern_destroy(it->second);
	}
}

bool Animation::load(std::string jsonFile)
{
	m_frames.clear();

	using boost::property_tree::ptree;

	try {
		boost::property_tree::json_parser::read_json(jsonFile, m_animationProperties);
	} catch (boost::property_tree::ptree_bad_data e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ptree_bad_data", e.what(), NULL);
		return false;
	} catch (boost::property_tree::ptree_bad_path e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ptree_bad_path", e.what(), NULL);
		return false;
	} catch (boost::property_tree::ptree_error e) {
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "ptree_error", e.what(), NULL);
		return false;
	}

	m_frameWidth = m_animationProperties.get("width", 512);
	m_frameHeight = m_animationProperties.get("height", 512);
	m_framerate = m_animationProperties.get("framerate", 320.0);

	b2Vec2 gravity(m_animationProperties.get("gravityx", 0.0f), m_animationProperties.get("gravityy", 0.0f));
	if (m_world != NULL) delete m_world;
	m_world = new b2World(gravity);

 	// The b2World destructor frees b2Body objects automatically.
	m_objects.clear();
	m_light = NULL;

	b2BodyDef groundBodyDef;
	groundBodyDef.position.Set(0.0f, 10.0f);
	b2Body* groundBody = m_world->CreateBody(&groundBodyDef);
	b2PolygonShape groundBox;
	groundBox.SetAsBox(50.0f, 10.0f);
	groundBody->CreateFixture(&groundBox, 0.0f);

	boost::property_tree::ptree objectsTree = m_animationProperties.get_child("objects");
	boost::property_tree::ptree::const_iterator end = objectsTree.end();
	for (boost::property_tree::ptree::const_iterator it = objectsTree.begin(); it != end; ++ it) {
		boost::property_tree::ptree objectTree = it->second;
		b2Body *body = NULL;
		if (objectTree.get("type", "") == "box") {
			body = spawnCrate(objectTree.get("x", 0.0), objectTree.get("y", 0.0), objectTree.get("density", 1.0));
		}
		else if (objectTree.get("type", "") == "circle") {
			body = spawnBall(objectTree.get("x", 0.0), objectTree.get("y", 0.0), objectTree.get("density", 1.0));
		}

		if (body != NULL) {
			body->ApplyLinearImpulse(b2Vec2(objectTree.get("vx", 0.0) *
body->GetMass(), objectTree.get("vy", 0.0) * body->GetMass()), body->GetPosition(), true);

			std::string imageFilename = objectTree.get("image", "");
			if (m_imagePatterns.find(imageFilename) == m_imagePatterns.end()) {
				cairo_surface_t *surface = g_imageCache.get(imageFilename);
				cairo_pattern_t *pattern = cairo_pattern_create_for_surface(surface);
				cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
				cairo_matrix_t matrix;
				cairo_matrix_init_identity(&matrix);
				cairo_matrix_translate(&matrix, cairo_image_surface_get_width(surface) / 2.0, cairo_image_surface_get_height(surface) / 2.0);
				int pixelsPerUnit = 64;
				double objectSize = 2.0;
				cairo_matrix_scale(&matrix, 1.0 / objectSize, 1.0 / objectSize);
				cairo_matrix_scale(&matrix, pixelsPerUnit, pixelsPerUnit);
				cairo_pattern_set_matrix(pattern, &matrix);
				m_imagePatterns[imageFilename] = pattern;
			}

			m_objects.push_back(Object(body, imageFilename));

			if (objectTree.get("light", false) == true) {
				m_light = &m_objects.back();
			}
		}
	}

	render();

	return true;
}

void Animation::save(void)
{
	int frameIndex = 0;
	if (!m_reversed) {
		for (std::vector<FramePtr>::iterator it = m_frames.begin(); it != m_frames.end(); ++ it) {
			SDL_SaveBMP((*it)->surface(), (boost::format("output/frame%04d.bmp") % frameIndex).str().c_str());
			frameIndex ++;
		}
	}
	else
	{
		for (std::vector<FramePtr>::reverse_iterator it = m_frames.rbegin(); it != m_frames.rend(); ++ it) {
			SDL_SaveBMP((*it)->surface(), (boost::format("output/frame%04d.bmp") % frameIndex).str().c_str());
			frameIndex ++;
		}
	}
}

void Animation::pause(void)
{
	m_paused = !m_paused;
}

void Animation::resume(void)
{
	m_paused = false;
}

void Animation::reverse(void)
{
	m_reversed = !m_reversed;
}

void Animation::frameStep(int steps)
{
	if (m_frames.size() == 0) {
		m_frameIndex = 0;
		return;
	}

	m_frameIndex += steps;
	while (m_frameIndex < 0) m_frameIndex += m_frames.size();
	m_frameIndex %= m_frames.size();
}

int Animation::width(void)
{
	return m_frameWidth;
}

int Animation::height(void)
{
	return m_frameHeight;
}

void Animation::framerate(double framerate)
{
	m_framerate = framerate;
}

double Animation::framerate(void)
{
	return m_framerate;
}

std::vector<FramePtr> &Animation::frames(void)
{
	return m_frames;
}

FramePtr Animation::currentFrame(double currentTime)
{
	if (m_frames.empty()) return FramePtr();

	m_animationTimeStep = std::max(1, boost::math::iround(1000.0 / m_framerate));

	if (!m_paused) {
		Uint32 now = SDL_GetTicks();
		if (now >= m_nextAnimationFrame) {
			int frameSteps = 0;
			while (m_nextAnimationFrame < now) {
				frameSteps += 1;
				m_nextAnimationFrame += m_animationTimeStep;
			}

			if (!m_reversed) {
				frameStep(frameSteps);
			}
			else {
				frameStep(-frameSteps);
			}
		}
	}

	frameStep(0); // Make sure m_frameIndex is within range.
	return m_frames[m_frameIndex];
}

void Animation::blendFrames(void)
{
	if (m_frames.size() == 0) return;

	int nrofFramesToBlend = 16;

	// Repeat last frame until number of frames is divisible by nrofFramesToBlend.
	int remainder = m_frames.size() % nrofFramesToBlend;
	if (remainder > 0) {
		int framesToAdd = nrofFramesToBlend - remainder;
		FramePtr lastFrame = m_frames.back();
		for (int i = 0; i < framesToAdd; i ++) {
			m_frames.push_back(lastFrame);
		}
	}

	SDL_assert(m_frames.size() % nrofFramesToBlend == 0);

	std::vector<double> frameWeights;
	SDL_assert(nrofFramesToBlend == 16);
	for (int i = 0; i < 16; i ++) {
		frameWeights.push_back(std::max(0.0, sin(cml::constantsd::pi() / (double)(nrofFramesToBlend - 1) * (double)i)));
	}

	int threadCount = 4;
	boost::thread_group threads;
	std::vector<FramePtr> output(m_frames.size() / nrofFramesToBlend);
	for (int threadIndex = 0; threadIndex < threadCount; ++threadIndex) {
		threads.create_thread(boost::bind(&Animation::blendFramesStriped, this, threadIndex, threadCount, frameWeights, &output));
	}
	threads.join_all();

	SDL_assert(output[0].use_count() > 0);
	m_frames = output;
}

void Animation::blendFramesStriped(int threadIndex, int threadCount, std::vector<double> &frameWeights, std::vector<FramePtr> *output)
{
	double weightTotal = 0.0;
	for (std::vector<double>::iterator it = frameWeights.begin(); it != frameWeights.end(); ++ it) {
		weightTotal += (*it);
	}

	int nrofFramesToBlend = frameWeights.size();

	for (int i = threadIndex; i < (int)(*output).size(); i += threadCount) {
		std::vector<FramePtr>::iterator first = m_frames.begin() + (i * nrofFramesToBlend);
		std::vector<FramePtr>::iterator last = m_frames.begin() + (i * nrofFramesToBlend) + nrofFramesToBlend;

		FramePtr result(new Frame((*first)->surface()->w, (*first)->surface()->h));
		(*output)[i] = result;
		SDL_assert((*output)[i].use_count() > 0);
		SDL_Surface *surface = result->surface();

		for (int j = 0; j < (surface->w * surface->h); ++j) {
			double r = 0.0;
			double g = 0.0;
			double b = 0.0;
			double a = 0.0;
			int frameIndex = 0;
			for (std::vector<FramePtr>::iterator it = first; it != last; ++ it) {
				SDL_Surface *sdlFrame = (*it)->surface();
				Uint32 pixel = ((Uint32*)sdlFrame->pixels)[j];
				Uint8 pr,pg,pb;
				SDL_GetRGB(pixel, surface->format, &pr, &pg, &pb);
				r += (double)pr * frameWeights[frameIndex];
				g += (double)pg * frameWeights[frameIndex];
				b += (double)pb * frameWeights[frameIndex];
				frameIndex ++;
			}

			r = r / weightTotal;
			g = g / weightTotal;
			b = b / weightTotal;
			a = 255.0;
			((Uint32*)surface->pixels)[j] = SDL_MapRGBA(surface->format, (Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a);
		}
	}
}


void Animation::render(void)
{
	float32 physicsTimeStep = 1.0f / m_animationProperties.get("framerate", 320.0);
	int32 velocityIterations = 8;
	int32 positionIterations = 3;

	cairo_pattern_t *backgroundPattern = NULL;
	std::string backgroundImage = m_animationProperties.get("background", "");
	if (backgroundImage != "") {
		cairo_surface_t *backgroundSurface = NULL;
		backgroundSurface = g_imageCache.get(backgroundImage);
		backgroundPattern = cairo_pattern_create_for_surface(backgroundSurface);
		cairo_pattern_set_extend(backgroundPattern, CAIRO_EXTEND_REPEAT);
	}
	else {
		try {
			boost::property_tree::ptree colorValues = m_animationProperties.get_child("backgroundcolor");
			boost::property_tree::ptree::const_iterator it = colorValues.begin();
			double red = (it->second).get_value(0.0);
			++it;
			double green = (it->second).get_value(0.0);
			++it;
			double blue = (it->second).get_value(0.0);
			backgroundPattern = cairo_pattern_create_rgb(red / 255.0, green / 255.0, blue / 255.0);
		}
		catch (...) {
			backgroundPattern = cairo_pattern_create_rgb(0.0, 0.3, 0.0);
		}
	}

	int pixelsPerUnit = 64;
	float32 time = 0.0f;
	cairo_matrix_t view;
	cairo_matrix_init_identity(&view);
	cairo_matrix_translate(&view, m_frameWidth / 2.0, m_frameHeight / 2.0);
	cairo_matrix_scale(&view, pixelsPerUnit, pixelsPerUnit);
	cairo_matrix_scale(&view, m_animationProperties.get("zoom", 1.0), m_animationProperties.get("zoom", 1.0));
	cairo_matrix_translate(&view, m_animationProperties.get("camerax", 0.0), m_animationProperties.get("cameray", 0.0));
	int frameCount = int(m_animationProperties.get("framerate", 320.0) * m_animationProperties.get("animationlength", 320.0));
	for (int i = 0; i < frameCount; i++) {
		m_world->Step(physicsTimeStep, velocityIterations, positionIterations);

		m_frames.push_back(FramePtr(new Frame(m_frameWidth, m_frameHeight)));
		FramePtr frame = m_frames.back();
		cairo_t *cr = frame->cairoContext();

		cairo_identity_matrix(cr);
		cairo_set_source(cr, backgroundPattern);
		cairo_paint(cr);

		std::vector< std::pair<cml::vector2d, cml::vector2d> > sides;
		for (std::vector<Object>::iterator it = m_objects.begin(); it != m_objects.end(); ++ it) {
			Object &object = *it;
			b2Body *body = (*it).body;
			b2Vec2 position = body->GetPosition();
			float32 angle = body->GetAngle();
			std::string imageFilename = (*it).image;
			cairo_pattern_t *pattern = m_imagePatterns[imageFilename];
			const double boxSize = 2.0;

			cairo_set_matrix(cr, &view);
			cairo_translate(cr, position.x, position.y);
			cairo_rotate(cr, angle);

			double imageSize = 1.0;
			if (pattern != NULL) {
				cairo_set_source(cr, pattern);
				cairo_surface_t *surface = NULL;
				cairo_pattern_get_surface(pattern, &surface);
				imageSize = (double)cairo_image_surface_get_width(surface) / pixelsPerUnit;
			}

			cairo_rectangle(cr, -(boxSize / 2.0) * imageSize, -(boxSize / 2.0) * imageSize, boxSize * imageSize, boxSize * imageSize);
			cairo_fill(cr);

			// Information about box sides, for use with drawing shadows.
			cml::vector2d pos(position.x, position.y);
			cml::vector2d ex = cml::vector2d(std::cos(-angle), -std::sin(-angle));
			cml::vector2d ey = cml::vector2d(std::sin(angle), -std::cos(angle));

			if (&object != m_light) {
				sides.push_back(std::pair<cml::vector2d, cml::vector2d>(pos - ex - ey, pos + ex - ey));
				sides.push_back(std::pair<cml::vector2d, cml::vector2d>(pos - ex + ey, pos - ex - ey));
				sides.push_back(std::pair<cml::vector2d, cml::vector2d>(pos + ex + ey, pos - ex + ey));
				sides.push_back(std::pair<cml::vector2d, cml::vector2d>(pos + ex - ey, pos + ex + ey));
			}
		}

		if (m_light != NULL) {
			cairo_surface_t *shadowSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, m_frameWidth, m_frameHeight);
			cairo_t *shadows = cairo_create(shadowSurface);
			cairo_set_matrix(shadows, &view);
			cairo_set_matrix(cr, &view);
			SDL_assert(m_light->body != NULL);
			cml::vector2d lightPosition(m_light->body->GetPosition().x, m_light->body->GetPosition().y);
			for (int i = 0; i < (int)sides.size(); i ++) {
				cml::vector2d objectVertexA = sides[i].first;
				cml::vector2d objectVertexB = sides[i].second;

				cml::vector2d line(objectVertexA - objectVertexB);
				cml::vector2d normal(line[1], -line[0]);

				// Discard sides facing away from the light.
				if (dot(normal, objectVertexA - lightPosition) >= 0) continue;

				double shadowLength = 128.0; // Sufficiently large to go off the screen.
				cml::vector2d shadowVertexA = objectVertexA + (objectVertexA - lightPosition).normalize() * shadowLength;
				cml::vector2d shadowVertexB = objectVertexB + (objectVertexB - lightPosition).normalize() * shadowLength;

				cairo_move_to(shadows, objectVertexA[0], objectVertexA[1]);
				cairo_line_to(shadows, shadowVertexA[0], shadowVertexA[1]);
				cairo_line_to(shadows, shadowVertexB[0], shadowVertexB[1]);
				cairo_line_to(shadows, objectVertexB[0], objectVertexB[1]);
			}

			cairo_set_source_rgba(shadows, 0.0, 0.0, 0.0, 1.0);
			cairo_fill(shadows);
			cairo_pattern_t *radialPattern = cairo_pattern_create_radial(lightPosition[0], lightPosition[1], 0.0, lightPosition[0], lightPosition[1], 16.0);
			cairo_pattern_add_color_stop_rgba(radialPattern, 0.0, 0.0, 0.0, 0.0, 0.0);
			cairo_pattern_add_color_stop_rgba(radialPattern, 1.0, 0.0, 0.0, 0.0, 1.0);
			cairo_set_source(shadows, radialPattern);
			cairo_paint(shadows);
			cairo_pattern_destroy(radialPattern);
			cairo_destroy(shadows);

			cairo_identity_matrix(cr);
			cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.5);
			cairo_mask_surface(cr, shadowSurface, 0.0, 0.0);
			cairo_surface_destroy(shadowSurface);
		}

		frame->updateTexture();
		time += physicsTimeStep;
	}

	cairo_pattern_destroy(backgroundPattern);
}

b2Body *Animation::spawnCrate(float x, float y, float density)
{
	SDL_assert(m_world != NULL);

	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(x, y);

	b2PolygonShape shape;
	shape.SetAsBox(1.0f, 1.0f);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = density;
	fixtureDef.friction = 0.3f;

	b2Body* body = m_world->CreateBody(&bodyDef);
	body->CreateFixture(&fixtureDef);
	return body;
}

b2Body *Animation::spawnBall(float x, float y, float density)
{
	SDL_assert(m_world != NULL);

	b2BodyDef bodyDef;
	bodyDef.type = b2_dynamicBody;
	bodyDef.position.Set(x, y);

	b2CircleShape shape;
	shape.m_radius = 1.0f;

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shape;
	fixtureDef.density = density;
	fixtureDef.friction = 0.3f;

	b2Body* body = m_world->CreateBody(&bodyDef);
	body->CreateFixture(&fixtureDef);
	return body;
}
