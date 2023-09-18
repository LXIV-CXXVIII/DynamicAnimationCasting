#pragma once
#include "DynamicAnimationCasting.h"
#include <chrono>

namespace Loki {

    class DynamicAnimationCasting {
    
    public:

        static void ReadToml(std::filesystem::path path);
        static void LoadTomls();

        static void InstallGraphEventSink();

        struct MagicCastHook {
            static uint64_t CastSpellImmediate(RE::MagicCaster* a_caster, RE::MagicItem* a_spell, bool unk1_bool1,
                                               RE::TESObjectREFR* a_target, float a_magOverride, bool unk2_bool2,
                                               float unk3, RE::TESObjectREFR* a_blameTarget) {
                if (a_spell == ReplacingSpell && ReplacementSpell) {
                    a_spell = ReplacementSpell;
                }
                auto ret = BaseFunction(a_caster, a_spell, unk1_bool1, a_target, a_magOverride, unk2_bool2, unk3,
                                        a_blameTarget);
                return ret;
            }

            static inline RE::MagicItem* ReplacingSpell = nullptr;
            static inline RE::MagicItem* ReplacementSpell = nullptr;

            static inline REL::Relocation<decltype(CastSpellImmediate)> BaseFunction;

            static void Install() {
                REL::Relocation<uintptr_t> CasterVtbl{RE::VTABLE_MagicCaster[0]};
                REL::Relocation<uintptr_t> ActorCasterVtbl{RE::VTABLE_ActorMagicCaster[0]};
                REL::Relocation<uintptr_t> NonActorCasterVtbl{RE::VTABLE_NonActorMagicCaster[0]};

                BaseFunction = CasterVtbl.write_vfunc(0x1, &CastSpellImmediate);
                BaseFunction = ActorCasterVtbl.write_vfunc(0x1, &CastSpellImmediate);
                BaseFunction = ActorCasterVtbl.write_vfunc(0x1, &CastSpellImmediate);
            }
        };

        static void ReplaceMagicCasterSpell(RE::MagicCaster* caster, RE::MagicItem* replacing, RE::MagicItem* replacement);

        static RE::BSEventNotifyControl ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this,
                                                     RE::BSAnimationGraphEvent& a_event,
                                                     RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src);

        static inline REL::Relocation<decltype(ProcessEvent)> _PCProcessEvent;
        static inline REL::Relocation<decltype(ProcessEvent)> _NPCProcessEvent;

        static RE::BSEventNotifyControl PCProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this,
                                                       RE::BSAnimationGraphEvent& a_event,
                                                       RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src);

        static RE::BSEventNotifyControl NPCProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this,
                                                        RE::BSAnimationGraphEvent& a_event,
                                                        RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src);

        static constexpr int MaxExclusiveGroups = 256;
        using GroupCounterType = std::bitset<MaxExclusiveGroups>;
        static inline bool TomlLoaded = false;
        static inline std::vector<AnimationCasting::CastTrigger> CastTriggers;
        static inline std::unordered_map<std::string, int> ExclusiveGroups;
        static inline RE::BSTHashMap<RE::BSFixedString, RE::MagicItem*> CustomSpells;
        static inline int MagicFavouriteIndex = 0;
        // For trigger cooldown
        using clock = std::chrono::high_resolution_clock;
        static inline std::unordered_map<std::int64_t, clock::time_point> TriggerTimes;
        static bool UpdateTriggerCooldown(float cooldown, const AnimationCasting::CastTrigger* trigger, const RE::Actor* actor);

        static int SetMagicFavourite(int Index);
        static int NextMagicFavourite(int Delta);
        static void ResetCustomSpells();
		static bool RegisterCustomSpell(RE::BSFixedString a_name, RE::MagicItem* a_spell);

        /**
         * The serialization handler for reverting game state.
         *
         * <p>
         * This is called as the handler for revert. Revert is called by SKSE on a plugin that is registered for
         * serialization handling on a new game or before a save game is loaded. It should be used to revert the state
         * of the plugin back to its default.
         * </p>
         */
        static void OnRevert(SKSE::SerializationInterface*);

        /**
         * The serialization handler for saving data to the cosave.
         *
         * @param serde The serialization interface used to write data.
         */
        static void OnGameSaved(SKSE::SerializationInterface* serde);

        /**
         * The serialization handler for loading data from a cosave.
         *
         * @param serde  The serialization interface used to read data.
         */
        static void OnGameLoaded(SKSE::SerializationInterface* serde);

    };

};