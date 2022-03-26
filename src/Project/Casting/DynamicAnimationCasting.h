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
				std::pair<std::int32_t, std::string> racePair;
				std::pair<std::int32_t, std::string> actorPair;
				std::pair<std::int32_t, std::string> weapPair;
				std::uint32_t                        weapType;
				std::pair<std::int32_t, std::string> effectPair;
				std::pair<std::int32_t, std::string> keywordPair;
				bool  targetCaster = false;
				float healthCost   = 0.00f;
				float staminaCost  = 0.00f;
				float magickaCost  = 0.00f;
			};

			Cast(
				std::unordered_map<std::string, std::vector<std::int32_t>> a_spells,
				std::pair<std::int32_t, std::string> a_racePair,
				std::pair<std::int32_t, std::string> a_actorPair,
				std::pair<std::int32_t, std::string> a_weapPair,
				std::uint32_t                        a_weapType,
				std::pair<std::int32_t, std::string> a_effectPair,
				std::pair<std::int32_t, std::string> a_keywordPair,
				bool a_targetCaster, 
				float a_hCost, float a_sCost, float a_mCost
			) {
				_properties.racePair     = a_racePair;
				_properties.actorPair    = a_actorPair;
				_properties.weapPair     = a_weapPair;
				_properties.weapType     = a_weapType;
				_properties.effectPair   = a_effectPair;
				_properties.keywordPair  = a_keywordPair;
				_properties.spells       = a_spells;
				_properties.targetCaster = a_targetCaster;
				_properties.healthCost   = a_hCost;
				_properties.staminaCost  = a_sCost;
				_properties.magickaCost  = a_mCost;
			}
			virtual ~Cast() {
			
			}

			void CastSpells(const RE::Actor* a_actor);

		private:
			Properties _properties;

		};

	};

};