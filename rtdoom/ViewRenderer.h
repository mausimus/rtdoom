#pragma once

#include "Renderer.h"
#include "WADFile.h"
#include "Projection.h"
#include "Frame.h"
#include "Painter.h"

namespace rtdoom
{
	class ViewRenderer : public Renderer
	{
	public:
		enum class RenderingMode
		{
			Wireframe,
			Solid,
			Textured
		};

	protected:
		struct VisibleSegment
		{
			VisibleSegment(const Segment& segment) : mapSegment{ segment } {}

			const Segment& mapSegment;
			Vector normalVector;
			int startX;
			int endX;
			float startAngle;
			float endAngle;
			float normalOffset;
		};

		const float s_skyHeight = NAN;

		void Initialize(FrameBuffer& frameBuffer);
		void RenderMapSegment(const Segment& segment) const;
		void RenderMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const;
		void RenderPlanes() const;
		void RenderSprites() const;
		void RenderOverlay() const;
		Angle GetViewAngle(int x, const VisibleSegment& visibleSegment) const;

		FrameBuffer* m_frameBuffer;
		const WADFile& m_wadFile;
		std::unique_ptr<Projection> m_projection;
		std::unique_ptr<Frame> m_frame;
		std::unique_ptr<Painter> m_painter;
		RenderingMode m_renderingMode;

	public:
		ViewRenderer(const GameState& gameState, const WADFile& wadFile);
		~ViewRenderer();

		virtual void RenderFrame(FrameBuffer& frameBuffer) override;
		Frame* GetLastFrame() const;
		void SetMode(RenderingMode renderingMode);
	};
}
