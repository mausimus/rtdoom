#pragma once

#include "Frame.h"
#include "Projection.h"
#include "GameState.h"

namespace rtdoom
{
	struct TextureContext
	{
		std::string textureName;
		float yScale;
		float xPos;
		int yPegging;
		int yOffset;
		bool isEdge;
		float lightness;
	};

	class Painter
	{
	protected:
		FrameBuffer& m_frameBuffer;

	public:
		virtual void PaintWall(int x, const Frame::Span& span, const TextureContext& textureContext) const = 0;
		virtual void PaintPlane(const Frame::Plane& plane) const = 0;

		Painter(FrameBuffer& frameBuffer);
		virtual ~Painter();
	};
}