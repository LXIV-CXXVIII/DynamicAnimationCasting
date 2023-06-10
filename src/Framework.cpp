#include "PCH.h"
#include "framework.h"
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>
#include "Expression.h"

void Loki::DynamicAnimationCasting::ReadToml(std::filesystem::path path) {
    logger::info("Reading {}...", path.string());

    using namespace std::string_literals;

    static const std::pair<std::string, int> noformid;
    const std::string empty_string;

    auto handle = RE::TESDataHandler::GetSingleton();

    auto ParseFormID2 = [handle](toml::node_view<const toml::node> id_node, std::string_view esp = {}) -> RE::FormID {
        auto ParseInteger = [](std::string_view str, int default_value) -> int {
            int base = 10;
            if (str.starts_with("0x"sv) || str.starts_with("0X"sv)) {
                base = 16;
                str.remove_prefix(2);
            }
            int value = default_value;
            const char* last = str.data() + str.size();
            auto [ptr, ec] = std::from_chars(str.data(), last, value, base);
            if (ptr != last) {
                logger::warn("Failed to parse integer from {}", str);
            }
            return value;
        };

        int id = -1;
        if (id_node.is_string()) {
            // Parse "ModeName.esp|0xAAAA"
            std::string strval = id_node.as_string()->get();
            std::string_view expr = strval;
            int mid = expr.find('|');
            if (mid != std::string::npos) {
                esp = expr.substr(0, mid);
                expr = expr.substr(mid + 1);
                id = ParseInteger(expr, -1);
                if (id > -1) {
                    return handle->LookupFormID(id, esp);
                }
            } else if (!expr.empty() && std::isdigit(expr[0])) {
                // Parse ""0xAAAA"", when user mistakenly write integer as string
                id = ParseInteger(expr, -1);
                if (id > -1) {
                    return handle->LookupFormID(id, esp);
                }
            }
        } else {
            id = id_node.value_or(-1);
            if (!esp.empty() && id > -1) {
                return handle->LookupFormID(id, esp);
            }
        }
        return -1;
    };

    auto ParseFormID = [&](const toml::table& parent, std::string_view id_key, std::string_view esp_key) -> RE::FormID {
        std::string esp = parent[esp_key].value_or(std::string{});
        return ParseFormID2(parent[id_key], esp);
    };

    auto ParseWeaponType = [](toml::node_view<const toml::node> node) -> std::int8_t {
        if (auto int_node = node.as_integer()) {
            return int_node->get();
        } else if (auto str_node = node.as_string()) {
            std::string str = str_node->get();
            for (auto& c : str) {
                c = static_cast<char>(std::tolower(c));
            }
            static constexpr std::string_view weapon_types[] = {
		        "handtohandmelee"sv,
		        "onehandsword"sv,
		        "onehanddagger"sv,
		        "onehandaxe"sv,
		        "onehandmace"sv,
		        "twohandsword"sv,
		        "twohandaxe"sv,
		        "bow"sv,
		        "staff"sv,
		        "crossbow"sv,
		        "spell"sv,
		        "shield"sv,
		        "torch"sv
            };
            auto itr = std::find(std::begin(weapon_types), std::end(weapon_types), str);
            if (itr != std::end(weapon_types)) {
                return itr - std::begin(weapon_types);
            }
        }
        return -1;
    };

    auto ParseOptionalBool = [](toml::node_view<const toml::node> node) -> std::optional<bool> {
        if (auto boolnode = node.as_boolean()) {
            return boolnode->get();
        } else {
            return {};
        }
    };

    toml::parse_result file;
    try {
        file = toml::parse_file(path.c_str());
    } catch (const toml::parse_error& e) {
        logger::error("Error parsing file '{}':\t{}\n\t\t({}:{})", *e.source().path, e.description(),
                      e.source().begin.line, e.source().begin.column);
        return;
    } 
    
    CastTriggers.reserve(1024);
    logger::info("Start : {}", path.string());
    if (auto arr = file.table().get_as<toml::array>("event")) {
        for (const auto& elem : *arr) {
            const auto& event = *elem.as_table();
            using std::string;

            if (CastTriggers.size() == CastTriggers.capacity()) {
                logger::error("Number of triggers overflow (>1024), ignore more triggers...");
                break;
            }

            auto& trigger = CastTriggers.emplace_back();
            trigger.tag = event["AnimationEvent"].value_or(empty_string);
            logger::info("- Cast Trigger [{}]", trigger.tag);

            auto exGroupName = event["ExclusiveGroup"].value_or(empty_string);
            if (!exGroupName.empty()) {
                trigger.group = ExclusiveGroups.try_emplace(std::move(exGroupName), static_cast<int>(exGroupName.size())).first->second;
                if (trigger.group > MaxExclusiveGroups) {
                    logger::error("Number of exclusive groups overflow (>{}), ignore group {}...", MaxExclusiveGroups, exGroupName);
                    trigger.group = -1;
                }
            }

            trigger.race = ParseFormID(event, "HasRaceFormID", "RaceEspName");
            trigger.effect = ParseFormID(event, "HasEffectFormID", "EffectEspName");
            trigger.caster = ParseFormID(event, "HasActorFormID", "ActorEspName");
            trigger.perk = RE::TESForm::LookupByID<RE::BGSPerk>(ParseFormID(event, "HasPerkFormID", "PerkEspName"));
            trigger.keyword = RE::TESForm::LookupByID<RE::BGSKeyword>(ParseFormID(event, "HasKeywordFormID", "KeywordEspName"));
            trigger.isOnMount = ParseOptionalBool(event["IsOnMount"]);
            trigger.isSneaking = ParseOptionalBool(event["IsSneaking"]);
            trigger.isRunning = ParseOptionalBool(event["IsRunning"]);
            trigger.weapons[0].formid = ParseFormID(event, "IsEquippedRightFormID", "WeaponEspName");
            trigger.weapons[1].formid = ParseFormID(event, "IsEquippedLeftFormID", "WeaponEspName");
            trigger.weapons[0].enchant = ParseFormID(event, "HasWeaponEnchantEffect", "WeaponEnchantEffectEspName");
            trigger.weapons[1].enchant = trigger.weapons[0].enchant;
            trigger.weapons[0].type = ParseWeaponType(event["HasWeaponType"]);
            trigger.weapons[1].type = trigger.weapons[0].type;
            trigger.weapons[1].enchant = trigger.weapons[0].enchant;
            trigger.weapons[0].keyword = RE::TESForm::LookupByID<RE::BGSKeyword>(ParseFormID(event, "HasWeaponKeyword", "KeywordEspName"));
            trigger.weapons[1].keyword = trigger.weapons[0].keyword;
            trigger.weapons[0].cast = event["CastFromWeapon"].value_or(trigger.weapons[0].Constrainted());
            trigger.weapons[1].cast = event["CastFromWeapon"].value_or(trigger.weapons[1].Constrainted());
            trigger.weapons[0].enchantMagnitudeFactor = event["WeaponEnchantMagnitudeFactor"].value_or(1.f);
            trigger.weapons[1].enchantMagnitudeFactor = trigger.weapons[0].enchantMagnitudeFactor;
            trigger.cooldown = event["Cooldown"].value_or(0.f);
            trigger.chance = event["Chance"].value_or(1.f);
            trigger.castOnlyOneSpell = event["CastOnlyFirstSpell"].value_or(false);
            trigger.castOnlyKnownSpell = event["CastOnlyKnownSpell"].value_or(false);
            trigger.replaceCastingSpell = event["ReplaceCastingSpell"].value_or(false);
            trigger.ignoreConcentrationSpell = event["IgnoreConcentrationSpell"].value_or(false);
            trigger.ignoreBoundWeapon = event["IgnoreBoundWeaponSpell"].value_or(false);
            trigger.targetCaster = event["TargetCaster"].value_or(false);
            trigger.dualCasting = event["DualCasting"].value_or(false);
            trigger.castOffHandSpell = event["CastOffHandSpell"].value_or(false);
            trigger.castForeHandSpell = event["CastForeHandSpell"].value_or(false);
            trigger.castEquipedPower = event["CastEquipedPower"].value_or(false);
            trigger.castFaviouriteMagic = event["CastFaviouriteMagic"].value_or(false);
            trigger.effectiveness = event["Effectiveness"].value_or(1.f);
            trigger.healthCost = event["HealthCost"].value_or(0.f);
            trigger.magickaCost = event["MagickaCost"].value_or(0.f);
            trigger.staminaCost = event["StaminaCost"].value_or(0.f);
            trigger.castMagickaCostFactor = event["CastMagickaCostFactor"].value_or(1.f);
            trigger.baseMagnitude = event["Magnitude"].value_or(1.f);

            auto spellEsp = event["SpellEspName"].value_or(empty_string);
            if (auto spellFormIDs = event["SpellFormIDs"].as_array()) {
                for (auto& spellNode : *spellFormIDs) {
                    bool added = false;
                    if (auto spellName = spellNode.as_string(); spellName && spellName->get().starts_with('@')) {
                        auto customSpellName = spellName->get();
                        if (customSpellName.starts_with('@')) {
                            customSpellName = customSpellName.substr(1);
                            static constexpr std::array<std::string_view,4> SpecialSpellNames = {
                                "FOREHAND",
                                "OFFHAND",
                                "POWER",
                                "FAVOURITE"
                            };
                            if (auto itr = std::find(SpecialSpellNames.begin(), SpecialSpellNames.end(), customSpellName); itr != SpecialSpellNames.end()) {
                                int index = itr - SpecialSpellNames.begin();
                                switch (index) {
                                    case 0:
                                        trigger.castForeHandSpell = true;
                                        break;
                                    case 1:
                                        trigger.castOffHandSpell = true;
                                        break;
                                    case 2:
                                        trigger.castEquipedPower = true;
                                        break;
                                    case 3:
                                        trigger.castFaviouriteMagic = true;
                                        break;
                                }
                                added = true;
                                logger::info("Special Spell {}", customSpellName);
                            } else {
                                trigger.customSpells.emplace_back(customSpellName);
                                CustomSpells.emplace(trigger.customSpells.back(), nullptr);
                                added = true;
                                logger::info("Custom Spell {}", customSpellName);
                            }
                        }
                    } else {
                        RE::FormID formid = ParseFormID2(toml::node_view{spellNode}, spellEsp);
                        if (auto magicitem = RE::TESForm::LookupByID<RE::MagicItem>(formid)) {
                            logger::info("Spell {}", magicitem->GetName());
                            trigger.spells.push_back(magicitem);
                            added = true;
                        }
                    }
                    if (!added) {
                        logger::error("Invalid Spell {}", spellEsp);
                    }
                }
            }

            if (trigger.spells.empty() && 
                trigger.customSpells.empty() && 
                !trigger.castOffHandSpell &&
                !trigger.castEquipedPower &&
                !trigger.castForeHandSpell && 
                !trigger.castFaviouriteMagic) {
                logger::warn("No spells on the trigger, ignored");
                CastTriggers.pop_back();
            }
        }
        logger::info("Complete : {}\n", path.string());
    }
}

