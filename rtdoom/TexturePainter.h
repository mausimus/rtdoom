#pragma once

#include "Painter.h"

namespace rtdoom
{
	class TexturePainter : public Painter
	{
	protected:
		const Thing& m_pov;
		const Projection& m_projection;
		const WADFile& m_wadFile;

		static std::list<Frame::Span> MergeSpans(const std::vector<Frame::Span>& spans);

	public:
		void PaintWall(int x, const Frame::Span& span, const Frame::PainterContext& textureContext) const override;
		void PaintSprite(int x, int sy, std::vector<bool> occlusion, const Frame::PainterContext& textureContext) const override;
		void PaintPlane(const Frame::Plane& plane) const override;

		TexturePainter(FrameBuffer& frameBuffer, const Thing& pov, const Projection& projection, const WADFile& wadFile);
		~TexturePainter();
	};
}