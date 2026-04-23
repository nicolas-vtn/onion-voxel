#include "BlockState.hpp"

namespace onion::voxel
{
	// ----- Static Initialization -----
	std::vector<bool> BlockState::s_TransparencyLookupTable = []()
	{
		std::vector<bool> table(static_cast<size_t>(BlockIds::GetBlockIdCount()), false);
		table[static_cast<size_t>(BlockId::Air)] = true;
		return table;
	}();

	const std::vector<bool> BlockState::s_SolidLookupTable = []()
	{
		std::vector<bool> table(static_cast<size_t>(BlockIds::GetBlockIdCount()), true);

		// ── Air / void ──────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Air)] = false;
		table[static_cast<size_t>(BlockId::CaveAir)] = false;
		table[static_cast<size_t>(BlockId::VoidAir)] = false;

		// ── Fluids ──────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Water)] = false;
		table[static_cast<size_t>(BlockId::Lava)] = false;
		table[static_cast<size_t>(BlockId::BubbleColumn)] = false;

		// ── Fire ────────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Fire)] = false;
		table[static_cast<size_t>(BlockId::SoulFire)] = false;

		// ── Tall plants / grass / ferns ─────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::ShortGrass)] = false;
		table[static_cast<size_t>(BlockId::TallGrass)] = false;
		table[static_cast<size_t>(BlockId::Fern)] = false;
		table[static_cast<size_t>(BlockId::LargeFern)] = false;
		table[static_cast<size_t>(BlockId::ShortDryGrass)] = false;
		table[static_cast<size_t>(BlockId::TallDryGrass)] = false;

		// ── Flowers ─────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Dandelion)] = false;
		table[static_cast<size_t>(BlockId::Poppy)] = false;
		table[static_cast<size_t>(BlockId::BlueOrchid)] = false;
		table[static_cast<size_t>(BlockId::Allium)] = false;
		table[static_cast<size_t>(BlockId::AzureBluet)] = false;
		table[static_cast<size_t>(BlockId::RedTulip)] = false;
		table[static_cast<size_t>(BlockId::OrangeTulip)] = false;
		table[static_cast<size_t>(BlockId::WhiteTulip)] = false;
		table[static_cast<size_t>(BlockId::PinkTulip)] = false;
		table[static_cast<size_t>(BlockId::OxeyeDaisy)] = false;
		table[static_cast<size_t>(BlockId::Cornflower)] = false;
		table[static_cast<size_t>(BlockId::LilyOfTheValley)] = false;
		table[static_cast<size_t>(BlockId::WitherRose)] = false;
		table[static_cast<size_t>(BlockId::Torchflower)] = false;
		table[static_cast<size_t>(BlockId::OpenEyeblossom)] = false;
		table[static_cast<size_t>(BlockId::ClosedEyeblossom)] = false;
		table[static_cast<size_t>(BlockId::Wildflowers)] = false;

		// ── Double-tall flowers ──────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Sunflower)] = false;
		table[static_cast<size_t>(BlockId::Lilac)] = false;
		table[static_cast<size_t>(BlockId::RoseBush)] = false;
		table[static_cast<size_t>(BlockId::Peony)] = false;
		table[static_cast<size_t>(BlockId::PitcherPlant)] = false;

		// ── Saplings ─────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::OakSapling)] = false;
		table[static_cast<size_t>(BlockId::SpruceSapling)] = false;
		table[static_cast<size_t>(BlockId::BirchSapling)] = false;
		table[static_cast<size_t>(BlockId::JungleSapling)] = false;
		table[static_cast<size_t>(BlockId::AcaciaSapling)] = false;
		table[static_cast<size_t>(BlockId::DarkOakSapling)] = false;
		table[static_cast<size_t>(BlockId::CherrySapling)] = false;
		table[static_cast<size_t>(BlockId::BambooSapling)] = false;
		table[static_cast<size_t>(BlockId::PaleOakSapling)] = false;
		table[static_cast<size_t>(BlockId::MangrovePropagule)] = false;

		// ── Dead bush / bush ─────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::DeadBush)] = false;
		table[static_cast<size_t>(BlockId::Bush)] = false;
		table[static_cast<size_t>(BlockId::FireflyBush)] = false;
		table[static_cast<size_t>(BlockId::SweetBerryBush)] = false;
		table[static_cast<size_t>(BlockId::Azalea)] = false;
		table[static_cast<size_t>(BlockId::FloweringAzalea)] = false;

		// ── Nether plants ────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::NetherSprouts)] = false;
		table[static_cast<size_t>(BlockId::CrimsonRoots)] = false;
		table[static_cast<size_t>(BlockId::WarpedRoots)] = false;
		table[static_cast<size_t>(BlockId::CrimsonFungus)] = false;
		table[static_cast<size_t>(BlockId::WarpedFungus)] = false;
		table[static_cast<size_t>(BlockId::NetherWart)] = false;

		// ── Bamboo ───────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Bamboo)] = false;

		// ── Sugar cane ───────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::SugarCane)] = false;

		// ── Cactus flower ────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::CactusFlower)] = false;

		// ── Kelp / seagrass / aquatic plants ────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Kelp)] = false;
		table[static_cast<size_t>(BlockId::KelpPlant)] = false;
		table[static_cast<size_t>(BlockId::Seagrass)] = false;
		table[static_cast<size_t>(BlockId::TallSeagrass)] = false;

		// ── Corals (non-block variants) ──────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::BrainCoral)] = false;
		table[static_cast<size_t>(BlockId::BrainCoralFan)] = false;
		table[static_cast<size_t>(BlockId::BrainCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::BubbleCoral)] = false;
		table[static_cast<size_t>(BlockId::BubbleCoralFan)] = false;
		table[static_cast<size_t>(BlockId::BubbleCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::FireCoral)] = false;
		table[static_cast<size_t>(BlockId::FireCoralFan)] = false;
		table[static_cast<size_t>(BlockId::FireCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::HornCoral)] = false;
		table[static_cast<size_t>(BlockId::HornCoralFan)] = false;
		table[static_cast<size_t>(BlockId::HornCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::TubeCoral)] = false;
		table[static_cast<size_t>(BlockId::TubeCoralFan)] = false;
		table[static_cast<size_t>(BlockId::TubeCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::DeadBrainCoral)] = false;
		table[static_cast<size_t>(BlockId::DeadBrainCoralFan)] = false;
		table[static_cast<size_t>(BlockId::DeadBrainCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::DeadBubbleCoral)] = false;
		table[static_cast<size_t>(BlockId::DeadBubbleCoralFan)] = false;
		table[static_cast<size_t>(BlockId::DeadBubbleCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::DeadFireCoral)] = false;
		table[static_cast<size_t>(BlockId::DeadFireCoralFan)] = false;
		table[static_cast<size_t>(BlockId::DeadFireCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::DeadHornCoral)] = false;
		table[static_cast<size_t>(BlockId::DeadHornCoralFan)] = false;
		table[static_cast<size_t>(BlockId::DeadHornCoralWallFan)] = false;
		table[static_cast<size_t>(BlockId::DeadTubeCoral)] = false;
		table[static_cast<size_t>(BlockId::DeadTubeCoralFan)] = false;
		table[static_cast<size_t>(BlockId::DeadTubeCoralWallFan)] = false;

		// ── Sea pickle / frogspawn ──────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::SeaPickle)] = false;
		table[static_cast<size_t>(BlockId::Frogspawn)] = false;

		// ── Amethyst buds / cluster ──────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::SmallAmethystBud)] = false;
		table[static_cast<size_t>(BlockId::MediumAmethystBud)] = false;
		table[static_cast<size_t>(BlockId::LargeAmethystBud)] = false;
		table[static_cast<size_t>(BlockId::AmethystCluster)] = false;

		// ── Vines ────────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Vine)] = false;
		table[static_cast<size_t>(BlockId::TwistingVines)] = false;
		table[static_cast<size_t>(BlockId::TwistingVinesPlant)] = false;
		table[static_cast<size_t>(BlockId::WeepingVines)] = false;
		table[static_cast<size_t>(BlockId::WeepingVinesPlant)] = false;
		table[static_cast<size_t>(BlockId::CaveVines)] = false;
		table[static_cast<size_t>(BlockId::CaveVinesPlant)] = false;

		// ── Hanging / draping decorations ────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::HangingRoots)] = false;
		table[static_cast<size_t>(BlockId::SporeBlossom)] = false;
		table[static_cast<size_t>(BlockId::PaleHangingMoss)] = false;
		table[static_cast<size_t>(BlockId::GlowLichen)] = false;
		table[static_cast<size_t>(BlockId::SculkVein)] = false;
		table[static_cast<size_t>(BlockId::ResinClump)] = false;
		table[static_cast<size_t>(BlockId::LeafLitter)] = false;
		table[static_cast<size_t>(BlockId::PinkPetals)] = false;

		// ── Thin carpets / moss / snow layer ────────────────────────────────────────
		table[static_cast<size_t>(BlockId::MossCarpet)] = false;
		table[static_cast<size_t>(BlockId::PaleMossCarpet)] = false;
		table[static_cast<size_t>(BlockId::Snow)] = false;

		// ── Lily pad ─────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::LilyPad)] = false;

		// ── Torches ──────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Torch)] = false;
		table[static_cast<size_t>(BlockId::WallTorch)] = false;
		table[static_cast<size_t>(BlockId::RedstoneTorch)] = false;
		table[static_cast<size_t>(BlockId::RedstoneWallTorch)] = false;
		table[static_cast<size_t>(BlockId::SoulTorch)] = false;
		table[static_cast<size_t>(BlockId::SoulWallTorch)] = false;
		table[static_cast<size_t>(BlockId::CopperTorch)] = false;
		table[static_cast<size_t>(BlockId::CopperWallTorch)] = false;

		// ── Redstone components (flat / thin) ────────────────────────────────────────
		table[static_cast<size_t>(BlockId::RedstoneWire)] = false;
		table[static_cast<size_t>(BlockId::Repeater)] = false;
		table[static_cast<size_t>(BlockId::Comparator)] = false;
		table[static_cast<size_t>(BlockId::DaylightDetector)] = false;
		table[static_cast<size_t>(BlockId::TripwireHook)] = false;
		table[static_cast<size_t>(BlockId::Tripwire)] = false;

		// ── Rails ────────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Rail)] = false;
		table[static_cast<size_t>(BlockId::PoweredRail)] = false;
		table[static_cast<size_t>(BlockId::DetectorRail)] = false;
		table[static_cast<size_t>(BlockId::ActivatorRail)] = false;

		// ── Buttons ──────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::OakButton)] = false;
		table[static_cast<size_t>(BlockId::SpruceButton)] = false;
		table[static_cast<size_t>(BlockId::BirchButton)] = false;
		table[static_cast<size_t>(BlockId::JungleButton)] = false;
		table[static_cast<size_t>(BlockId::AcaciaButton)] = false;
		table[static_cast<size_t>(BlockId::DarkOakButton)] = false;
		table[static_cast<size_t>(BlockId::CrimsonButton)] = false;
		table[static_cast<size_t>(BlockId::WarpedButton)] = false;
		table[static_cast<size_t>(BlockId::BambooButton)] = false;
		table[static_cast<size_t>(BlockId::CherryButton)] = false;
		table[static_cast<size_t>(BlockId::PaleOakButton)] = false;
		table[static_cast<size_t>(BlockId::MangroveButton)] = false;
		table[static_cast<size_t>(BlockId::StoneButton)] = false;
		table[static_cast<size_t>(BlockId::PolishedBlackstoneButton)] = false;

		// ── Pressure plates ──────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::OakPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::SprucePressurePlate)] = false;
		table[static_cast<size_t>(BlockId::BirchPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::JunglePressurePlate)] = false;
		table[static_cast<size_t>(BlockId::AcaciaPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::DarkOakPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::CrimsonPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::WarpedPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::BambooPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::CherryPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::PaleOakPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::MangrovePressurePlate)] = false;
		table[static_cast<size_t>(BlockId::StonePressurePlate)] = false;
		table[static_cast<size_t>(BlockId::LightWeightedPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::HeavyWeightedPressurePlate)] = false;
		table[static_cast<size_t>(BlockId::PolishedBlackstonePressurePlate)] = false;

		// ── Lever ────────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Lever)] = false;

		// ── Signs / hanging signs (flat decorations) ─────────────────────────────────
		table[static_cast<size_t>(BlockId::OakSign)] = false;
		table[static_cast<size_t>(BlockId::OakWallSign)] = false;
		table[static_cast<size_t>(BlockId::OakHangingSign)] = false;
		table[static_cast<size_t>(BlockId::OakWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::SpruceSign)] = false;
		table[static_cast<size_t>(BlockId::SpruceWallSign)] = false;
		table[static_cast<size_t>(BlockId::SpruceHangingSign)] = false;
		table[static_cast<size_t>(BlockId::SpruceWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::BirchSign)] = false;
		table[static_cast<size_t>(BlockId::BirchWallSign)] = false;
		table[static_cast<size_t>(BlockId::BirchHangingSign)] = false;
		table[static_cast<size_t>(BlockId::BirchWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::JungleSign)] = false;
		table[static_cast<size_t>(BlockId::JungleWallSign)] = false;
		table[static_cast<size_t>(BlockId::JungleHangingSign)] = false;
		table[static_cast<size_t>(BlockId::JungleWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::AcaciaSign)] = false;
		table[static_cast<size_t>(BlockId::AcaciaWallSign)] = false;
		table[static_cast<size_t>(BlockId::AcaciaHangingSign)] = false;
		table[static_cast<size_t>(BlockId::AcaciaWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::DarkOakSign)] = false;
		table[static_cast<size_t>(BlockId::DarkOakWallSign)] = false;
		table[static_cast<size_t>(BlockId::DarkOakHangingSign)] = false;
		table[static_cast<size_t>(BlockId::DarkOakWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::CrimsonSign)] = false;
		table[static_cast<size_t>(BlockId::CrimsonWallSign)] = false;
		table[static_cast<size_t>(BlockId::CrimsonHangingSign)] = false;
		table[static_cast<size_t>(BlockId::CrimsonWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::WarpedSign)] = false;
		table[static_cast<size_t>(BlockId::WarpedWallSign)] = false;
		table[static_cast<size_t>(BlockId::WarpedHangingSign)] = false;
		table[static_cast<size_t>(BlockId::WarpedWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::BambooSign)] = false;
		table[static_cast<size_t>(BlockId::BambooWallSign)] = false;
		table[static_cast<size_t>(BlockId::BambooHangingSign)] = false;
		table[static_cast<size_t>(BlockId::BambooWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::CherrySign)] = false;
		table[static_cast<size_t>(BlockId::CherryWallSign)] = false;
		table[static_cast<size_t>(BlockId::CherryHangingSign)] = false;
		table[static_cast<size_t>(BlockId::CherryWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::MangroveSign)] = false;
		table[static_cast<size_t>(BlockId::MangroveWallSign)] = false;
		table[static_cast<size_t>(BlockId::MangroveHangingSign)] = false;
		table[static_cast<size_t>(BlockId::MangroveWallHangingSign)] = false;
		table[static_cast<size_t>(BlockId::PaleOakSign)] = false;
		table[static_cast<size_t>(BlockId::PaleOakWallSign)] = false;
		table[static_cast<size_t>(BlockId::PaleOakHangingSign)] = false;
		table[static_cast<size_t>(BlockId::PaleOakWallHangingSign)] = false;

		// ── Banners / wall banners ────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::WhiteBanner)] = false;
		table[static_cast<size_t>(BlockId::WhiteWallBanner)] = false;
		table[static_cast<size_t>(BlockId::OrangeBanner)] = false;
		table[static_cast<size_t>(BlockId::OrangeWallBanner)] = false;
		table[static_cast<size_t>(BlockId::MagentaBanner)] = false;
		table[static_cast<size_t>(BlockId::MagentaWallBanner)] = false;
		table[static_cast<size_t>(BlockId::LightBlueBanner)] = false;
		table[static_cast<size_t>(BlockId::LightBlueWallBanner)] = false;
		table[static_cast<size_t>(BlockId::YellowBanner)] = false;
		table[static_cast<size_t>(BlockId::YellowWallBanner)] = false;
		table[static_cast<size_t>(BlockId::LimeBanner)] = false;
		table[static_cast<size_t>(BlockId::LimeWallBanner)] = false;
		table[static_cast<size_t>(BlockId::PinkBanner)] = false;
		table[static_cast<size_t>(BlockId::PinkWallBanner)] = false;
		table[static_cast<size_t>(BlockId::GrayBanner)] = false;
		table[static_cast<size_t>(BlockId::GrayWallBanner)] = false;
		table[static_cast<size_t>(BlockId::LightGrayBanner)] = false;
		table[static_cast<size_t>(BlockId::LightGrayWallBanner)] = false;
		table[static_cast<size_t>(BlockId::CyanBanner)] = false;
		table[static_cast<size_t>(BlockId::CyanWallBanner)] = false;
		table[static_cast<size_t>(BlockId::PurpleBanner)] = false;
		table[static_cast<size_t>(BlockId::PurpleWallBanner)] = false;
		table[static_cast<size_t>(BlockId::BlueBanner)] = false;
		table[static_cast<size_t>(BlockId::BlueWallBanner)] = false;
		table[static_cast<size_t>(BlockId::BrownBanner)] = false;
		table[static_cast<size_t>(BlockId::BrownWallBanner)] = false;
		table[static_cast<size_t>(BlockId::GreenBanner)] = false;
		table[static_cast<size_t>(BlockId::GreenWallBanner)] = false;
		table[static_cast<size_t>(BlockId::RedBanner)] = false;
		table[static_cast<size_t>(BlockId::RedWallBanner)] = false;
		table[static_cast<size_t>(BlockId::BlackBanner)] = false;
		table[static_cast<size_t>(BlockId::BlackWallBanner)] = false;

		// ── Candles ──────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Candle)] = false;
		table[static_cast<size_t>(BlockId::WhiteCandle)] = false;
		table[static_cast<size_t>(BlockId::OrangeCandle)] = false;
		table[static_cast<size_t>(BlockId::MagentaCandle)] = false;
		table[static_cast<size_t>(BlockId::LightBlueCandle)] = false;
		table[static_cast<size_t>(BlockId::YellowCandle)] = false;
		table[static_cast<size_t>(BlockId::LimeCandle)] = false;
		table[static_cast<size_t>(BlockId::PinkCandle)] = false;
		table[static_cast<size_t>(BlockId::GrayCandle)] = false;
		table[static_cast<size_t>(BlockId::LightGrayCandle)] = false;
		table[static_cast<size_t>(BlockId::CyanCandle)] = false;
		table[static_cast<size_t>(BlockId::PurpleCandle)] = false;
		table[static_cast<size_t>(BlockId::BlueCandle)] = false;
		table[static_cast<size_t>(BlockId::BrownCandle)] = false;
		table[static_cast<size_t>(BlockId::GreenCandle)] = false;
		table[static_cast<size_t>(BlockId::RedCandle)] = false;
		table[static_cast<size_t>(BlockId::BlackCandle)] = false;

		// ── Candle cakes (cake + candle on top, thin candle part) ────────────────────
		table[static_cast<size_t>(BlockId::CandleCake)] = false;
		table[static_cast<size_t>(BlockId::WhiteCandleCake)] = false;
		table[static_cast<size_t>(BlockId::OrangeCandleCake)] = false;
		table[static_cast<size_t>(BlockId::MagentaCandleCake)] = false;
		table[static_cast<size_t>(BlockId::LightBlueCandleCake)] = false;
		table[static_cast<size_t>(BlockId::YellowCandleCake)] = false;
		table[static_cast<size_t>(BlockId::LimeCandleCake)] = false;
		table[static_cast<size_t>(BlockId::PinkCandleCake)] = false;
		table[static_cast<size_t>(BlockId::GrayCandleCake)] = false;
		table[static_cast<size_t>(BlockId::LightGrayCandleCake)] = false;
		table[static_cast<size_t>(BlockId::CyanCandleCake)] = false;
		table[static_cast<size_t>(BlockId::PurpleCandleCake)] = false;
		table[static_cast<size_t>(BlockId::BlueCandleCake)] = false;
		table[static_cast<size_t>(BlockId::BrownCandleCake)] = false;
		table[static_cast<size_t>(BlockId::GreenCandleCake)] = false;
		table[static_cast<size_t>(BlockId::RedCandleCake)] = false;
		table[static_cast<size_t>(BlockId::BlackCandleCake)] = false;

		// ── Item frames ──────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::ItemFrame)] = false;
		table[static_cast<size_t>(BlockId::GlowItemFrame)] = false;

		// ── Flower pot & all potted variants ─────────────────────────────────────────
		table[static_cast<size_t>(BlockId::FlowerPot)] = false;
		table[static_cast<size_t>(BlockId::PottedAcaciaSapling)] = false;
		table[static_cast<size_t>(BlockId::PottedAllium)] = false;
		table[static_cast<size_t>(BlockId::PottedAzaleaBush)] = false;
		table[static_cast<size_t>(BlockId::PottedAzureBluet)] = false;
		table[static_cast<size_t>(BlockId::PottedBamboo)] = false;
		table[static_cast<size_t>(BlockId::PottedBirchSapling)] = false;
		table[static_cast<size_t>(BlockId::PottedBlueOrchid)] = false;
		table[static_cast<size_t>(BlockId::PottedBrownMushroom)] = false;
		table[static_cast<size_t>(BlockId::PottedCactus)] = false;
		table[static_cast<size_t>(BlockId::PottedCherrySapling)] = false;
		table[static_cast<size_t>(BlockId::PottedClosedEyeblossom)] = false;
		table[static_cast<size_t>(BlockId::PottedCornflower)] = false;
		table[static_cast<size_t>(BlockId::PottedCrimsonFungus)] = false;
		table[static_cast<size_t>(BlockId::PottedCrimsonRoots)] = false;
		table[static_cast<size_t>(BlockId::PottedDandelion)] = false;
		table[static_cast<size_t>(BlockId::PottedDarkOakSapling)] = false;
		table[static_cast<size_t>(BlockId::PottedDeadBush)] = false;
		table[static_cast<size_t>(BlockId::PottedFern)] = false;
		table[static_cast<size_t>(BlockId::PottedFloweringAzaleaBush)] = false;
		table[static_cast<size_t>(BlockId::PottedJungleSapling)] = false;
		table[static_cast<size_t>(BlockId::PottedLilyOfTheValley)] = false;
		table[static_cast<size_t>(BlockId::PottedMangrovePropagule)] = false;
		table[static_cast<size_t>(BlockId::PottedOakSapling)] = false;
		table[static_cast<size_t>(BlockId::PottedOpenEyeblossom)] = false;
		table[static_cast<size_t>(BlockId::PottedOrangeTulip)] = false;
		table[static_cast<size_t>(BlockId::PottedOxeyeDaisy)] = false;
		table[static_cast<size_t>(BlockId::PottedPaleOakSapling)] = false;
		table[static_cast<size_t>(BlockId::PottedPinkTulip)] = false;
		table[static_cast<size_t>(BlockId::PottedPoppy)] = false;
		table[static_cast<size_t>(BlockId::PottedRedMushroom)] = false;
		table[static_cast<size_t>(BlockId::PottedRedTulip)] = false;
		table[static_cast<size_t>(BlockId::PottedSpruceSapling)] = false;
		table[static_cast<size_t>(BlockId::PottedTorchflower)] = false;
		table[static_cast<size_t>(BlockId::PottedWarpedFungus)] = false;
		table[static_cast<size_t>(BlockId::PottedWarpedRoots)] = false;
		table[static_cast<size_t>(BlockId::PottedWhiteTulip)] = false;
		table[static_cast<size_t>(BlockId::PottedWitherRose)] = false;

		// ── Crop stems ───────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::MelonStem)] = false;
		table[static_cast<size_t>(BlockId::PumpkinStem)] = false;
		table[static_cast<size_t>(BlockId::AttachedMelonStem)] = false;
		table[static_cast<size_t>(BlockId::AttachedPumpkinStem)] = false;

		// ── Crops ────────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::Wheat)] = false;
		table[static_cast<size_t>(BlockId::Carrots)] = false;
		table[static_cast<size_t>(BlockId::Potatoes)] = false;
		table[static_cast<size_t>(BlockId::Beetroots)] = false;
		table[static_cast<size_t>(BlockId::Cocoa)] = false;
		table[static_cast<size_t>(BlockId::TorchflowerCrop)] = false;
		table[static_cast<size_t>(BlockId::PitcherCrop)] = false;

		// ── Dripleaf ─────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::BigDripleaf)] = false;
		table[static_cast<size_t>(BlockId::BigDripleafStem)] = false;
		table[static_cast<size_t>(BlockId::SmallDripleaf)] = false;

		// ── Chorus ───────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::ChorusFlower)] = false;
		table[static_cast<size_t>(BlockId::ChorusPlant)] = false;

		// ── End rod ──────────────────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::EndRod)] = false;

		// ── Portals / gateways ───────────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::NetherPortal)] = false;
		table[static_cast<size_t>(BlockId::EndPortal)] = false;
		table[static_cast<size_t>(BlockId::EndGateway)] = false;

		// ── Structure void / light ───────────────────────────────────────────────────
		table[static_cast<size_t>(BlockId::StructureVoid)] = false;
		table[static_cast<size_t>(BlockId::Light)] = false;

		// ── Climbable / passable non-full blocks ─────────────────────────────────────
		table[static_cast<size_t>(BlockId::Ladder)] = false;
		table[static_cast<size_t>(BlockId::Scaffolding)] = false;
		table[static_cast<size_t>(BlockId::Cobweb)] = false;
		table[static_cast<size_t>(BlockId::MangroveRoots)] = false;
		table[static_cast<size_t>(BlockId::BrewingStand)] = false;

		return table;
	}();

