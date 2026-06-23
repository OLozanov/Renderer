#include <SDL3/SDL.h>
#include "App.h"

#include <string>
#include <memory>

namespace
{
	constexpr int width = 1280;
	constexpr int height = 720;
}

void testDraw(uint32_t* data, int pitch)
{
	for (size_t k = 0; k < height; k++)
	{
		for (size_t i = 0; i < width; i++)
		{
			data[width * k + i] = 0x0000FFFF;
		}
	}
}

int main(int argc, char* args[])
{
	// Initialize SDL.
	if (SDL_Init(SDL_INIT_VIDEO) < 0) return 1;

	SDL_Window* window = SDL_CreateWindow("Renderer", width, height, 0);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);

	SDL_SetWindowRelativeMouseMode(window, true);

	SDL_Texture* frameBuffer = SDL_CreateTexture(renderer,
												 SDL_PIXELFORMAT_RGBA8888,
												 SDL_TEXTUREACCESS_STREAMING,
												 width,
												 height);
	bool run = true;

	std::string scene = argc > 1 ? args[1] : "courtyard";

	App app(frameBuffer, scene);
	app.resize(width, height);

	float t = 1.0f;
	float dt = 1.0f / 60.0f;
	uint64_t curtime = SDL_GetTicks();

	while (run)
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_EVENT_QUIT) run = false;
			else
				if (event.type == SDL_EVENT_KEY_UP && event.key.key == SDLK_ESCAPE) run = false;

			app.input(event);
		}

		static constexpr float w = 1.0f / 5.0f;

		uint64_t time = SDL_GetTicks();
		float delta = (time - curtime) / 1000.0;
		dt = delta * w + dt * (1.0f - w);
		curtime = time;
		
		t += delta;

		if (t >= 1.0f)
		{
			std::string caption("Renderer FPS: ");
			caption += std::to_string(uint32_t(1.0f / dt));

			SDL_SetWindowTitle(window, caption.c_str());

			t = 0.0f;
		}

		app.update(dt);
		app.display();

		SDL_RenderTexture(renderer, frameBuffer, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	//SDL_DestroyTexture(img);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	return 0;
}
