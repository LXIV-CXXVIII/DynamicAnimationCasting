#include "framework.h"
#define TOML_EXCEPTIONS 0
#include <toml++/toml.h>

void Loki::DynamicAnimationCasting::ReadToml(std::filesystem::path path) {
    logger::info("Reading {}...", path.string());

    using namespace std::string_literals;

    static const std::pair<std::string, int> noformid;

    auto handle = RE::TESDataHandler::GetSingleton();
    auto parse_formid = [handle](const toml::table& parent, std::string_view key) -> RE::FormID
    { 
        if (auto table = parent.get_as<toml::table>(key)) {
            auto esp = table->get_as<std::string>("Esp");
            auto id = table->get_as<int64_t>("Id");
            if (esp && id) {
                return handle->LookupFormID(**id, **esp);
            }
        }
        return -1;
    };

    auto parse_formid2 = [handle](const toml::table& parent, std::string_view id_key, std::string_view esp_key) -> RE::FormID {
        auto esp = parent.get_as<std::string>(esp_key);
        auto id = parent.get_as<int64_t>(id_key);
        if (esp && id) {
            return handle->LookupFormID(**id, **esp);
        }
        return -1;
    };

    auto parse_weapon_type = [](const toml::node_view<toml::node>& node) -> std::int8_t {
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

    auto parse_optional_bool = [](const toml::node_view<toml::node>& node) -> std::optional<bool> {
        if (auto boolnode = node.as_boolean()) {
            return boolnode->get();
        } else {
            return {};
        }
    };

    const std::string empty_string;

    toml::parse_result file;
    try {
        file = toml::parse_file(path.c_str());
    } catch (const toml::parse_error& e) {
        logger::error("Error parsing file '{}':\t{}\n\t\t({}:{})", *e.source().path, e.description(),
                      e.source().begin.line, e.source().begin.column);
        return;
    } 
    
    if (auto arr = file.table().get_as<toml::array>("event")) {
        for (auto&& elem : *arr) {
            auto& event = *elem.as_table();
            using std::string;

            logger::info("- - - - - - - - - - - - - - - - - - - - -");

            auto& trigger = _CastTriggers.emplace_back();
            trigger.tag = event["AnimationEvent"].value_or(empty_string);
            trigger.race = parse_formid2(event, "HasRaceFormID", "RaceEspName");
            trigger.effect = parse_formid2(event, "HasEffectFormID", "EffectEspName");
            trigger.caster = parse_formid2(event, "HasActorFormID", "ActorEspName");
            trigger.keyword = handle->LookupForm<RE::BGSKeyword>(event["HasKeywordFormID"].value_or(-1), event["KeywordEspName"].value_or(empty_string));
            trigger.isOnMount = parse_optional_bool(event["IsOnMount"]);
            trigger.isSneaking = parse_optional_bool(event["IsSneaking"]);
            trigger.isRunning = parse_optional_bool(event["IsRunning"]);
            trigger.weapons[0].formid = parse_formid2(event, "IsEquippedRightFormID", "WeaponEspName");
            trigger.weapons[1].formid = parse_formid2(event, "IsEquippedLeftFormID", "WeaponEspName");
            trigger.weapons[0].enchant = parse_formid2(event, "HasWeaponEnchantEffect", "WeaponEnchantEffectEspName");
            trigger.weapons[1].enchant = trigger.weapons[0].enchant;
            trigger.weapons[0].type = parse_weapon_type(event["HasWeaponType"]);
            trigger.weapons[1].type = trigger.weapons[0].type;
            trigger.weapons[1].enchant = trigger.weapons[0].enchant;
            trigger.weapons[0].keyword = handle->LookupForm<RE::BGSKeyword>(event["HasWeaponKeyword"].value_or(-1), event["WeaponKeywordEspName"].value_or(empty_string));
            trigger.weapons[1].keyword = trigger.weapons[0].keyword;
            trigger.weapons[0].cast = event["CastFromWeapon"].value_or(trigger.weapons[0].Constrainted());
            trigger.weapons[1].cast = event["CastFromWeapon"].value_or(trigger.weapons[1].Constrainted());
            trigger.weapons[0].enchantMagnitudeFactor = event["WeaponEnchantMagnitudeFactor"].value_or(1.f);
            trigger.weapons[1].enchantMagnitudeFactor = trigger.weapons[0].enchantMagnitudeFactor;
            trigger.targetCaster = event["TargetCaster"].value_or(false);
            trigger.dualCasting = event["DualCasting"].value_or(false);
            trigger.castOffHandSpell = event["CastOffHandSpell"].value_or(false);
            trigger.castForeHandSpell = event["CastForeHandSpell"].value_or(false);
            trigger.castEquipedPower = event["CastEquipedPower"].value_or(false);
            trigger.castFaviouriteMagic = event["CastFaviouriteMagic"].value_or(false);
            trigger.healthCost = event["HealthCost"].value_or(0.f);
            trigger.magickaCost = event["MagickaCost"].value_or(0.f);
            trigger.staminaCost = event["StaminaCost"].value_or(0.f);
            trigger.castMagickaCostFactor = event["CastMagickaCostFactor"].value_or(1.f);
            trigger.baseMagnitude = event["Magnitude"].value_or(1.f);

            auto spellEsp = event["SpellEspName"].value_or(empty_string);
            if (auto a_spells = event["SpellFormIDs"].as_array()) {
                trigger.spells_buffer.resize(a_spells->size() * sizeof(RE::SpellItem*));
                auto spells = trigger.Spells();
                int n_spells = 0;
                for (auto& snode : *a_spells) {
                    bool added = false;
                    auto spellFormID = snode.value_or(-1);
                    if (auto spellform = handle->LookupForm(spellFormID, spellEsp)) {
                        if (auto magicitem = spellform->As<RE::MagicItem>()) {
                            spells[n_spells++] = magicitem;
                            added = true;
                        }
                    }
                    if (!added) {
                        logger::warn("{}|{} is not a valid magic item form", spellEsp, spellFormID);
                    }
                }
                trigger.spells_buffer.resize(n_spells * sizeof(RE::MagicItem*));
            }

            if (trigger.Spells().empty() && !trigger.castOffHandSpell && !trigger.castEquipedPower && !trigger.castForeHandSpell && !trigger.castFaviouriteMagic)
                _CastTriggers.pop_back();
        }
        logger::info("Successfully read {}...", path.string());
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

    std::sort(_CastTriggers.begin(), _CastTriggers.end(),
              [](const auto& lhs, const auto& rhs) { return (std::string_view)lhs.tag < (std::string_view)rhs.tag; });

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

RE::BSEventNotifyControl Loki::DynamicAnimationCasting::ProcessEvent(RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_this, RE::BSAnimationGraphEvent& a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_src) {

    if (a_event.tag != NULL && a_event.holder != NULL) {
        auto actorPtr = a_event.holder->As<RE::Actor>();

        for (auto& trigger : DynamicAnimationCasting::_CastTriggers) {
            // BSFixedString::operator== is just a pointer compare, super efficient
            if (a_event.tag == trigger.tag) {
                // logger::info("Event Found: {}", a_event.tag);
                trigger.CastSpells(actorPtr);
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
