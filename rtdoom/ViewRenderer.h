#pragma once

#include "Renderer.h"
#include "WADFile.h"
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
			VisibleSegment(const Segment& segment) : mapSegment{ segment } {}

			const Segment& mapSegment;
			Vector normalVector;
			int startX;
			int endX;
			float startAngle;
			float endAngle;
			float normalOffset;
		};

		struct TextureContext
		{
			std::string textureName;
			float yScale;
			float xPos;
			int yPegging;
			int yOffset;
		};

		const float s_minDistance = 1.0f;
		const float s_lightnessFactor = 2500.0f;
		const float s_skyHeight = NAN;
		const int s_wallColor = 1;
		const int s_planeColor = 5;

		void DrawMapSegment(const Segment& segment) const;
		void DrawMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const;
		void DrawPlanes() const;

		void RenderColumnSpan(int x, const Frame::Span& outerSpan, const TextureContext& outerTexture, float lightness, const VisibleSegment& visibleSegment) const;
		void TextureWall(int x, const Frame::Span& span, const TextureContext& textureContext, float lightness) const;
		void TexturePlane(const Frame::Plane& plane) const;

		Angle GetViewAngle(int x, const VisibleSegment& visibleSegment) const;
		float GetLightness(float distance, const Segment* segment = nullptr) const;

		FrameBuffer* m_frameBuffer;
		const WADFile& m_wadFile;
		std::unique_ptr<Projection> m_projection;
		std::unique_ptr<Frame> m_frame;
		RenderingMode m_renderingMode;
		bool m_stepFrame;

	public:
		ViewRenderer(const GameState& gameState, const WADFile& wadFile);
		~ViewRenderer();

		virtual void RenderFrame(FrameBuffer& frameBuffer) override;
		void StepFrame();
		Frame* GetLastFrame() const;
		void SetMode(RenderingMode renderingMode);
	};
}
