#pragma once

#include <renderer/assets_manager/AssetsManager.hpp>
#include <renderer/inputs_manager/inputs_manager.hpp>
#include <shared/world/world_manager/WorldManager.hpp>

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

		// ----- Public Static API -----
	  public:
		static void Initialize(WorldManager* world, AssetsManager* assets, InputsManager* inputs)
		{
			if (s_Instance)
				throw std::runtime_error("EngineContext already initialized");

			s_Instance = new EngineContext(world, assets, inputs);
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
		EngineContext(WorldManager* world, AssetsManager* assets, InputsManager* inputs)
			: World(world), Assets(assets), Inputs(inputs)
		{
		}

		// ----- Private Static Members -----
	  private:
		static inline EngineContext* s_Instance = nullptr;
	};
} // namespace onion::voxel