	const std::vector<BlockId> BlockState::Flowers = {
		// ── Single-block flowers ─────────────────────────────────────────────────────
		BlockId::Poppy,
		BlockId::Dandelion,
		BlockId::BlueOrchid,
		BlockId::Allium,
		BlockId::AzureBluet,
		BlockId::RedTulip,
		BlockId::OrangeTulip,
		BlockId::WhiteTulip,
		BlockId::PinkTulip,
		BlockId::OxeyeDaisy,
		BlockId::Cornflower,
		BlockId::LilyOfTheValley,
		BlockId::WitherRose,
		BlockId::Torchflower,
		BlockId::OpenEyeblossom,
		BlockId::ClosedEyeblossom,
		BlockId::Wildflowers,

		// ── Double-tall flowers ──────────────────────────────────────────────────────
		BlockId::Sunflower,
		BlockId::Lilac,
		BlockId::RoseBush,
		BlockId::Peony,
		BlockId::PitcherPlant,
	};

	// ----- Constructor / Destructor -----

	BlockState::BlockState(BlockId blockID) : ID(blockID) {}

	BlockState::BlockState(BlockId blockID, uint8_t variantIndex) : ID(blockID), VariantIndex(variantIndex) {}

	bool BlockState::operator==(const BlockState& other) const
	{
		return ID == other.ID && VariantIndex == other.VariantIndex;
	}

	bool BlockState::operator!=(const BlockState& other) const
	{
		return !(*this == other);
	}

	bool BlockState::IsOpaque(BlockId blockID)
	{
		return !IsTransparent(blockID);
	}

	bool BlockState::IsTransparent(BlockId blockID)
	{
		std::shared_lock lock(s_TransparencyLookupTableMutex);
		return s_TransparencyLookupTable[static_cast<size_t>(blockID)];
	}

	bool BlockState::IsSolid(BlockId blockID)
	{
		return s_SolidLookupTable[static_cast<size_t>(blockID)];
	}

	void BlockState::SetTransparency(BlockId blockID, bool transparent)
	{
		std::lock_guard lock(s_TransparencyLookupTableMutex);
		s_TransparencyLookupTable[static_cast<size_t>(blockID)] = transparent;
	}
} // namespace onion::voxel
