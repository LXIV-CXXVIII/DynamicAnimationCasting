#pragma once
#include "DynamicAnimationCasting.h"
#include <chrono>

namespace Loki {

    class DynamicAnimationCasting {
    
    public:

        static void ReadToml(std::filesystem::path path);
        static void LoadTomls();

        static void InstallGraphEventSink();

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

        static inline bool TomlLoaded = false;
        static inline std::vector<AnimationCasting::CastTrigger> CastTriggers;
        static inline RE::BSTHashMap<RE::BSFixedString, RE::MagicItem*> CustomSpells;
        static inline int MagicFavouriteIndex = 0;
        // For trigger cooldown
        using clock = std::chrono::high_resolution_clock;
        static inline std::unordered_map<std::int64_t, clock::time_point> TriggerTimes;
        static bool UpdateTriggerCooldown(float cooldown, const AnimationCasting::CastTrigger* trigger, const RE::Actor* actor);

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