#pragma once

#include <glm/glm.hpp>

#include <filesystem>
#include <string>

namespace onion::voxel
{

	class Shader
	{
	  public:
		Shader() = delete;
		/// @brief Constructs a shader program by loading and compiling the vertex and fragment shaders from the specified file paths. Compilation is deferred until the first call to Use().
		Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath);
		~Shader();

		/// @brief Deleted copy constructor by design.
		Shader(const Shader&) = delete;
		/// @brief Deleted copy assignment operator by design.
		Shader& operator=(const Shader&) = delete;

		/// @brief Implement move constructor
		Shader(Shader&& other) noexcept;
		/// @brief Implement move assignment operator
		Shader& operator=(Shader&& other) noexcept;

		/// @brief Checks if the shader program has been compiled. Compilation is deferred until the first call to Use().
		bool HasBeenCompiled() const { return m_HasBeenCompiled; }

		/// @brief Activates the shader program for use in the current OpenGL context. If the shader has not been compiled yet, it will be compiled at this point.
		void Use() const;
		/// @brief Deletes the shader program from the GPU, freeing up resources. After calling this, the shader object should not be used.
		void Delete();

		/// @brief Sets a boolean uniform variable in the shader program.
		void setBool(const std::string& name, bool value) const;
		/// @brief Sets an integer uniform variable in the shader program.
		void setInt(const std::string& name, int value) const;
		/// @brief Sets a float uniform variable in the shader program.
		void setFloat(const std::string& name, float value) const;
		/// @brief Sets a glm::vec2 uniform variable in the shader program.
		void setVec2(const std::string& name, const glm::vec2& value) const;
		/// @brief Sets a vec2 uniform variable in the shader program.
		void setVec2(const std::string& name, float x, float y) const;
		/// @brief Sets a glm::vec3 uniform variable in the shader program.
		void setVec3(const std::string& name, const glm::vec3& value) const;
		/// @brief Sets a vec3 uniform variable in the shader program.
		void setVec3(const std::string& name, float x, float y, float z) const;
		/// @brief Sets a glm::mat2 uniform variable in the shader program.
		void setMat2(const std::string& name, const glm::mat2& mat) const;
		/// @brief Sets a glm::mat3 uniform variable in the shader program.
		void setMat3(const std::string& name, const glm::mat3& mat) const;
		/// @brief Sets a glm::mat4 uniform variable in the shader program.
		void setMat4(const std::string& name, const glm::mat4& mat) const;

	  private:
		/// @brief Compiles the vertex and fragment shaders from their respective file paths and links them into a shader program. This is called internally when Use() is called for the first time. After compilation, the shader program ID is stored.
		void Compile() const;
		/// @brief The OpenGL shader program ID. This is generated when the shaders are compiled and linked.
		mutable unsigned int m_ProgramID = 0;
		/// @brief Flag indicating whether the shader program has been compiled. This is set to true after successful compilation in Compile().
		mutable bool m_HasBeenCompiled = false;
		/// @brief The file path of the vertex shader source code.
		std::filesystem::path m_VertexPath;
		/// @brief The file path of the fragment shader source code.
		std::filesystem::path m_FragmentPath;
	};
} // namespace onion::voxel
