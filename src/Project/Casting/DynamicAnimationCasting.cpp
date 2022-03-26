#include "DynamicAnimationCasting.h"

void Loki::HUD::FlashHUDMeter(RE::ActorValue a_av) {
    using func_t = decltype(&Loki::HUD::FlashHUDMeter);
    REL::Relocation<func_t> func{ REL::ID(51907) };
    return func(a_av);
}

void Loki::AnimationCasting::Cast::CastSpells(const RE::Actor* a_actor) {

	RE::Actor* actor = (RE::Actor*)a_actor;

    bool failed = false;
    if (actor->GetActorValue(RE::ActorValue::kStamina) < _properties.staminaCost) {
        logger::info("Failed kStamina check"); failed = true;
        HUD::FlashHUDMeter(RE::ActorValue::kStamina);
    }
    if (actor->GetActorValue(RE::ActorValue::kMagicka) < _properties.magickaCost) {
        logger::info("Failed kMagicka check"); failed = true;
        HUD::FlashHUDMeter(RE::ActorValue::kMagicka);
    }
    if (failed) { return; }

    //static RE::Effect* ActorHasEffectWithKeyword(RE::Actor* a_actor, RE::FormID a_formID);
    auto HasEffect = [actor](RE::FormID a_id) -> bool {
        auto activeEffect = actor->GetActiveEffectList();
        if (activeEffect) {
            bool hasIt = false;
            if (auto dhandle = RE::TESDataHandler::GetSingleton(); dhandle) {
                for (auto& ae : *activeEffect) {
                    if (!ae->effect) {
                        continue;
                    }
                    if (!ae->effect->baseEffect) {
                        continue;
                    }
                    if (ae->effect->baseEffect->formID == a_id) {
                        hasIt = true;
                    }
                }
                return hasIt;
            }
        }
    };

    logger::info("Passed requirement check");
    RE::FormID id = 0;
    RE::WEAPON_TYPE type = {};
    auto weapon = a_actor->GetEquippedObject(false);
    if (weapon) { 
        id = weapon->formID;
        type = actor->GetEquippedObject(false)->As<RE::TESObjectWEAP>()->weaponData.animationType.get();
    };
    if (auto handle = RE::TESDataHandler::GetSingleton(); handle) {

        auto vrace = handle->LookupForm<RE::TESRace>(_properties.racePair.first, _properties.racePair.second);
        if (_properties.racePair.first == -1 || _properties.racePair.first == 0 || (vrace && vrace->formID == actor->race->formID)) {
        
            auto vactor = handle->LookupForm<RE::Actor>(_properties.actorPair.first, _properties.actorPair.second);
            if (_properties.actorPair.first == -1 || _properties.actorPair.first == 0 || (vactor && vactor->formID == actor->formID)) {
            
                auto vweapon = handle->LookupForm<RE::TESObjectWEAP>(_properties.weapPair.first, _properties.weapPair.second);
                if (_properties.weapPair.first == -1 || _properties.weapPair.first == 0 || (vweapon && vweapon->formID == id)) {
                
                    if (_properties.weapType == -1 || _properties.weapType == 0 || (RE::WEAPON_TYPE)_properties.weapType == type) {
                    
                        auto veffect = handle->LookupForm<RE::EffectSetting>(_properties.effectPair.first, _properties.effectPair.second);
                        if (_properties.effectPair.first == -1 || _properties.effectPair.first == 0 || (veffect && HasEffect(veffect->formID))) {
                        
                            auto vkeyword = handle->LookupForm<RE::BGSKeyword>(_properties.keywordPair.first, _properties.keywordPair.second);
                            if (_properties.keywordPair.first == -1 || _properties.keywordPair.first == 0 || (vkeyword && actor->HasKeyword(vkeyword))) {
                            
                                auto type = actor->GetEquippedObject(false)->As<RE::TESObjectWEAP>()->weaponData.animationType.get();

                                logger::info("Passed all conditional checks, subtracting costs and casting spells now...");

                                actor->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, _properties.healthCost * -1.00f);
                                actor->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, _properties.magickaCost * -1.00f);
                                actor->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kStamina, _properties.staminaCost * -1.00f);

                                for (auto spell : _properties.spells) {
                                    for (auto it = spell.second.begin(); it < spell.second.end(); ++it) {
                                        auto single = handle->LookupForm<RE::SpellItem>((RE::FormID)*it, spell.first.c_str());
                                        logger::info("Casting Spell ' {} ' now", single->GetFullName());
                                        actor->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant)->Cast(
                                            single,                                     // spell
                                            false,                                      // noHitEffectArt
                                            _properties.targetCaster ? actor : nullptr, // target
                                            1.00f,                                      // effectiveness
                                            false,                                      // hostileEffectivenessOnly
                                            0.0f,                                       // magnitude override
                                            _properties.targetCaster ? nullptr : actor  // cause
                                        );
                                    }
                                }

                                logger::info("... Finished casting spells.");
                            
                            } else { logger::info("keyword check failed"); }

                        } else { logger::info("effect check failed"); }
                    
                    } else { logger::info("weapon type check failed"); }
                
                } else { logger::info("weapon check failed"); }
            
            } else { logger::info("actor check failed"); }

        } else { logger::info("race check failed"); }
    }
    else { logger::info("Error: invalid data handler"); };

}