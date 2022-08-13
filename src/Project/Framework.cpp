#include "framework.h"
#include <toml++/toml.h>

void Loki::DynamicAnimationCasting::ReadToml(std::filesystem::path path) {
    logger::info("Reading {}...", path.string());
    try {
        const auto tbl = toml::parse_file(path.c_str());
        auto& arr = *tbl.get_as<toml::array>("event");

        for (auto&& elem : arr) {
            auto& eventTable = *elem.as_table();

            logger::info("- - - - - - - - - - - - - - - - - - - - -");

            auto event = eventTable["AnimationEvent"].value<std::string>();
            logger::info("Animation Event -> {}", *event);

            auto raceFormID = eventTable["HasRaceFormID"].value<std::int32_t>();
            logger::info("Race Form ID -> {0:#x}", *raceFormID);
            auto raceEspName = eventTable["RaceEspName"].value<std::string>();
            logger::info("Race ESP name -> {}", *raceEspName);
            std::pair racePair = {*raceFormID, *raceEspName};

            auto actorFormID = eventTable["HasActorFormID"].value<std::int32_t>();
            logger::info("Actor Form ID -> {0:#x}", *actorFormID);
            auto actorEspName = eventTable["ActorEspName"].value<std::string>();
            logger::info("Actor ESP name -> {}", *actorEspName);
            std::pair<RE::FormID, std::string> actorPair = {*actorFormID, *actorEspName};

            auto weapFormID = eventTable["IsEquippedRightFormID"].value<std::int32_t>();
            logger::info("Right Form ID -> {0:#x}", *weapFormID);
            auto weapFormID2 = eventTable["IsEquippedLeftFormID"].value<std::int32_t>();
            logger::info("Left Form ID -> {0:#x}", *weapFormID2);
            auto weapEspName = eventTable["WeaponEspName"].value<std::string>();
            logger::info("Weapon ESP name -> {}", *weapEspName);
            std::pair<RE::FormID, RE::FormID> pair = {*weapFormID, *weapFormID2};
            std::pair<std::string, std::pair<RE::FormID, RE::FormID>> weapPair = {*weapEspName, pair};

            auto weapontype = eventTable["HasWeaponType"].value<std::int32_t>();
            logger::info("Weapon Type -> {}", *weapontype);
            auto weapType = *weapontype;

            auto effectFormID = eventTable["HasEffectFormID"].value<std::int32_t>();
            logger::info("Effect Form ID -> {0:#x}", *effectFormID);
            auto effectEspName = eventTable["EffectEspName"].value<std::string>();
            logger::info("Effect ESP name -> {}", *effectEspName);
            std::pair<RE::FormID, std::string> effectPair = {*effectFormID, *effectEspName};

            auto keywordFormID = eventTable["HasKeywordFormID"].value<std::int32_t>();
            logger::info("Keyword Form ID -> {0:#x}", *keywordFormID);
            auto keywordEspName = eventTable["KeywordEspName"].value<std::string>();
            logger::info("Keyword ESP name -> {}", *keywordEspName);
            std::pair<std::int32_t, std::string> keywordPair = {*keywordFormID, *keywordEspName};

            auto spells = eventTable["SpellFormIDs"].as_array();
            std::unordered_map<std::string, std::vector<std::int32_t>> map = {};
            std::vector<std::int32_t> vector = {};
            if (spells) {
                for (auto& spell : *spells) {
                    logger::info("Spell Form ID -> {0:#x}", *spell.value<std::int32_t>());
                    vector.push_back(*spell.value<std::int32_t>());
                }
                auto spellEspName = eventTable["SpellEspName"].value<std::string>();
                map.insert_or_assign(*spellEspName, vector);
            }
            auto targetPlayer = eventTable["TargetCaster"].value<bool>();
            logger::info("Target Caster -> {}", *targetPlayer);

            auto healthCost = eventTable["HealthCost"].value<float>();
            logger::info("Health Cost -> {}", *healthCost);
            auto staminaCost = eventTable["StaminaCost"].value<float>();
            logger::info("Stamina Cost -> {}", *staminaCost);
            auto magickaCost = eventTable["MagickaCost"].value<float>();
            logger::info("Magicka Cost -> {}", *magickaCost);

            //AnimationCasting::Cast* cast =
            //    new AnimationCasting::Cast(map, racePair, actorPair, weapPair, weapType, effectPair, keywordPair,
            //                               *targetPlayer, *healthCost, *staminaCost, *magickaCost);

            //std::pair<std::string, AnimationCasting::Cast> pair2 = {*event, cast};

            _eventVector.emplace_back(std::piecewise_construct, std::tuple{*event},
                                      std::tuple{std::move(map), racePair, actorPair, weapPair, weapType, effectPair, keywordPair,
                                                 *targetPlayer, *healthCost, *staminaCost, *magickaCost});
        }
        logger::info("Successfully read {}...", path.string());

    } catch (const toml::parse_error& e) {
        logger::error("Error parsing file '{}':\t{}\n\t\t({}:{})", *e.source().path, e.description(),
                      e.source().begin.line, e.source().begin.column);
    } catch (const std::exception& e) {
        logger::error("{}", e.what());
    } catch (...) {
        logger::error("Unknown failure");
    }
}