void Loki::DynamicAnimationCasting::LoadTomls() {
    constexpr auto path = L"Data/SKSE/Plugins/_DynamicAnimationCasting";
    constexpr auto ext = L".toml";

    if (TomlLoaded) 
        return;

    TomlLoaded = true;

    logger::info("Reading all .tomls in file...");
    ExclusiveGroups.clear();
    CastTriggers.clear();

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
    // logger::warn("ChracterUpdate offset = {:#x}, base = {:#x}", RELOCATION_ID(39375, 40447).offset(), (size_t)REL::Module::get().base());
    REL::Relocation<uintptr_t> PCProcessAnimGraphEventVtbl{RE::VTABLE_PlayerCharacter[2]};
    REL::Relocation<uintptr_t> NPCProcessAnimGraphEventVtbl{RE::VTABLE_Character[2]};
    _PCProcessEvent = PCProcessAnimGraphEventVtbl.write_vfunc(0x1, &PCProcessEvent);
    _NPCProcessEvent = NPCProcessAnimGraphEventVtbl.write_vfunc(0x1, &NPCProcessEvent);
}

void Loki::DynamicAnimationCasting::ReplaceMagicCasterSpell(RE::MagicCaster* caster, RE::MagicItem* replacing,
                                                            RE::MagicItem* replacement) {
    MagicCastHook::ReplacingSpell = replacing;
    MagicCastHook::ReplacementSpell = replacement;
}

