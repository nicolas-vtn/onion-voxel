#pragma once

#include <glm/glm.hpp>

#include <atomic>
#include <mutex>
#include <vector>

#include <renderer/OpenGL.hpp>
#include <renderer/shader/shader.hpp>
#include <renderer/texture_atlas/TextureAtlas.hpp>

namespace onion::voxel
{
	class MeshBuilder;

	/// @brief Base class that owns the three-pass (opaque/cutout/transparent) OpenGL buffers
	/// and the shared blocks_ui shader used to render item/block meshes.
	/// Subclasses: UiBlockMesh (inventory UI) and BlockEntityMesh (world-space dropped items).
	class ItemMesh
	{
		friend class MeshBuilder;

	  public:
		struct Vertex
		{
			float x;
			float y;
			float z;

			float texX, texY;

			uint8_t tintR, tintG, tintB;
			uint8_t facing; // Facing direction (0-5 for the 6 faces of a cube)
		};

		// ----- Constructor / Destructor -----
	  public:
		ItemMesh() = default;
		virtual ~ItemMesh() = default;

		// ----- Public API -----
	  public:
		void Delete();
		uint32_t GetVertexCount() const;

		// ----- Static Light / Shading Settings -----
	  public:
		static void SetLightColor(const glm::vec3& lightColor);
		static glm::vec3 GetLightColor();

		static void SetUseFaceShading(bool useFaceShading);
		static bool GetUseFaceShading();

		static void SetUseOcclusion(bool useOcclusion);
		static bool GetUseOcclusion();

		// ----- Static Shader -----
	  public:
		static Shader s_Shader;

	  protected:
		// ----- Buffer Lifecycle (called on GL thread) -----
		void BuffersUpdated();
		void PrepareForRendering();

		// Three-pass helpers — called by subclass Render() methods
		void PrepareForRenderingOpaque();
		void RenderOpaque();

		void PrepareForRenderingCutout();
		void RenderCutout();

		void PrepareForRenderingTransparent();
		void RenderTransparent();

		void ResetOpenGLState();

		// ----- Texture Atlas (set by MeshBuilder) -----
	  protected:
		std::shared_ptr<TextureAtlas> m_TextureAtlas;

		// ----- CPU-side buffers (written by MeshBuilder) -----
	  protected:
		mutable std::mutex m_Mutex;

		std::vector<Vertex> m_VerticesOpaque;
		std::vector<uint32_t> m_IndicesOpaque;
		unsigned int m_IndicesOpaqueCount = 0;

		std::vector<Vertex> m_VerticesCutout;
		std::vector<uint32_t> m_IndicesCutout;
		unsigned int m_IndicesCutoutCount = 0;

		std::vector<Vertex> m_VerticesTransparent;
		std::vector<uint32_t> m_IndicesTransparent;
		unsigned int m_IndicesTransparentCount = 0;

		// ----- OpenGL objects -----
	  protected:
		unsigned int VBO_Opaque = 0, VAO_Opaque = 0, EBO_Opaque = 0;
		unsigned int VBO_Cutout = 0, VAO_Cutout = 0, EBO_Cutout = 0;
		unsigned int VBO_Transparent = 0, VAO_Transparent = 0, EBO_Transparent = 0;

		// ----- State flags -----
	  private:
		std::atomic_bool m_AreBuffersGenerated{false};
		std::atomic_bool m_AreBuffersDataUpToDate{true};
		std::atomic_bool m_NeedsToPrepareRendering{true};
		std::atomic_uint32_t m_VertexCount{0};

		void InitOpenGlBuffers();
		void UpdateOpenGlBuffers();
		void CleanupOpenGlBuffers();

		// ----- Static Settings -----
	  private:
		static inline glm::vec3 s_LightColor{1.0f, 1.0f, 1.0f};
		static inline bool s_UseFaceShading = true;
		static inline bool s_UseOcclusion = true;
	};

} // namespace onion::voxel
