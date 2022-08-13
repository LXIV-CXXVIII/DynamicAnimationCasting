#pragma once

namespace Loki {

	namespace HUD {
		static void FlashHUDMeter(RE::ActorValue a_av);
	};

	namespace AnimationCasting {

		class Cast {

		public:
			struct Properties {
				std::unordered_map<std::string, std::vector<std::int32_t>> spells = {};
				std::pair<std::int32_t, std::string>                       racePair;
				std::pair<std::int32_t, std::string>                       actorPair;
				std::pair<std::string, std::pair<RE::FormID, RE::FormID>>  weapPair;
				std::int32_t                                               weapType;
				std::pair<std::int32_t, std::string>                       effectPair;
				std::pair<std::int32_t, std::string>                       keywordPair;
				bool  targetCaster = false;
				float healthCost   = 0.00f;
				float staminaCost  = 0.00f;
				float magickaCost  = 0.00f;
			};

			struct Caches {
                RE::TESRace* race = nullptr;
                RE::Actor* actor = nullptr;
                RE::TESObjectWEAP* weapon_r = nullptr;
                RE::TESObjectWEAP* weapon_l = nullptr;
                RE::EffectSetting* effect = nullptr;
                RE::BGSKeyword* keyword = nullptr;
			};

			Cast(
				std::unordered_map<std::string, std::vector<std::int32_t>> a_spells,
				std::pair<std::int32_t, std::string>                       a_racePair,
				std::pair<std::int32_t, std::string>                       a_actorPair,
				std::pair<std::string, std::pair<RE::FormID, RE::FormID>>  a_weapPair,
				std::int32_t                                               a_weapType,
				std::pair<std::int32_t, std::string>                       a_effectPair,
				std::pair<std::int32_t, std::string>                       a_keywordPair,
				bool a_targetCaster, 
				float a_hCost, float a_sCost, float a_mCost
			) {
				_properties.racePair     = std::move(a_racePair);
				_properties.actorPair    = std::move(a_actorPair);
				_properties.weapPair     = std::move(a_weapPair);
				_properties.weapType     = std::move(a_weapType);
				_properties.effectPair   = std::move(a_effectPair);
				_properties.keywordPair  = std::move(a_keywordPair);
				_properties.spells       = std::move(a_spells);
				_properties.targetCaster = a_targetCaster;
				_properties.healthCost   = a_hCost;
				_properties.staminaCost  = a_sCost;
				_properties.magickaCost  = a_mCost;
			}

			void CastSpells(const RE::Actor* a_actor);

		private:
			Properties _properties;
            std::optional<Caches> _caches;
		};

	};

};