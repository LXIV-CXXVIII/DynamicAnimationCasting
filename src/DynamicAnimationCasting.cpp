#include "DynamicAnimationCasting.h"
#include "Framework.h"

void Loki::HUD::FlashHUDMeter(RE::ActorValue a_av) {
    static REL::Relocation<decltype(FlashHUDMeter)> FlashHUDMenuMeter{RELOCATION_ID(51907, 52845)};
    return FlashHUDMenuMeter(a_av);
}

RE::EnchantmentItem* GetEquipedObjectEnchantment(RE::Actor* actor, int hand) 
{
    RE::EnchantmentItem* enchantment = nullptr;
    if (auto equipment = actor->GetEquippedObject(hand)) {
        if (auto enchantable = equipment->As<RE::TESEnchantableForm>()) {
            enchantment = enchantable->formEnchanting;
        }
        if (!enchantment) {
            if (auto entry = actor->GetEquippedEntryData(hand)) {
                if (entry->extraLists) {
                    for (const auto& xList : *entry->extraLists) {
                        if (const auto xEnch = xList->GetByType<RE::ExtraEnchantment>()) {
                            enchantment = xEnch->enchantment;
                        }
                    }
                }
            }
        }
    }
    return enchantment;
};

RE::FormID GetEnchantmentEffectId(RE::EnchantmentItem* enchantment)
{
    if (enchantment) {
        if (enchantment->data.baseEnchantment) {
            enchantment = enchantment->data.baseEnchantment;
        }
        if (auto effect = enchantment->GetCostliestEffectItem()) {
            if (auto base_effect = effect->baseEffect) {
                return base_effect->formID;
            }
        }
    }
    return 0;
};

