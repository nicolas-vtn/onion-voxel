#pragma once

#include <optional>
#include <shared_mutex>

#include <renderer/assets_manager/AssetsManager.hpp>
#include <renderer/inputs_manager/inputs_manager.hpp>
#include <renderer/key_binds/KeyBinds.hpp>
#include <shared/world/raycast/Raycast.hpp>
#include <shared/world/world_manager/WorldManager.hpp>
#include <user_settings/UserSettings.hpp>

namespace onion::voxel
{
	class WorldRenderer;
}

namespace onion::voxel
{
	class EngineContext
	{
		// ----- MEMBERS -----
	  public:
		std::string PlayerUUID;
		WorldManager* World;
		AssetsManager* Assets;
		InputsManager* Inputs;
		KeyBinds* Keys;
		WorldRenderer* WrldRenderer;
		std::atomic_bool ShowDebugMenus{true};
		std::atomic<uint64_t> FrameCount{0};

		/// @brief The block the local player is currently looking at. Written by Renderer each frame.
		/// std::nullopt when the player is not looking at any block (e.g. in the main menu or looking at the sky).
		std::optional<RaycastHit> LookedAtBlock;

		UserSettings Settings() const
		{
			std::shared_lock lock(m_MutexSettings);
			return m_Settings;
		}

		void UpdateSettings(const UserSettings& newSettings)
		{
			std::unique_lock lock(m_MutexSettings);
			m_Settings = newSettings;
		}

		void SaveSettings(const std::filesystem::path& path) const
		{
			std::shared_lock lock(m_MutexSettings);
			UserSettings::Save(m_Settings, path);
		}

		// ----- Public Static API -----
	  public:
		static void Initialize(WorldManager* world,
							   AssetsManager* assets,
							   InputsManager* inputs,
							   onion::voxel::KeyBinds* keyBinds,
							   const UserSettings& settings,
							   WorldRenderer* worldRenderer)
		{
			if (s_Instance)
				throw std::runtime_error("EngineContext already initialized");

			s_Instance = new EngineContext(world, assets, inputs, keyBinds, settings, worldRenderer);
		}

		static EngineContext& Get()
		{
			if (!s_Instance)
				throw std::runtime_error("EngineContext not initialized");

			return *s_Instance;
		}

		// ----- Public API -----
	  public:
		std::shared_ptr<Player> GetLocalPlayer() const { return World->GetPlayer(PlayerUUID); }

		// ----- Constructor / Destructor -----
	  private:
		EngineContext(WorldManager* world,
					  AssetsManager* assets,
					  InputsManager* inputs,
					  onion::voxel::KeyBinds* keyBinds,
					  const UserSettings& settings,
					  WorldRenderer* worldRenderer)
			: World(world), Assets(assets), Inputs(inputs), Keys(keyBinds), m_Settings(settings),
			  WrldRenderer(worldRenderer)
		{
		}

		// ----- Private Members -----
	  private:
		mutable std::shared_mutex m_MutexSettings;
		UserSettings m_Settings;

		// ----- Private Static Members -----
	  private:
		static inline EngineContext* s_Instance = nullptr;
	};
} // namespace onion::voxel
