#include "DebugDraws.hpp"

namespace onion::voxel
{
	void DebugDraws::SetViewProjMatrix(const glm::mat4& viewProjMatrix)
	{
		s_ViewProjMatrix = viewProjMatrix;

		s_LineShader.Use();
		s_LineShader.setMat4("u_ViewProj", s_ViewProjMatrix);
	}

	void DebugDraws::StaticUnload()
	{
		glDeleteVertexArrays(1, &s_VAOLine);
		glDeleteBuffers(1, &s_VBOLine);
		s_LineShader.Delete();
	}

	void DebugDraws::DrawWorldLine(
		const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, int widthPx, bool topMost)
	{
		Initialize();

		glm::vec3 vertices[2] = {start, end};

		glBindVertexArray(s_VAOLine);

		glBindBuffer(GL_ARRAY_BUFFER, s_VBOLine);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

		s_LineShader.Use();

		s_LineShader.setMat4("u_ViewProj", s_ViewProjMatrix);
		s_LineShader.setBool("u_NormalizedCoordonates", false);
		s_LineShader.setVec4("u_Color", color);
		s_LineShader.setBool("u_TopMost", topMost);

		glLineWidth((float) widthPx);

		glDrawArrays(GL_LINES, 0, 2);

		glBindVertexArray(0);
	}

	void DebugDraws::DrawWorldBoxMinMax(
		const glm::vec3& minCorner, const glm::vec3& maxCorner, const glm::vec4& color, int widthPx, bool topMost)
	{
		glm::vec3 v0 = {minCorner.x, minCorner.y, minCorner.z};
		glm::vec3 v1 = {maxCorner.x, minCorner.y, minCorner.z};
		glm::vec3 v2 = {maxCorner.x, maxCorner.y, minCorner.z};
		glm::vec3 v3 = {minCorner.x, maxCorner.y, minCorner.z};

		glm::vec3 v4 = {minCorner.x, minCorner.y, maxCorner.z};
		glm::vec3 v5 = {maxCorner.x, minCorner.y, maxCorner.z};
		glm::vec3 v6 = {maxCorner.x, maxCorner.y, maxCorner.z};
		glm::vec3 v7 = {minCorner.x, maxCorner.y, maxCorner.z};

		// bottom
		DrawWorldLine(v0, v1, color, widthPx, topMost);
		DrawWorldLine(v1, v2, color, widthPx, topMost);
		DrawWorldLine(v2, v3, color, widthPx, topMost);
		DrawWorldLine(v3, v0, color, widthPx, topMost);

		// top
		DrawWorldLine(v4, v5, color, widthPx, topMost);
		DrawWorldLine(v5, v6, color, widthPx, topMost);
		DrawWorldLine(v6, v7, color, widthPx, topMost);
		DrawWorldLine(v7, v4, color, widthPx, topMost);

		// verticals
		DrawWorldLine(v0, v4, color, widthPx, topMost);
		DrawWorldLine(v1, v5, color, widthPx, topMost);
		DrawWorldLine(v2, v6, color, widthPx, topMost);
		DrawWorldLine(v3, v7, color, widthPx, topMost);
	}

	void DebugDraws::DrawBlockOutline(const glm::vec3& blockPosition, const glm::vec4& color, int widthPx, bool topMost)
	{
		glm::vec3 minCorner = blockPosition;
		glm::vec3 maxCorner = blockPosition + glm::vec3(1.0f);

		DrawWorldBoxMinMax(minCorner, maxCorner, color, widthPx, topMost);
	}

	void DebugDraws::DrawWorldBoxCenterSize(
		const glm::vec3& center, const glm::vec3& size, const glm::vec4& color, int widthPx, bool topMost)
	{
		glm::vec3 half = size * 0.5f;

		glm::vec3 minCorner = center - half;
		glm::vec3 maxCorner = center + half;

		DrawWorldBoxMinMax(minCorner, maxCorner, color, widthPx, topMost);
	}

