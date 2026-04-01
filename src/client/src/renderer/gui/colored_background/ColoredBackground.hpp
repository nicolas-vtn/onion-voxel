#pragma once

#include <glm/glm.hpp>

#include <renderer/OpenGL.hpp>

#include <renderer/shader/shader.hpp>

namespace onion::voxel
{
	class ColoredBackground
	{
		// ----- Structs and Enums -----
	  public:
		struct Options
		{
			// Position of the background in pixels. The position is interpreted as the center of the background.
			glm::ivec2 Position{0, 0};
			// Size of the background in pixels.
			glm::ivec2 Size{100, 100};
			// Color of the background. The color is specified as a vec4. Red, Green, Blue, and Alpha components should be in the range [0, 1]
			glm::vec4 Color{1, 1, 1, 1};
			// Offset of the background in the Z direction. In range [-1, 1], where 1 is the closest to the camera and -1 is the farthest from the camera.
			float ZOffset = 0.f;
			// Rotation of the background in degrees. Rotation is applied around the center of the background, and is in the clockwise direction.
			float RotationDegrees = 0.f;
		};

		// ----- Public API -----
	  public:
		static void Render(const Options& options);

		static void StaticShutdown();

		static void SetProjectionMatrix(const glm::mat4& projection);

		// ----- Private Structs and Enums -----
	  private:
		struct Vertex
		{
			// Position in screen space (x, y) and depth (z) - Top-left corner is (0, 0)
			glm::vec3 Position;
		};

		// ----- Private Members -----
	  private:
		static inline bool s_IsInitialized;

		static inline GLuint m_VAO_Background{0};
		static inline GLuint m_VBO_Background{0};

		static Shader s_Shader;
		static inline glm::mat4 s_ProjectionMatrix{1.0f};

		// ----- Private Methods -----
	  private:
		static void Initialize();
	};
} // namespace onion::voxel
