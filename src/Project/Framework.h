#pragma once
#include "Project/Casting/DynamicAnimationCasting.h"

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

        static inline bool _TomlLoaded = false;
        static inline std::vector<std::pair<RE::BSFixedString, AnimationCasting::Cast>> _eventVector = {};
    };

};