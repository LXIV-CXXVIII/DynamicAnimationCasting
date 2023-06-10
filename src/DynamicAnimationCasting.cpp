#include "PCH.h"
#include "DynamicAnimationCasting.h"
#include "Framework.h"
#include <random>

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

bool DoesEnchantmentHasEffect(RE::EnchantmentItem* enchantment, RE::FormID effectID) {
    if (!enchantment) return false;
    if (enchantment->data.baseEnchantment) {
        enchantment = enchantment->data.baseEnchantment;
    }
    auto IsConstrainedEffect = [&](const RE::Effect* effect) -> bool {
        return effect && effect->baseEffect && effect->baseEffect->formID == effectID;
    };
    return std::ranges::any_of(enchantment->effects, IsConstrainedEffect);
}

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

std::int8_t GetEquipmentType(RE::TESForm* equipment) 
{
    using namespace Loki::AnimationCasting;
    if (!equipment) {
        return std::to_underlying(RE::WEAPON_TYPE::kHandToHandMelee);
    } else if (auto weapon = equipment->As<RE::TESObjectWEAP>()) {
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

bool HasKeyword(RE::TESForm* form, const RE::BGSKeyword* keyword) 
{
    if (form && keyword) {
        if (auto kwform = form->As<RE::BGSKeywordForm>()) {
            return kwform->HasKeyword(keyword);
        }
    }
    return false;
};

bool Loki::AnimationCasting::CastTrigger::Invoke(const RE::Actor* a_caster) 
{
    RE::Actor * const actor = const_cast<RE::Actor*>(a_caster);
    if (!actor) {
        return false;
    }

    // faster rejection for player only event
    if (!CheckFormID(caster, [&]() { return actor->formID; }) ||
        !CheckFormID(race, [&] { return actor->GetRace()->formID; })) {
        return false;
    }

    // Character status queries
    if (isOnMount.has_value() && isOnMount.value() != actor->IsOnMount()) {
        return false;
    }

    if (isSneaking.has_value() && isSneaking.value() != actor->IsSneaking()) {
        return false;
    }

    if (isRunning.has_value() && isRunning.value() != actor->IsRunning()) {
        return false;
    }

    RE::ActorValueOwner* actorAV = actor->AsActorValueOwner();
    if (!actorAV) {
        return false;
    }
    const RE::Actor::ACTOR_RUNTIME_DATA& actorRD = actor->GetActorRuntimeData();

    if (staminaCost > 0.f && actorAV->GetActorValue(RE::ActorValue::kStamina) < staminaCost) {
        HUD::FlashHUDMeter(RE::ActorValue::kStamina);
        return false;
    }

    if (healthCost > 0.f && actorAV->GetActorValue(RE::ActorValue::kHealth) < healthCost) {
        HUD::FlashHUDMeter(RE::ActorValue::kHealth);
        return false;
    }

    float actorMagicka = actorAV->GetActorValue(RE::ActorValue::kMagicka);
    if (magickaCost > 0.f && actorMagicka < magickaCost) {
        HUD::FlashHUDMeter(RE::ActorValue::kMagicka);
        return false;
    }

    if (chance < 1.f) {
        static std::mt19937 rng (std::random_device{}());  // Standard mersenne_twister_engine seeded with rd()
        std::uniform_real_distribution<float> distrib(0, 1.f);
        if (distrib(rng) >= chance) {
            return false;
        }
    }

    if (perk && !actor->HasPerk(perk) || keyword && !actor->HasKeyword(keyword)) {
        return false;
    }

    // Check the active effect is slow
    if (!IsInvalidFormID(effect) && !DoesActorHasEffect(actor, effect)) {
        return false;
    }

    auto CastSpells = [&](RE::MagicSystem::CastingSource source, float magnitude, bool dual_casting) -> bool {
        // logger::info("Passed all conditional checks, subtracting costs and casting spells now...");
        if (healthCost != 0.f && actorAV) {
            actorAV->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kHealth, healthCost * -1.00f);
        }
        if (staminaCost != 0.f && actorAV) {
            actorAV->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kStamina, staminaCost * -1.00f);
        }
        float totalMagikaCost = this->magickaCost;

        auto magicCaster = actor->GetMagicCaster(source);
        if (!magicCaster) return false;
        
        // Casting from weapon tip
        magicCaster->SetDualCasting(dual_casting);
        if (std::to_underlying(source) < 2) {
            if (auto actorMagicCaster = dynamic_cast<RE::ActorMagicCaster*>(magicCaster)) {
                if (auto root = actor->Get3D()) {
                    static constexpr std::string_view NodeNames[2] = {"WEAPON"sv, "SHIELD"sv};
                    if (auto weaponBone = root->GetObjectByName(NodeNames[source == RE::MagicSystem::CastingSource::kLeftHand])) {
                        if (auto weapNode = weaponBone->AsNode()) {
                            // Use Precision's weapon tip node if aviable
                            if (weapNode->GetChildren().size() > 0) {
                                if (auto weapTip = weapNode->GetChildren().front()) {
                                    if (auto weapTipNode = weapTip->AsNode()) {
                                        weapNode = weapTipNode;
                                    }
                                }
                            }
                            actorMagicCaster->magicNode = weapNode;
                        }
                    }
                }
            }
        }

        int casted = 0;

        auto CastOneSpell = [&](RE::MagicItem* spell) {
            if (!spell) return false;

            // Ignore enchantment on weapons
            if (spell->formType == RE::FormType::Enchantment) return false;

            if (!IsAllowedSpell(actor, spell)) return false;

            float spellCost = spell->CalculateMagickaCost(actor) * this->castMagickaCostFactor;
            if (actorMagicka < totalMagikaCost + spellCost) {
                HUD::FlashHUDMeter(RE::ActorValue::kMagicka);
                return false;
            }
            totalMagikaCost += spellCost;

            // logger::info("Casting Spell ' {} ' now", spell->GetFullName());
            bool targetSelf = targetCaster || spell->GetDelivery() == RE::MagicSystem::Delivery::kSelf;
            RE::Actor* target = targetSelf ? actor : actorRD.currentCombatTarget.get().get();
            float magnitudeOverride = magnitude;
            if (auto* effect = spell->GetCostliestEffectItem()) {
                magnitudeOverride = effect->GetMagnitude() * magnitude;
            }

            magicCaster->CastSpellImmediate(spell,                        // spell
                                            false,                        // noHitEffectArt
                                            target,                       // target
                                            effectiveness,                // effectiveness
                                            false,                        // hostileEffectivenessOnly
                                            magnitudeOverride,            // magnitude override
                                            targetSelf ? nullptr : actor  // cause
            );
            casted++;
            return true;
        };

        auto CastOneSpellOrShout = [&](RE::TESForm* power, int shoutLevel = RE::TESShout::VariationIDs::kThree) {
            if (power) {
                auto spell = power->As<RE::MagicItem>();
                if (!spell) {
                    if (auto shout = power->As<RE::TESShout>()) {
                        // how to check the words unlocked ???
                        shoutLevel = std::clamp(shoutLevel, 0, 2);
                        spell = shout->variations[shoutLevel].spell;
                    }
                }
                return CastOneSpell(spell);
            }
            return false;
        };

        auto GetPlayerFavouriteSpell = []() -> RE::MagicItem* {
            const auto& favSpells = RE::MagicFavorites::GetSingleton()->spells;
            int favIndex = DynamicAnimationCasting::MagicFavouriteIndex;
            if (favIndex >= 0) {
                return favSpells[favIndex % favSpells.size()]->As<RE::MagicItem>();
            }
            return nullptr;
        };

        int sourceIndex = std::to_underlying(source);
        if (sourceIndex >= 0 && sourceIndex <= 2) {
            if (castForeHandSpell) {
                if (auto spell = actorRD.selectedSpells[sourceIndex]) {
                    CastOneSpell(spell->As<RE::MagicItem>());
                    if (castOnlyOneSpell && casted) goto CAST_SPELLS_END;
                }
            }
            if (castOffHandSpell) {
                int offHandSourceIndex = !sourceIndex;
                if (auto spell = actorRD.selectedSpells[offHandSourceIndex]) {
                    CastOneSpell(spell->As<RE::MagicItem>());
                    if (castOnlyOneSpell && casted) goto CAST_SPELLS_END;
                }
            }
        }

        if (castEquipedPower) {
            CastOneSpellOrShout(actorRD.selectedPower, actor->GetCurrentShoutLevel());
            if (castOnlyOneSpell && casted) goto CAST_SPELLS_END;
        }

        static constexpr RE::FormID PlayerFormID = 0x14;
        if (castFaviouriteMagic) {
            RE::MagicItem* spell = nullptr;
            if (actor->formID == PlayerFormID) {
                // Use the player's selected favourite spell
                spell = GetPlayerFavouriteSpell();
            } else {
                // Use one of the NPC's added spell
                for (auto& spl : actorRD.addedSpells) {
                    if (IsAllowedSpell(actor, spl)) {
                        spell = spl;
                        break;
                    }
                }
            }
            CastOneSpellOrShout(spell, RE::TESShout::VariationIDs::kThree);
            if (castOnlyOneSpell && casted) goto CAST_SPELLS_END;
        }

        for (auto spell : spells) {
            CastOneSpellOrShout(spell);
            if (castOnlyOneSpell && casted) goto CAST_SPELLS_END;
        }

        for (const auto& customSpellName : customSpells) {
            if (auto itr = DynamicAnimationCasting::CustomSpells.find(customSpellName); itr != DynamicAnimationCasting::CustomSpells.end()) {
                if (auto spell = itr->second ) {
                    CastOneSpellOrShout(spell);
                    if (castOnlyOneSpell && casted) goto CAST_SPELLS_END;
                }
            }            
        }

        // Deduce the final magicka cost
        CAST_SPELLS_END:
        if (totalMagikaCost != 0.f && actorAV) {
            actorAV->RestoreActorValue(RE::ACTOR_VALUE_MODIFIER::kDamage, RE::ActorValue::kMagicka, totalMagikaCost * -1.00f);
        }

        return casted;
    };

    if (HasWeaponConstraints()) {
        bool casted = false;
        for (int hand = 0; hand < 2; hand++) {
            auto castingSource = static_cast<RE::MagicSystem::CastingSource>(!hand);
            auto equipment = actor->GetEquippedObject(hand);

            if (!CheckFormID(weapons[hand].formid, [&] { return equipment ? equipment->formID : 0; }))
                continue;

            if (weapons[hand].type >= 0 && weapons[hand].type != GetEquipmentType(equipment))
                continue;

            RE::EnchantmentItem* enchantment = nullptr;  // weapon enchantment
            if (!IsInvalidFormID(weapons[hand].enchant)) {
                enchantment = GetEquipedObjectEnchantment(actor, hand);
                if (!enchantment || !DoesEnchantmentHasEffect(enchantment, weapons[hand].enchant)) continue;
            }

            if (weapons[hand].keyword && !HasKeyword(equipment, weapons[hand].keyword)) continue;

            if (weapons[hand].cast) {
                float magnitudeModifier = baseMagnitude;
                if (enchantment && weapons[hand].enchantMagnitudeFactor != 0.f) {
                    if (auto effect = enchantment->GetCostliestEffectItem()) {
                        magnitudeModifier += effect->GetMagnitude() * weapons[hand].enchantMagnitudeFactor;
                    }
                }

                if (cooldown > 0 && !DynamicAnimationCasting::UpdateTriggerCooldown(cooldown, this, actor)) {
                    return false; // not continue
                }
                casted = casted || CastSpells(castingSource, magnitudeModifier, dualCasting);
            }
        }
        return casted;
    } else {
        if (cooldown > 0 && !DynamicAnimationCasting::UpdateTriggerCooldown(cooldown, this, actor)) {
            return false;
        }
        return CastSpells(RE::MagicSystem::CastingSource::kInstant, baseMagnitude, dualCasting);
    }
}

std::string Loki::AnimationCasting::CastTrigger::ToString() const {
    return {};
}
