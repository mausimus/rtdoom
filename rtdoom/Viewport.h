#pragma once

#include <SDL.h>
#include <SDL_render.h>

#include "Renderer.h"

namespace rtdoom
{
	class Viewport
	{
	protected:
		int m_width;
		int m_height;
		bool m_fillTarget;

		SDL_Renderer* m_sdlRenderer;
		SDL_Texture* m_screenTexture;
		Renderer& m_renderer;

		const Palette& m_palette;
		std::unique_ptr<SDL_Rect> m_targetRect;
		std::unique_ptr<FrameBuffer> m_frameBuffer;

		void Initialize();
		void Uninitialize();

	public:
		Viewport(SDL_Renderer* sdlRenderer, Renderer& renderer, int width, int height, const Palette& palette, bool fillTarget);
		void Resize(int width, int height);
		void Draw();
		void DrawSteps();
		~Viewport();
	};
}