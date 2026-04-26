#pragma once

#include <glm/glm.hpp>

#include <atomic>
#include <mutex>

#include <renderer/OpenGL.hpp>

#include <renderer/shader/shader.hpp>
#include <renderer/texture/texture.hpp>
#include <renderer/texture_atlas/TextureAtlas.hpp>

#include <shared/entities/components/Inventory.hpp>

namespace onion::voxel
{
	class MeshBuilder;

	class UiBlockMesh
	{
		friend class MeshBuilder;

	  public:
		struct Vertex;

		// ----- Constructor / Destructor -----
	  public:
		UiBlockMesh(const Inventory& inventory);
		~UiBlockMesh();

		// ----- Public API -----
	  public:
		void Render(const glm::vec2& topLeftPosition, int screenWidth, int screenHeight);

		void SetInventory(const Inventory& inventory, const glm::vec2& slotSize, const glm::vec2& slotPadding);

		void Delete();

		uint32_t GetVertexCount() const;

		// ----- Getters / Setters -----
	  public:
		bool IsDirty() const;
		void SetDirty(bool isDirty);

		static void SetLightColor(const glm::vec3& lightColor);
		static glm::vec3 GetLightColor();

		static void SetUseFaceShading(bool useFaceShading);
		static bool GetUseFaceShading();

		static void SetUseOcclusion(bool useOcclusion);
		static bool GetUseOcclusion();

		// ----- Members -----
	  private:
		Inventory m_Inventory;
		glm::vec2 m_SlotSize{50.f, 50.f};	   // In pixels
		glm::vec2 m_SlotPadding{10.f, 10.f};   // In pixels

		std::shared_ptr<TextureAtlas> m_TextureAtlas;

		// ----- States -----
	  private:
		std::atomic_bool m_IsDirty{true};
		std::atomic_bool m_NeedsToPrepareRendering{true};

		std::atomic_bool m_AreBuffersGenerated{false};
		std::atomic_bool m_AreBuffersDataUpToDate{true};

		std::atomic_uint32_t m_VertexCount = 0; // The total vertex count for this subchunk mesh

		// ----- OPENGL Buffers -----
	  protected:
		void BuffersUpdated();

		mutable std::mutex m_Mutex;

		std::vector<Vertex> m_VerticesOpaque;  // Vertex data for the mesh
		std::vector<uint32_t> m_IndicesOpaque; // Indices for the Opaque mesh
		unsigned int m_IndicesOpaqueCount = 0; // Count of indices for the classic mesh

		std::vector<Vertex> m_VerticesCutout;  // Vertex data for the mesh
		std::vector<uint32_t> m_IndicesCutout; // Indices for the mesh
		unsigned int m_IndicesCutoutCount = 0; // Count of indices for the classic mesh

		std::vector<Vertex> m_VerticesTransparent;	// Vertex data for the mesh
		std::vector<uint32_t> m_IndicesTransparent; // Indices for the mesh
		unsigned int m_IndicesTransparentCount = 0; // Count of indices for the classic mesh

		unsigned int VBO_Opaque = 0, VAO_Opaque = 0, EBO_Opaque = 0;
		unsigned int VBO_Cutout = 0, VAO_Cutout = 0, EBO_Cutout = 0;
		unsigned int VBO_Transparent = 0, VAO_Transparent = 0, EBO_Transparent = 0;

		// ----- OpenGl Methods / Initialization -----
	  private:
		void InitOpenGlBuffers();
		void UpdateOpenGlBuffers();
		void CleanupOpenGlBuffers();

		void PrepareForRendering();

		void PrepareForRenderingOpaque();
		void RenderOpaque();

		void PrepareForRenderingCutout();
		void RenderCutout();

		void PrepareForRenderingTransparent();
		void RenderTransparent();

		void ResetOpenGLState();

		// ----- Static Shader & Texture -----
	  public:
		static Shader s_Shader;

	  private:
		static inline glm::vec3 s_LightColor{1.0f, 1.0f, 1.0f};
		static inline bool s_UseFaceShading = true;
		static inline bool s_UseOcclusion = true;

		// ----- Structs -----
	  public:
		struct Vertex
		{
			float x;
			float y;
			float z;

			float texX, texY; // Texture coordinates

			uint8_t tintR, tintG, tintB; // RGB tint color
			uint8_t facing;				 // Facing direction (0-5 for the 6 faces of a cube)
		};
	};
} // namespace onion::voxel