	void DebugDraws::DrawScreenLine_Normalized(const glm::vec2& start,
											   const glm::vec2& end,
											   const glm::vec4& color,
											   int widthPx)
	{
		Initialize();

		glm::vec3 vertices[2] = {glm::vec3(start, 0.0f), glm::vec3(end, 0.0f)};

		glBindVertexArray(s_VAOLine);

		glBindBuffer(GL_ARRAY_BUFFER, s_VBOLine);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

		s_LineShader.Use();

		s_LineShader.setBool("u_NormalizedCoordonates", true);
		s_LineShader.setVec4("u_Color", color);
		s_LineShader.setBool("u_TopMost", true);

		glLineWidth((float) widthPx);

		glDrawArrays(GL_LINES, 0, 2);

		glBindVertexArray(0);
	}

	void DebugDraws::DrawScreenBoxMinMax_Normalized(const glm::vec2& minCorner,
													const glm::vec2& maxCorner,
													const glm::vec4& color,
													int widthPx)
	{
		glm::vec2 v0 = {minCorner.x, minCorner.y};
		glm::vec2 v1 = {maxCorner.x, minCorner.y};
		glm::vec2 v2 = {maxCorner.x, maxCorner.y};
		glm::vec2 v3 = {minCorner.x, maxCorner.y};

		DrawScreenLine_Normalized(v0, v1, color, widthPx);
		DrawScreenLine_Normalized(v1, v2, color, widthPx);
		DrawScreenLine_Normalized(v2, v3, color, widthPx);
		DrawScreenLine_Normalized(v3, v0, color, widthPx);
	}

	void DebugDraws::DrawScreenBoxCenterSize_Normalized(const glm::vec2& center,
														const glm::vec2& size,
														const glm::vec4& color,
														int widthPx)
	{
		glm::vec2 half = size * 0.5f;

		glm::vec2 minCorner = center - half;
		glm::vec2 maxCorner = center + half;

		DrawScreenBoxMinMax_Normalized(minCorner, maxCorner, color, widthPx);
	}

	void DebugDraws::DrawScreenLine_Pixels(const glm::ivec2& start,
										   const glm::ivec2& end,
										   const glm::vec4& color,
										   int widthPx)
	{
		glm::vec2 startNorm = PixelsToNormalized(glm::vec2(start));
		glm::vec2 endNorm = PixelsToNormalized(glm::vec2(end));
		DrawScreenLine_Normalized(startNorm, endNorm, color, widthPx);
	}

	void DebugDraws::DrawScreenBoxMinMax_Pixels(const glm::vec2& minCorner,
												const glm::vec2& maxCorner,
												const glm::vec4& color,
												int widthPx)
	{
		glm::vec2 minNorm = PixelsToNormalized(minCorner);
		glm::vec2 maxNorm = PixelsToNormalized(maxCorner);
		DrawScreenBoxMinMax_Normalized(minNorm, maxNorm, color, widthPx);
	}

	void DebugDraws::DrawScreenBoxCenterSize_Pixels(const glm::vec2& center,
													const glm::vec2& size,
													const glm::vec4& color,
													int widthPx)
	{
		glm::vec2 centerNorm = PixelsToNormalized(center);
		glm::vec2 sizeNorm = PixelsToNormalized(size);
		DrawScreenBoxCenterSize_Normalized(centerNorm, sizeNorm, color, widthPx);
	}

	glm::vec2 DebugDraws::PixelsToNormalized(const glm::vec2& pixels)
	{
		if (ScreenHeight == 0 || ScreenWidth == 0)
		{
			return glm::vec2(0.0f);
		}

		const float x = pixels.x / static_cast<float>(ScreenWidth);
		// Invert Y because pixel coordinates have origin at top-left
		const float y = 1.0f - (pixels.y / static_cast<float>(ScreenHeight));

		return glm::vec2(x, y);
	}

	void DebugDraws::Initialize()
	{
		if (s_VAOLine != 0)
			return;

		glGenVertexArrays(1, &s_VAOLine);
		glGenBuffers(1, &s_VBOLine);

		glBindVertexArray(s_VAOLine);

		glBindBuffer(GL_ARRAY_BUFFER, s_VBOLine);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * 2, nullptr, GL_DYNAMIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*) 0);

		glBindVertexArray(0);
	}

	void DebugDraws::Cleanup()
	{
		if (s_VBOLine)
			glDeleteBuffers(1, &s_VBOLine);

		if (s_VAOLine)
			glDeleteVertexArrays(1, &s_VAOLine);

		s_VBOLine = 0;
		s_VAOLine = 0;
	}
} // namespace onion::voxel