bool DoesActorHasEffect(RE::Actor* actor, RE::FormID a_id)
{
    if (auto activeEffect = actor->AsMagicTarget()->GetActiveEffectList()) {
        bool hasIt = false;
        for (auto& ae : *activeEffect) {
            if (!ae) {
                break;
            }
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
    return false;
};

void Loki::AnimationCasting::CastTrigger::Invoke(const RE::Actor* a_caster) 
{
    RE::Actor * const actor = const_cast<RE::Actor*>(a_caster);

    // faster rejection for player only event
    if (!CheckFormID(caster, [&]() { return actor->formID; }) ||
        !CheckFormID(race, [&] { return actor->GetRace()->formID; })) {
        return;
    }

    // Character status queries
    if (isOnMount.has_value() && isOnMount.value() != actor->IsOnMount()) {
        return;
    }

    if (isSneaking.has_value() && isSneaking.value() != actor->IsSneaking()) {
        return;
    }

    if (isRunning.has_value() && isRunning.value() != actor->IsRunning()) {
        return;
    }

    RE::ActorValueOwner* actorAV = actor->AsActorValueOwner();
    const RE::Actor::ACTOR_RUNTIME_DATA& actorRD = actor->GetActorRuntimeData();

    if (staminaCost > 0.f && actorAV->GetActorValue(RE::ActorValue::kStamina) < staminaCost) {
        HUD::FlashHUDMeter(RE::ActorValue::kStamina);
        return;
    }

    if (healthCost > 0.f && actorAV->GetActorValue(RE::ActorValue::kHealth) < healthCost) {
        HUD::FlashHUDMeter(RE::ActorValue::kHealth);
        return;
    }

    float actorMagicka = actorAV->GetActorValue(RE::ActorValue::kMagicka);
    if (magickaCost > 0.f && actorMagicka < magickaCost) {
        HUD::FlashHUDMeter(RE::ActorValue::kMagicka);
        return;
    }

    if (perk && !actor->HasPerk(perk) || keyword && !actor->HasKeyword(keyword)) {
        return;
    }

    // Check the active effect is slow
    if (!IsInvalidFormID(effect) && DoesActorHasEffect(actor, effect)) {
        return;
    }

    auto CastSpells = [&](RE::MagicSystem::CastingSource source, float magnitude, bool dual_casting) {
        // logger::info("Passed all conditional checks, subtracting costs and casting spells now...");

        actorAV->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, healthCost * -1.00f);
        actorAV->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kStamina, staminaCost * -1.00f);
        float totalMagikaCost = this->magickaCost;

        auto magicCaster = actor->GetMagicCaster(source);
        if (!magicCaster) return;
        
        // Casting from weapon tip
        magicCaster->SetDualCasting(dual_casting);
        if ((int)source < 2) {
            if (auto actorMagicCaster = dynamic_cast<RE::ActorMagicCaster*>(magicCaster)) {
                if (auto root = actor->Get3D()) {
                    static constexpr std::string_view NodeNames[2] = {"WEAPON"sv, "SHIELD"sv};
                    if (auto weaponBone = root->GetObjectByName(NodeNames[(int)source == 0])) {
                        if (auto weapNode = weaponBone->AsNode()) {
                            // Use Precision's weapon tip node if aviable
                            if (weapNode->GetChildren().size() > 0) {
                                weapNode = weapNode->GetChildren().front()->AsNode();
                            }
                            actorMagicCaster->magicNode = weapNode;
                        }
                    }
                }
            }
        }

        auto CastOneSpell = [&](RE::MagicItem* spell) {
            if (!spell) return true;

            if (ignoreConcentrationSpell && spell->GetCastingType() == RE::MagicSystem::CastingType::kConcentration) return true;

            float spellCost = spell->CalculateMagickaCost(actor) * this->castMagickaCostFactor;
            if (actorMagicka < totalMagikaCost + spellCost) {
                HUD::FlashHUDMeter(RE::ActorValue::kMagicka);
                return false;
            }
            totalMagikaCost += spellCost;

            // logger::info("Casting Spell ' {} ' now", spell->GetFullName());
            bool targetSelf = spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
            RE::Actor* target = targetSelf ? actor : actor->GetActorRuntimeData().currentCombatTarget.get().get();
            float magnitudeOverride = magnitude;
            if (auto* effect = spell->GetCostliestEffectItem()) {
                magnitudeOverride = effect->GetMagnitude() * magnitude;

                if (ignoreBoundWeapon &&
                    effect->baseEffect &&
                    effect->baseEffect->HasArchetype(RE::EffectArchetypes::ArchetypeID::kBoundWeapon))
                {
                    return true;
                }
            }

            magicCaster->CastSpellImmediate(spell,                        // spell
                                            false,                        // noHitEffectArt
                                            target,                       // target
                                            effectiveness,                // effectiveness
                                            false,                        // hostileEffectivenessOnly
                                            magnitudeOverride,            // magnitude override
                                            targetSelf ? nullptr : actor  // cause
            );
            return true;
        };

        auto CastOneSpellOrShout = [&](RE::TESForm* power, int shoutLevel = RE::TESShout::VariationIDs::kThree) {
            if (power) {
                auto spell = power->As<RE::MagicItem>();
                if (!spell) {
                    auto shout = power->As<RE::TESShout>();
                    // how to check the words unlocked ???
                    shoutLevel = std::clamp(shoutLevel, 0, 2);
                    spell = shout->variations[shoutLevel].spell;
                }
                return CastOneSpell(spell);
            }
            return true;
        };

        auto GetPlayerFavouriteSpell = []() -> RE::MagicItem* {
            const auto& favSpells = RE::MagicFavorites::GetSingleton()->spells;
            for (int i = 0; i < favSpells.size(); i++) {
                int favIndex = (DynamicAnimationCasting::MagicFavouriteIndex + i) % favSpells.size();
                if (auto magic = favSpells[favIndex]->As<RE::MagicItem>()) {
                    // DynamicAnimationCasting::MagicFavouriteIndex = favIndex + 1;
                    return magic;
                }
            }
            return nullptr;
        };

        static constexpr RE::FormID PlayerFormID = 20;
        if (castFaviouriteMagic && actor->formID == PlayerFormID) {
            CastOneSpellOrShout(GetPlayerFavouriteSpell(), RE::TESShout::VariationIDs::kThree);
        }

        if ((int)source < 2) {
            if (castForeHandSpell) {
                if (auto spell = actorRD.selectedSpells[(int)source]) {
                    CastOneSpell(spell->As<RE::MagicItem>());
                }
            }
            if (castOffHandSpell) {
                if (auto spell = actorRD.selectedSpells[!(int)source]) {
                    CastOneSpell(spell->As<RE::MagicItem>());
                }
            }
        }

        if (castEquipedPower) {
            CastOneSpellOrShout(actorRD.selectedPower, actor->GetCurrentShoutLevel());
        }

        for (auto spell : spells) {
            if (!CastOneSpellOrShout(spell)) break;
        }

        for (const auto& customSpellName : customSpells) {
            if (auto itr = DynamicAnimationCasting::CustomSpells.find(customSpellName); itr != DynamicAnimationCasting::CustomSpells.end()) {
                if (auto spell = itr->second ) {
                    if (!CastOneSpellOrShout(spell)) break;
                }
            }            
        }

        actorAV->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, totalMagikaCost * -1.00f);
    };

    if (HasWeaponConstraints()) {
        for (int hand = 0; hand < 2; hand++) {
            auto castingSource = static_cast<RE::MagicSystem::CastingSource>(!hand);
            if (auto equipment = actor->GetEquippedObject(hand)) {
                if (!CheckFormID(weapons[hand].formid, [&] { return equipment->formID; }))
                    continue;

                auto GetEquipmentType = [](RE::TESForm* equipment) -> std::int8_t {
                    if (auto weapon = equipment->As<RE::TESObjectWEAP>()) {
                        return (int8_t)weapon->weaponData.animationType.underlying();
                    } else if (equipment->As<RE::SpellItem>()) {
                        return kSpell;  // Spell or Scroll as 10th weapon type
                    } else if (equipment->As<RE::TESObjectARMO>()) {
                        return kShield;
                    } else if (equipment->As<RE::TESObjectARMO>()) {
                        return kTorch;
                    }
                    return -1;
                };

                auto EquipmentHasKeyword = [](RE::TESForm* equipment, const RE::BGSKeyword* keyword) {
                    if (auto kwform = equipment->As<RE::BGSKeywordForm>()) {
                        return kwform->HasKeyword(keyword);
                    }
                    return false;
                };

                if (weapons[hand].type >= 0 && weapons[hand].type != GetEquipmentType(equipment))
                    continue;

                RE::EnchantmentItem* enchantment = nullptr;  // weapon enchantment
                if (!IsInvalidFormID(weapons[hand].enchant)) {
                    enchantment = GetEquipedObjectEnchantment(actor, hand);
                }
                if (!CheckFormID(weapons[hand].enchant, [&] { return GetEnchantmentEffectId(enchantment); })) continue;

                if (weapons[hand].keyword && !EquipmentHasKeyword(equipment, weapons[hand].keyword)) continue;

                if (weapons[hand].cast) {
                    float magnitudeModifier = baseMagnitude;
                    if (enchantment) {
                        if (auto effect = enchantment->GetCostliestEffectItem()) {
                            magnitudeModifier += effect->GetMagnitude() * weapons[hand].enchantMagnitudeFactor;
                        }
                    }

                    if (cooldown > 0 && !DynamicAnimationCasting::UpdateTriggerCooldown(cooldown, this, actor)) {
                        return; // not continue
                    }
                    CastSpells(castingSource, magnitudeModifier, dualCasting);
                }
            }
        }
    } else {
        if (cooldown > 0 && !DynamicAnimationCasting::UpdateTriggerCooldown(cooldown, this, actor)) {
            return;
        }
        CastSpells(RE::MagicSystem::CastingSource::kInstant, baseMagnitude, dualCasting);
    }
}

std::string Loki::AnimationCasting::CastTrigger::ToString() const {
    return {};
}
