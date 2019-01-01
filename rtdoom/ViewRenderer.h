#pragma once

#include "Renderer.h"
#include "Projection.h"
#include "Frame.h"

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
			VisibleSegment(const Segment& segment) : segment{ segment } {}

			const Segment& segment;
			Vector normalVector;
			int startX;
			int endX;
			float startAngle;
			float endAngle;
		};

		const float s_minDistance = 1.0f;
		const float s_lightnessFactor = 2000.0f;
		const float s_skyHeight = NAN;
		const int s_wallColor = 1;
		const int s_planeColor = 5;

		void DrawMapSegment(const Segment& segment) const;
		void DrawMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const;
		void DrawPlanes() const;

		void RenderColumnOuterSpan(int x, const Frame::Span& outerSpan, float lightness, const VisibleSegment& visibleSegment) const;
		void RenderColumnInnerSpan(int x, const Frame::Span& innerSpan, bool isSky, float lightness, const Frame::Span& outerSpan, const VisibleSegment& visibleSegment) const;

		Angle GetViewAngle(int x, const VisibleSegment& visibleSegment) const;
		float GetLightness(float distance, const Segment* segment = nullptr) const;

		FrameBuffer* m_frameBuffer;
		std::unique_ptr<Projection> m_projection;
		std::unique_ptr<Frame> m_frame;
		RenderingMode m_renderingMode;

	public:
		ViewRenderer(const GameState& gameState);
		~ViewRenderer();

		virtual void RenderFrame(FrameBuffer& frameBuffer) override;
		Frame* GetLastFrame() const;
		void SetMode(RenderingMode renderingMode);
	};
}