void Loki::DynamicAnimationCasting::LoadTomls() {
    constexpr auto path = L"Data/SKSE/Plugins/_DynamicAnimationCasting";
    constexpr auto ext = L".toml";

    if (_TomlLoaded) 
        return;

    _TomlLoaded = true;

    auto dataHandle = RE::TESDataHandler::GetSingleton();

    logger::info("Reading all .tomls in file...");

    if (std::filesystem::is_directory(path)) {
        for (const auto& file : std::filesystem::directory_iterator(path)) {
            if (std::filesystem::is_regular_file(file) && file.path().extension() == ext) {
                ReadToml(file.path());
            }
        }
    }

    logger::info("Successfully read all .tomls in file.");
}

void Loki::DynamicAnimationCasting::InstallGraphEventSink() {
    logger::info("Injecting Graph Event Sink Hooks");
    // Install hook on Character/PlayerCharacter's BSTEventSink<RE::BSAnimationGraphEvent> base class
    REL::Relocation<uintptr_t> PCProcessAnimGraphEventVtbl{RE::VTABLE_PlayerCharacter[2]};
    REL::Relocation<uintptr_t> NPCProcessAnimGraphEventVtbl{RE::VTABLE_Character[2]};
    _PCProcessEvent = PCProcessAnimGraphEventVtbl.write_vfunc(0x1, &PCProcessEvent);
    _NPCProcessEvent = NPCProcessAnimGraphEventVtbl.write_vfunc(0x1, &NPCProcessEvent);
}

RE::BSEventNotifyControl Loki::DynamicAnimationCasting::ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this, RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src) {

    if (a_event.tag != NULL && a_event.holder != NULL) {
        auto actorPtr = a_event.holder->As<RE::Actor>();

        for (auto& [tag, caster] : DynamicAnimationCasting::_eventVector) {
            if (a_event.tag == (RE::BSFixedString)tag) {
                logger::info("Event Found: {}", a_event.tag);
                caster.CastSpells(actorPtr);
            }
        }
    }
    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Loki::DynamicAnimationCasting::PCProcessEvent(
    RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this, RE::BSAnimationGraphEvent& a_event,
    RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src) {
    ProcessEvent(a_this, a_event, a_src);
    return _PCProcessEvent(a_this, a_event, a_src);
}

RE::BSEventNotifyControl Loki::DynamicAnimationCasting::NPCProcessEvent(
    RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this, RE::BSAnimationGraphEvent& a_event,
    RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src) {
    ProcessEvent(a_this, a_event, a_src);
    return _NPCProcessEvent(a_this, a_event, a_src);
}
