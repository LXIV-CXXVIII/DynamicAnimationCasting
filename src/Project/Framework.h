#pragma once
#include "Project/Casting/DynamicAnimationCasting.h"
#include <toml++/toml.h>

namespace Loki {

    class DynamicAnimationCasting {
    
    public:
        DynamicAnimationCasting() {
            constexpr auto path = L"Data/SKSE/Plugins/_DynamicAnimationCasting";
            constexpr auto ext = L".toml";

            auto dataHandle = RE::TESDataHandler::GetSingleton();

            const auto readToml = [&](std::filesystem::path path) {
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
                            std::pair<RE::FormID, std::string> racePair = { *raceFormID, *raceEspName };

                        auto actorFormID = eventTable["HasActorFormID"].value<std::int32_t>();
                          logger::info("Actor Form ID -> {0:#x}", *actorFormID);
                        auto actorEspName = eventTable["ActorEspName"].value<std::string>();
                          logger::info("Actor ESP name -> {}", *actorEspName);
                            std::pair<RE::FormID, std::string> actorPair = { *actorFormID, *actorEspName };

                        auto weapFormID = eventTable["IsEquippedRightFormID"].value<std::int32_t>();
                          logger::info("Right Form ID -> {0:#x}", *weapFormID);
                        auto weapFormID2 = eventTable["IsEquippedLeftFormID"].value<std::int32_t>();
                          logger::info("Left Form ID -> {0:#x}", *weapFormID2);
                        auto weapEspName = eventTable["WeaponEspName"].value<std::string>();
                          logger::info("Weapon ESP name -> {}", *weapEspName);
                            std::pair<RE::FormID, RE::FormID> pair = { *weapFormID, *weapFormID2 };
                            std::pair<std::string, std::pair<RE::FormID, RE::FormID>> weapPair = { *weapEspName, pair };

                        auto weapontype = eventTable["HasWeaponType"].value<std::int32_t>();
                          logger::info("Weapon Type -> {}", *weapontype);
                        auto weapType = *weapontype;

                        auto effectFormID = eventTable["HasEffectFormID"].value<std::int32_t>();
                          logger::info("Effect Form ID -> {0:#x}", *effectFormID);
                        auto effectEspName = eventTable["EffectEspName"].value<std::string>();
                          logger::info("Effect ESP name -> {}", *effectEspName);
                            std::pair<RE::FormID, std::string> effectPair = { *effectFormID, *effectEspName };

                        auto keywordFormID = eventTable["HasKeywordFormID"].value<std::int32_t>();
                          logger::info("Keyword Form ID -> {0:#x}", *keywordFormID);
                        auto keywordEspName = eventTable["KeywordEspName"].value<std::string>();
                          logger::info("Keyword ESP name -> {}", *keywordEspName);
                            std::pair<std::int32_t, std::string> keywordPair = { *keywordFormID, *keywordEspName };

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

                        AnimationCasting::Cast* cast = new AnimationCasting::Cast(
                            map, 
                            racePair,
                            actorPair,
                            weapPair,
                            weapType,
                            effectPair,
                            keywordPair,
                            *targetPlayer, 
                            *healthCost, 
                            *staminaCost, 
                            *magickaCost
                        );

                        _eventMap.insert({*event, cast});

                    }
                    logger::info("Successfully read {}...", path.string());

                } catch (const toml::parse_error& e) {
                    std::ostringstream ss;
                    ss << "Error parsing file \'" << *e.source().path << "\':\n"
                        << '\t' << e.description() << '\n'
                        << "\t\t(" << e.source().begin << ')';
                    logger::error(ss.str());
                } catch (const std::exception& e) {
                    logger::error("{}", e.what());
                } catch (...) {
                    logger::error("Unknown failure"sv);
                }
            };

            logger::info("Reading all .tomls in file...");

            if (std::filesystem::is_directory(path)) {
                for (const auto& file : std::filesystem::directory_iterator(path)) {
                    if (std::filesystem::is_regular_file(file) && file.path().extension() == ext) {
                        readToml(file.path());
                    }
                }
            }

            logger::info("Successfully read all .tomls in file.");
        }
        virtual ~DynamicAnimationCasting() {
        
        }
        static DynamicAnimationCasting* GetSingleton() {
            static DynamicAnimationCasting singleton;
            return &singleton;
        }
        template<class Ty>
        Ty SafeWrite64Function(uintptr_t addr, Ty data) {
            DWORD oldProtect;
            void* _d[2];
            memcpy(_d, &data, sizeof(data));
            size_t len = sizeof(_d[0]);

            VirtualProtect((void*)addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
            Ty olddata;
            memset(&olddata, 0, sizeof(Ty));
            memcpy(&olddata, (void*)addr, len);
            memcpy((void*)addr, &_d[0], len);
            VirtualProtect((void*)addr, len, oldProtect, &oldProtect);
            return olddata;
        }
        typedef RE::BSEventNotifyControl(DynamicAnimationCasting::* fProcessEvent)(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* dispatcher);
        RE::BSEventNotifyControl HookedProcessEvent(RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* src);

        void InstallGraphEventSink2ElectricBoogaloo();

        static inline std::unordered_map<std::string, AnimationCasting::Cast*> _eventMap = {};
        static inline std::unordered_map<std::string, std::vector<std::int32_t>> _spellMap = {};

    private:

    protected:
        static inline std::unordered_map<uint64_t, fProcessEvent> fHash;
    
    };

};