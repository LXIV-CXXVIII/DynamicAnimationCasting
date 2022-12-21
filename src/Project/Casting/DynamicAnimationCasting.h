#pragma once

namespace Loki {

	namespace HUD {
		static void FlashHUDMeter(RE::ActorValue a_av);
	};

	namespace AnimationCasting {
		using FormIdentifier = std::pair<std::int32_t, std::string>;
        static constexpr int8_t kSpell = std::to_underlying(RE::WEAPON_TYPE::kCrossbow) + 1;
        static constexpr int8_t kShield = std::to_underlying(RE::WEAPON_TYPE::kCrossbow) + 2;
        static constexpr int8_t kTorch = std::to_underlying(RE::WEAPON_TYPE::kCrossbow) + 3;

        struct WeaponTrigger {
            RE::FormID formid = 0;
            std::int8_t type = -1;
            RE::BGSKeyword* keyword = nullptr;
            RE::FormID enchant = 0;
            float enchantCost = 0.f;
            float enchantCostFactor = 0.f;
            float enchantMagnitudeFactor = 0.f;
            bool cast = true;

            bool Constrainted() const noexcept {
                return formid >0 || type >=0 || keyword || enchant > 0; 
            }
        };

		struct CastTrigger {
            RE::BSFixedString tag;  // event tag for triggering this spell

            // Conditions
            RE::FormID caster = 0;
            RE::FormID race = 0;
            RE::FormID effect = 0;
            RE::BGSPerk* perk = nullptr;
            RE::BGSKeyword* keyword = nullptr;
            std::optional<bool> isOnMount;
            std::optional<bool> isRunning;
            std::optional<bool> isSneaking;
            WeaponTrigger weapons[2];  // right, left
            float chance = 1.0f;

            // Properties
            bool targetCaster = false;
            bool useWeaponCast = false;
            bool castOffHandSpell = false;
            bool castForeHandSpell = false;
            bool castEquipedPower = false;
            bool castFaviouriteMagic = false;
            bool dualCasting = false;
            float healthCost = 0.f;
            float staminaCost = 0.f;
            float magickaCost = 0.f;
            float effectiveness = 1.f;
            float baseMagnitude = 1.f;
            float castMagickaCostFactor = 1.f;

            std::string spells_buffer;  // small_vector<RE::SpellItem*>, using std::string as the underlying data

            bool HasWeaponConstraints() const { return weapons[0].Constrainted() || weapons[1].Constrainted(); }

            auto Spells() {
                return std::span {reinterpret_cast<RE::MagicItem**>(spells_buffer.data()), spells_buffer.size() / sizeof(RE::MagicItem*)};
            }

			void CastSpells(const RE::Actor* a_caster);
		};

	};

};