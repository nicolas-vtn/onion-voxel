#include "shader.hpp"

#include <cassert>
#include <fstream>
#include <glad/glad.h>
#include <iostream>
#include <sstream>

using namespace onion::voxel;

Shader::Shader(const std::filesystem::path& vertexPath, const std::filesystem::path& fragmentPath)
	: m_FragmentPath(fragmentPath), m_VertexPath(vertexPath)
{
	if (!std::filesystem::exists(vertexPath))
	{
		std::cerr << "[SHADER] [ERROR] : Vertex shader file does not exist: " << vertexPath << std::endl;
		throw std::runtime_error("Vertex shader file does not exist: " + vertexPath.string());
	}
	if (!std::filesystem::exists(fragmentPath))
	{
		std::cerr << "[SHADER] [ERROR] : Fragment shader file does not exist: " << fragmentPath << std::endl;
		throw std::runtime_error("Fragment shader file does not exist: " + fragmentPath.string());
	}
}

onion::voxel::Shader::Shader(Shader&& other) noexcept : m_ProgramID(other.m_ProgramID)
{
	other.m_ProgramID = 0;
	m_HasBeenCompiled = other.m_HasBeenCompiled;
}

Shader& Shader::operator=(Shader&& other) noexcept
{
	if (this != &other)
	{
		m_ProgramID = other.m_ProgramID; // Transfer ownership
		other.m_ProgramID = 0;			 // Reset the moved-from object
		m_HasBeenCompiled = other.m_HasBeenCompiled;
	}
	return *this;
}

void Shader::Compile() const
{
	// 1. Retrieve the vertex/fragment source code from filePath
	std::string vertexCode, fragmentCode;
	std::ifstream vShaderFile, fShaderFile;

	// Enable exceptions to catch read errors
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		// Open shader files
		vShaderFile.open(m_VertexPath);
		fShaderFile.open(m_FragmentPath);

		// Check if actually opened (warns about missing file)
		if (!vShaderFile.is_open())
		{
			std::cerr << "[SHADER] [ERROR] : Failed to open vertex shader file: '" << m_VertexPath << "'" << std::endl;
			throw std::runtime_error("Shader file " + m_VertexPath.string() + " read error");
		}
		if (!fShaderFile.is_open())
		{
			std::cerr << "[SHADER] [ERROR] : Failed to open fragment shader file: '" << m_FragmentPath << "'"
					  << std::endl;
			throw std::runtime_error("Shader file " + m_FragmentPath.string() + " read error");
		}

		// Read file content into streams
		std::stringstream vShaderStream, fShaderStream;
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();

		// Close files
		vShaderFile.close();
		fShaderFile.close();

		// Convert to strings
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure& e)
	{
		std::cerr << "[SHADER] [ERROR] : Shader file '" << m_VertexPath << "' or '" << m_FragmentPath
				  << "' not successfully read: " << e.what() << std::endl;
		throw std::runtime_error("Shader file read error");
	}

	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();

	// 2. Compile shaders
	unsigned int vertex, fragment;
	int success;
	char infoLog[512];

	// Vertex Shader
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, NULL, infoLog);
		std::cerr << "[SHADER] [ERROR] : Vertex shader '" << m_VertexPath << "' compilation failed :" << infoLog
				  << std::endl;
		throw std::runtime_error("Vertex shader '" + m_VertexPath.string() + "' compilation failed");
	}

	// Fragment Shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragment, 512, NULL, infoLog);
		std::cerr << "[SHADER] [ERROR] : Fragment shader '" << m_FragmentPath << "' compilation failed :" << infoLog
				  << std::endl;
		throw std::runtime_error("Fragment shader '" + m_FragmentPath.string() + "' compilation failed");
	}

	// Shader Program
	m_ProgramID = glCreateProgram();
	glAttachShader(m_ProgramID, vertex);
	glAttachShader(m_ProgramID, fragment);
	glLinkProgram(m_ProgramID);
	glGetProgramiv(m_ProgramID, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(m_ProgramID, 512, NULL, infoLog);
		std::cerr << "[SHADER] [ERROR] : Shader program '" << m_ProgramID << "' from '" << m_VertexPath << "' and '"
				  << m_FragmentPath << "' linking failed :" << infoLog << std::endl;
		throw std::runtime_error("Shader program '" + std::to_string(m_ProgramID) + "' linking failed");
	}

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	m_HasBeenCompiled = true; // Mark shader as compiled
}

Shader::~Shader()
{
	if (m_ProgramID != 0)
	{
		std::cerr << "[SHADER] [ERROR] : Shaders '" << m_VertexPath << "' and '" << m_FragmentPath
				  << "' not deleted before destruction. There is a memory leak." << std::endl;
	}
}

void Shader::Use() const
{
	if (!m_HasBeenCompiled)
	{
		Compile();
	}
	glUseProgram(m_ProgramID);
}

void Shader::Delete()
{
	glDeleteProgram(m_ProgramID);
	m_ProgramID = 0;
}

void Shader::setBool(const std::string& name, bool value) const
{
	glUniform1i(glGetUniformLocation(m_ProgramID, name.c_str()), (int) value);
}

void Shader::setInt(const std::string& name, int value) const
{
	glUniform1i(glGetUniformLocation(m_ProgramID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value) const
{
	glUniform1f(glGetUniformLocation(m_ProgramID, name.c_str()), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const
{
	glUniform2fv(glGetUniformLocation(m_ProgramID, name.c_str()), 1, &value[0]);
}

void Shader::setVec2(const std::string& name, float x, float y) const
{
	glUniform2f(glGetUniformLocation(m_ProgramID, name.c_str()), x, y);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const
{
	glUniform3fv(glGetUniformLocation(m_ProgramID, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, float x, float y, float z) const
{
	glUniform3f(glGetUniformLocation(m_ProgramID, name.c_str()), x, y, z);
}

void Shader::setMat2(const std::string& name, const glm::mat2& mat) const
{
	glUniformMatrix2fv(glGetUniformLocation(m_ProgramID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat) const
{
	glUniformMatrix3fv(glGetUniformLocation(m_ProgramID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const
{
	glUniformMatrix4fv(glGetUniformLocation(m_ProgramID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}
