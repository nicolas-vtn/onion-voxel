#pragma once

#include <glad/glad.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

#include "../Variables.hpp"
#include "../shader/shader.hpp"

namespace onion::voxel
{
	// Static Class
	class DebugDraws
	{
		// ----- Constructor / Destructor -----
	  public:
		DebugDraws() = delete;
		~DebugDraws() = delete;

		// ----- Public API -----
	  public:
		/// @brief Sets the view-projection matrix to be used for drawing debug lines in world space. This should be called at the beginning of each frame with the current camera's view-projection matrix.
		static void SetViewProjMatrix(const glm::mat4& viewProjMatrix);

		/// @brief Draws a line in world space.
		/// @param start The starting point of the line in world coordinates.
		/// @param end The ending point of the line in world coordinates.
		/// @param color The color of the line.
		/// @param widthPx The width of the line in pixels.
		/// @param topMost Whether the line should be drawn on top of all other objects.
		static void DrawWorldLine(
			const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, int widthPx = 1, bool topMost = true);

		/// @brief Draws a box in world space defined by its minimum and maximum corners.
		/// @param minCorner The minimum corner of the box in world coordinates.
		/// @param maxCorner The maximum corner of the box in world coordinates.
		/// @param color The color of the box.
		/// @param widthPx The width of the box lines in pixels.
		/// @param topMost Whether the box should be drawn on top of all other objects.
		static void DrawWorldBoxMinMax(const glm::vec3& minCorner,
									   const glm::vec3& maxCorner,
									   const glm::vec4& color,
									   int widthPx = 1,
									   bool topMost = true);

		/// @brief Draws a block outline in world space at the specified block position. The block is assumed to be axis-aligned and of size 1 unit.
		/// @param blockPosition The position of the block in world coordinates.
		/// @param color The color of the block outline.
		/// @param widthPx The width of the block outline lines in pixels.
		/// @param topMost Whether the block outline should be drawn on top of all other objects.
		static void
		DrawBlockOutline(const glm::vec3& blockPosition, const glm::vec4& color, int widthPx = 1, bool topMost = true);

		/// @brief Draws a box in world space defined by its center and size.
		/// @param center The center of the box in world coordinates.
		/// @param size The size of the box in world coordinates.
		/// @param color The color of the box.
		/// @param widthPx The width of the box lines in pixels.
		/// @param topMost Whether the box should be drawn on top of all other objects.
		static void DrawWorldBoxCenterSize(const glm::vec3& center,
										   const glm::vec3& size,
										   const glm::vec4& color,
										   int widthPx = 1,
										   bool topMost = true);

		/// @brief Draws a line in screen space (normalized device coordinates (0.0 is left/bottom, 1.0 is right/top)).
		/// @param start The starting point of the line in normalized screen coordinates (0.0 is left/bottom, 1.0 is right/top).
		/// @param end The ending point of the line in normalized screen coordinates (0.0 is left/bottom, 1.0 is right/top).
		/// @param color The color of the line.
		/// @param widthPx The width of the line in pixels.
		static void
		DrawScreenLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, int widthPx = 1);

		/// @brief Draws a box in screen space defined by its minimum and maximum corners (normalized device coordinates (0.0 is left/bottom, 1.0 is right/top)).
		/// @param minCorner The minimum corner of the box in normalized screen coordinates (0.0 is left/bottom, 1.0 is right/top).
		/// @param maxCorner The maximum corner of the box in normalized screen coordinates (0.0 is left/bottom, 1.0 is right/top).
		/// @param color The color of the box.
		/// @param widthPx The width of the box lines in pixels.
		static void DrawScreenBoxMinMax(const glm::vec2& minCorner,
										const glm::vec2& maxCorner,
										const glm::vec4& color,
										int widthPx = 1);

		/// @brief Draws a box in screen space defined by its center and size (normalized device coordinates (0.0 is left/bottom, 1.0 is right/top)).
		/// @param center The center of the box in normalized screen coordinates (0.0 is left/bottom, 1.0 is right/top).
		/// @param size The size of the box in normalized screen coordinates (0.0 is left/bottom, 1.0 is right/top).
		/// @param color The color of the box.
		/// @param widthPx The width of the box lines in pixels.
		static void DrawScreenBoxCenterSize(const glm::vec2& center,
											const glm::vec2& size,
											const glm::vec4& color,
											int widthPx = 1);

		// ----- Private Members -----
	  private:
		static inline glm::mat4 s_ViewProjMatrix;

		static inline Shader s_LineShader{GetAssetsPath() / "shaders" / "line.vert",
										  GetAssetsPath() / "shaders" / "line.frag"};
		static inline GLuint s_VAOLine = 0;
		static inline GLuint s_VBOLine = 0;

		static void Initialize();
		static void Cleanup();
	};
} // namespace onion::voxel
