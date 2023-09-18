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

        inline bool IsInvalidFormID(RE::FormID id) noexcept { return id == 0 || id == static_cast<decltype(id)>(-1); };

        inline bool CheckFormID(RE::FormID required, auto getter) {
            return IsInvalidFormID(required) || required == getter();
        };

        static inline auto GetSpellArchetype(RE::MagicItem* spell) -> RE::EffectSetting::Archetype {
            if (auto effect = spell->GetCostliestEffectItem()) {
                if (auto baseEffect = effect->baseEffect) {
                    return baseEffect->GetArchetype();
                }
            }
            return RE::EffectSetting::Archetype::kNone;
        };

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
                return !IsInvalidFormID(formid) || type >= 0 || keyword || !IsInvalidFormID(enchant); 
            }
        };

		struct CastTrigger {
            // event tag for triggering this spell
            RE::BSFixedString tag;

            // Conditions
            RE::FormID caster = 0;
            RE::FormID race = 0;
            RE::FormID effect = 0;
            RE::BGSPerk* perk = nullptr;
            RE::BGSKeyword* keyword = nullptr;
            std::optional<bool> isOnMount;
            std::optional<bool> isRunning;
            std::optional<bool> isSneaking;
            std::optional<bool> isSprinting;
            WeaponTrigger weapons[2];  // right, left
            float chance = 1.0f;
            int group = -1;

            // Properties
            float   cooldown = 0.f;
            bool    targetCaster = false;
            bool    useWeaponCast = false;
            bool    dualCasting = false;
            float   healthCost = 0.f;
            float   staminaCost = 0.f;
            float   magickaCost = 0.f;
            float   effectiveness = 1.f;
            float   baseMagnitude = 1.f;
            float   castMagickaCostFactor = 1.f;

            // Spell Filters
            bool castOnlyOneSpell = false;
            bool castOnlyKnownSpell = false;
            bool ignoreConcentrationSpell = false;
            bool ignoreBoundWeapon = false;
            bool replaceCastingSpell = false;

            // Spells
            bool castForeHandSpell = false;
            bool castOffHandSpell = false;
            bool castEquipedPower = false;
            bool castFaviouriteMagic = false;
            //!!! BSTSmallArray is neither copiable or movable
            RE::BSTSmallArray<RE::MagicItem*,1> spells;
            RE::BSTSmallArray<RE::BSFixedString, 1> customSpells;

            bool HasWeaponConstraints() const { return weapons[0].Constrainted() || weapons[1].Constrainted(); }
            bool IsAllowedSpell(const RE::Actor* actor, RE::MagicItem* spell) const {
                return  spell &&
                        (!castOnlyKnownSpell || actor->HasSpell(spell->As<RE::SpellItem>())) &&
                        (!ignoreConcentrationSpell || spell->GetCastingType() != RE::MagicSystem::CastingType::kConcentration) &&
                        (!ignoreBoundWeapon || GetSpellArchetype(spell) != RE::EffectSetting::Archetype::kBoundWeapon);
            }

			bool Invoke(const RE::Actor* a_caster);

            std::string ToString() const;
		};

	};

};