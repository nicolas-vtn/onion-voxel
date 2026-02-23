#pragma once

#include <glm/glm.hpp>
#include <string>

namespace onion::voxel
{

	class Shader
	{
	  public:
		Shader(const char* vertexPath, const char* fragmentPath);
		~Shader();

		// Delete copy constructor and assignment
		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		// Implement move constructor
		Shader(Shader&& other) noexcept : m_ProgramID(other.m_ProgramID)
		{
			other.m_ProgramID = 0;
			m_HasBeenCompiled = other.m_HasBeenCompiled;
		}
		// Implement move assignment
		Shader& operator=(Shader&& other) noexcept;

		void Compile() const;
		bool HasBeenCompiled() const { return m_HasBeenCompiled; }

		void Use() const;
		void Delete();

		void setBool(const std::string& name, bool value) const;
		void setInt(const std::string& name, int value) const;
		void setFloat(const std::string& name, float value) const;
		void setVec2(const std::string& name, const glm::vec2& value) const;
		void setVec2(const std::string& name, float x, float y) const;
		void setVec3(const std::string& name, const glm::vec3& value) const;
		void setVec3(const std::string& name, float x, float y, float z) const;
		void setMat2(const std::string& name, const glm::mat2& mat) const;
		void setMat3(const std::string& name, const glm::mat3& mat) const;
		void setMat4(const std::string& name, const glm::mat4& mat) const;

	  private:
		mutable unsigned int m_ProgramID = 0;
		mutable bool m_HasBeenCompiled = false;
		std::string m_VertexPath;
		std::string m_FragmentPath;
	};
} // namespace onion::voxel
