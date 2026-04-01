#include "ColoredBackground.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <shared/utils/Utils.hpp>

namespace onion::voxel
{
	Shader ColoredBackground::s_Shader(Utils::GetExecutableDirectory() / "assets" / "shaders" / "rectangle.vert",
									   Utils::GetExecutableDirectory() / "assets" / "shaders" / "rectangle.frag");

	void ColoredBackground::Render(const Options& options)
	{
		if (!s_IsInitialized)
			Initialize();

		// --- Rotation Matrix ---
		glm::mat4 model(1.0f);

		if (options.RotationDegrees != 0.0f)
		{
			model = glm::translate(model, glm::vec3(options.Position, 0.0f));
			model = glm::rotate(model, glm::radians(options.RotationDegrees), glm::vec3(0, 0, 1));
			model = glm::translate(model, glm::vec3(-options.Position, 0.0f));
		}

		// --- Render Background ---
		if (options.Color.a > 0.0f)
		{
			const float& zOffset = options.ZOffset;
			glm::ivec2 topLeftCorner{options.Position.x - options.Size.x / 2, options.Position.y - options.Size.y / 2};
			glm::ivec2 bottomRightCorner{options.Position.x + options.Size.x / 2,
										 options.Position.y + options.Size.y / 2};

			// Build background vertices
			std::vector<Vertex> vertices = {{{topLeftCorner.x, topLeftCorner.y, zOffset}},
											{{bottomRightCorner.x, topLeftCorner.y, zOffset}},
											{{bottomRightCorner.x, bottomRightCorner.y, zOffset}},
											{{topLeftCorner.x, topLeftCorner.y, zOffset}},
											{{bottomRightCorner.x, bottomRightCorner.y, zOffset}},
											{{topLeftCorner.x, bottomRightCorner.y, zOffset}}};
			// Upload background vertices
			glBindVertexArray(m_VAO_Background);
			glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Background);

			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

			// Set Uniforms
			s_Shader.Use();
			s_Shader.setMat4("uModel", model);
			s_Shader.setMat4("uProjection", s_ProjectionMatrix);
			s_Shader.setVec4("uColor", options.Color);

			// Draw background
			glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));

			glBindVertexArray(0);
		}
	}

	void ColoredBackground::StaticShutdown()
	{
		if (m_VAO_Background)
			glDeleteVertexArrays(1, &m_VAO_Background);

		if (m_VBO_Background)
			glDeleteBuffers(1, &m_VBO_Background);

		m_VAO_Background = 0;
		m_VBO_Background = 0;

		s_Shader.Delete();
	}

	void ColoredBackground::SetProjectionMatrix(const glm::mat4& projection)
	{
		s_ProjectionMatrix = projection;
	}

	void ColoredBackground::Initialize()
	{
		// Generate Buffers
		glGenVertexArrays(1, &m_VAO_Background);
		glGenBuffers(1, &m_VBO_Background);

		// Setup VAO and VBO for background
		glBindVertexArray(m_VAO_Background);
		glBindBuffer(GL_ARRAY_BUFFER, m_VBO_Background);
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, Position));
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);

		s_IsInitialized = true;
	}

} // namespace onion::voxel
