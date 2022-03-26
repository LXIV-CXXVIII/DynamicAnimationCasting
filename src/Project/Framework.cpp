#include "framework.h"

void Loki::DynamicAnimationCasting::InstallGraphEventSink2ElectricBoogaloo() {
    REL::Relocation<uintptr_t> npcPtr{ REL::ID(261399) };
    REL::Relocation<uintptr_t> pcPtr{ REL::ID(261918) };

    fProcessEvent f0 = SafeWrite64Function(npcPtr.address() + 0x8, &DynamicAnimationCasting::HookedProcessEvent);
    fHash.insert(std::pair<std::uint64_t, fProcessEvent>(npcPtr.address(), f0));
    fProcessEvent f1 = SafeWrite64Function(pcPtr.address() + 0x8, &DynamicAnimationCasting::HookedProcessEvent);
    fHash.insert(std::pair<std::uint64_t, fProcessEvent>(pcPtr.address(), f1));

    logger::info("graphEventSink Hooks Injected");
}



RE::BSEventNotifyControl Loki::DynamicAnimationCasting::HookedProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src) {
    fProcessEvent fn = fHash.at(*(std::uintptr_t*)this);
    if (a_event.tag != NULL && a_event.holder != NULL) {
        auto actorPtr = a_event.holder->As<RE::Actor>();
        auto event = DynamicAnimationCasting::_eventMap.find((std::string)a_event.tag);
        if (event != DynamicAnimationCasting::_eventMap.end()) {
            logger::info("Event Found: {}", a_event.tag);
            event->second->CastSpells(actorPtr);
        } 
    }
    return fn ? (this->*fn)(a_event, a_src) : RE::BSEventNotifyControl::kContinue;

}