static thread_local int InProcessEvent = 0;

struct ProcessEventRecursionGaurd {
    ProcessEventRecursionGaurd() { 
        InProcessEvent++;
    }
    ~ProcessEventRecursionGaurd() {
        InProcessEvent--;
    }
};

RE::BSEventNotifyControl Loki::DynamicAnimationCasting::ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this, RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src) {
    
    // MRh_SpellFire_Event may be fired by CastSpellImmediate
    // Prevent infinite recursion in this case
    if (InProcessEvent > 0) {
        return RE::BSEventNotifyControl::kContinue;
    }

    ProcessEventRecursionGaurd Guard;

    constexpr auto IsValidGroup = [](int group) -> bool { return group >= 0 && group < MaxExclusiveGroups; };

    const RE::Actor* actor = nullptr;
    if (!a_event.tag.empty() && a_event.holder && (actor = a_event.holder->As<RE::Actor>())) {
        #ifdef _DEBUG
        logger::info("{} {}", a_event.tag.c_str(), a_event.payload.c_str());
        #endif
        GroupCounterType groups;
        for (auto& trigger : DynamicAnimationCasting::CastTriggers) {
            // BSFixedString::operator== is just a pointer compare, super efficient
            if (a_event.tag == trigger.tag) {
                // logger::info("Event Found: {}", a_event.tag);
                if (!IsValidGroup(trigger.group) || !groups.test(trigger.group)) {
                    bool triggered = trigger.Invoke(actor);
                    if (triggered && IsValidGroup(trigger.group)) {
                        groups.set(trigger.group);
                    }
                }
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

void Loki::DynamicAnimationCasting::ResetCustomSpells() {
    CustomSpells.clear();
    // Register fireball as the TEST spell
    CustomSpells.emplace("TEST"sv, nullptr);
    auto Fireball = RE::TESDataHandler::GetSingleton()->LookupForm<RE::SpellItem>(0x7D997, "Skyrim.esm"sv);
    RegisterCustomSpell("TEST"sv, Fireball);
}

bool Loki::DynamicAnimationCasting::UpdateTriggerCooldown(float cooldown,
                                                          const AnimationCasting::CastTrigger* trigger,
                                                          const RE::Actor* actor) 
{
    if (!actor) return false;
    std::uint64_t triggerKey = (trigger - CastTriggers.data()) & 0xffffffff;
    std::uint64_t actorKey = actor->formID & 0xffffffff;
    std::uint64_t key = triggerKey << 32 | actorKey;
    auto currentTime = clock::now();
    auto& lastTime = TriggerTimes[key];
    std::chrono::duration<float> _cooldown(cooldown);
    if (currentTime - lastTime > _cooldown) {
        lastTime = currentTime;
        return true;
    }
    return false;
}

bool Loki::DynamicAnimationCasting::RegisterCustomSpell(RE::BSFixedString a_name, RE::MagicItem* a_spell) {
    if (auto itr = CustomSpells.find(a_name); itr != CustomSpells.end()) {
        itr->second = a_spell;
        return true;
    }
    return false;
}

int Loki::DynamicAnimationCasting::SetMagicFavourite(int Index) {
    const auto& favSpells = RE::MagicFavorites::GetSingleton()->spells;
    if (Index < 0 || Index >= favSpells.size()) {
        MagicFavouriteIndex = -1;
        RE::DebugNotification("Selected Spell : None");
    } else {
        MagicFavouriteIndex = Index;
        auto message = fmt::format("Selected Spell : {}", favSpells[Index]->GetName());
        RE::DebugNotification(message.c_str());
    }
    return MagicFavouriteIndex;
}

int Loki::DynamicAnimationCasting::NextMagicFavourite(int Delta) {
    logger::info("NextMagicFavourite");
    const auto& favSpells = RE::MagicFavorites::GetSingleton()->spells;
    const int nFavSpells = static_cast<int>(favSpells.size()) + 1;
    for (int i = Delta; std::abs(i) < nFavSpells; i += (Delta < 0 ? -1 : +1)) {
        int favIndex = (DynamicAnimationCasting::MagicFavouriteIndex + i + nFavSpells) % nFavSpells;
        if (favIndex == nFavSpells - 1 || favSpells[favIndex]->As<RE::MagicItem>()) 
        {
            SetMagicFavourite(favIndex);
            break;
        }
    }
    return MagicFavouriteIndex;
}

// These four-character record types, which store data in the SKSE cosave, are little-endian. That means they are
// reversed from the character order written here. Using the byteswap functions reverses them back to the order
// the characters are written in the source code.
inline const auto CustomSpellRecord = _byteswap_ulong('DYAC');

void Loki::DynamicAnimationCasting::OnRevert(SKSE::SerializationInterface*)
{
    ResetCustomSpells();
    // TriggerTimes.clear();
    MagicFavouriteIndex = 0;
}

void Loki::DynamicAnimationCasting::OnGameSaved(SKSE::SerializationInterface* serde) {
    // To write data open a record with a given name. The name must be unique within your mod, and has a version number
    // assigned (in this case 0). You can increase the version number if making breaking format change from a previous
    // version of your mod, so that the load handler can know which version of the format is being used.
    if (!serde->OpenRecord(CustomSpellRecord, 0)) {
        logger::error("Unable to open record to write cosave data.");
        return;
    }
    // Write the favourite index
    serde->WriteRecordData(MagicFavouriteIndex);

    // First, write the number of items that will be written in this record. That way when we load the data, we know how
    // many times to iterate reading in items.
    int customSpellSize = CustomSpells.size();
    serde->WriteRecordData(customSpellSize);
    for (auto& [name, spell] : CustomSpells) {
        int nameSize = name.size();
        serde->WriteRecordData(nameSize);
        serde->WriteRecordData(name.c_str(), sizeof(char) * name.size());
        serde->WriteRecordData(spell ? spell->formID : 0);
    }
}

[[nodiscard]] inline RE::FormID ReadFormID(SKSE::SerializationInterface* serde) {
    RE::FormID formId;
    serde->ReadRecordData(formId);
    // When reading back a form ID from a save, that form ID may no longer be valid because the user's
    // load order may have changed. During the load call, it is possible to use
    // <code>ResolveFormID</code> to attempt to find the new form ID, based on the user's current load
    // order and the load order that was recorded in the save file.
    RE::FormID newFormId = 0;
    if (!serde->ResolveFormID(formId, newFormId)) 
    {
        logger::warn("Spell ID {:X} could not be found after loading the save.", formId);
    }
    return newFormId;
}

inline std::string& ReadString(SKSE::SerializationInterface* serde, std::string& outStr) {
    int sz;
    serde->ReadRecordData(sz);
    outStr.resize_and_overwrite(sz, [&](char* buf, std::size_t bufSize) {
        serde->ReadRecordData(buf, sz);
        return sz;
    });
    return outStr;
}

void Loki::DynamicAnimationCasting::OnGameLoaded(SKSE::SerializationInterface* serde) {
    std::uint32_t type;
    std::uint32_t size;
    std::uint32_t version;

    ResetCustomSpells();
    // To load records from a cosave, use <code>GetNextRecordInfo</code> to iterate from one record to the next.
    // You will be given records in the order they were written, but otherwise you do not look up a record by its name.
    // Instead check the result of each iteration to find out which record it is and handle it appropriately.
    //
    // If you make breaking changes to your data format, you can increase the version number used when writing the data
    // out and check that number here to handle previous versions.
    std::string name;
    while (serde->GetNextRecordInfo(type, version, size)) {
        if (type == CustomSpellRecord) {
            serde->ReadRecordData(MagicFavouriteIndex);
            // First read how many items follow in this record, so we know how many times to iterate.
            int customSpellSize;
            serde->ReadRecordData(customSpellSize);
            // Iterate over the remaining data in the record.
            for (; customSpellSize > 0; --customSpellSize) {
                ReadString(serde, name);

                RE::FormID spellFormID = ReadFormID(serde);
                auto* spell = RE::TESForm::LookupByID<RE::MagicItem>(spellFormID);

                RegisterCustomSpell(name, spell);
            }
        } else {
            logger::warn("Unknown record type in cosave.");
        }
    }
}
