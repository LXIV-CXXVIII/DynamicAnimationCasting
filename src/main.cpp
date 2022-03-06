#include "C:/dev/ExamplePlugin-CommonLibSSE/build/simpleini-master/SimpleIni.h"
#include "TrueHUDAPI.h"

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface * a_skse, SKSE::PluginInfo * a_info)
{
#ifndef NDEBUG
    auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
    auto path = logger::log_directory();
    if (!path) {
        return false;
    }

    *path /= "loki_NoFollowerAttackCollision.log"sv;
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

    auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
    log->set_level(spdlog::level::trace);
#else
    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);
#endif

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

    logger::info("loki_NoFollowerAttackCollision v1.0.0");

    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = "loki_NoFollowerAttackCollision";
    a_info->version = 1;

    if (a_skse->IsEditor()) {
        logger::critical("Loaded in editor, marking as incompatible"sv);
        return false;
    }

    const auto ver = a_skse->RuntimeVersion();
    if (ver < SKSE::RUNTIME_1_5_39) {
        logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
        return false;
    }

    return true;
}

/**
using ItemMap = RE::TESObjectREFR::InventoryItemMap;
class SkyrimOnlineService_Host {

    struct Player {
    public:
        std::string      PlayerName = {};
        std::uint16_t    PlayerLevel = NULL;
        //ItemMap          PlayerInventory = {};
        RE::BGSLocation* PlayerLocation = {};
        bool IsLobbyOpen = false;

    };
    struct ConnectedPlayer {
    public:
        std::string   PlayerInstance = {};
        std::uint64_t SteamID = {};
        std::string   Token = {};
        Player        player;
    };
    struct Pool {
    public:
        std::string ID = {};
        std::list<ConnectedPlayer> Players;
    };
    struct Scope {
    public:
        std::string     App;
        std::list<Pool> Pools;
    };

public:
    virtual HSteamListenSocket CreateListenSocket(const SteamNetworkingIPAddr& localAddress, int nOptions, const SteamNetworkingConfigValue_t* pOptions) {

    }
    virtual SteamAPICall_t  CreateCoopLobby(ISteamMatchmaking* a_mm, ELobbyType a_lobbyType) { return _CreateCoopLobbyImpl(a_mm, a_lobbyType); };
    virtual SteamAPICall_t  CreateGroupLobby(ISteamMatchmaking* a_mm, ELobbyType a_lobbyType) { return _CreateGroupLobbyImpl(a_mm, a_lobbyType); };
    virtual SteamAPICall_t  LeaveLobby(ISteamMatchmaking* a_mm, CSteamID a_lobby) { return _LeaveLobbyImpl(a_mm, a_lobby); };
    virtual Player          GetPlayer() { return ConstructPlayerInformation(); };
    virtual RE::NiAVObject* GetPlayer3D() { return _GetPlayer3DImpl(); };

    // members
    bool isNetworkActive = false;

private:
    SteamAPICall_t _CreateCoopLobbyImpl(ISteamMatchmaking* a_mm, ELobbyType a_lobbyType) {
        isNetworkActive = player.IsLobbyOpen ? true : false;
        return a_mm->CreateLobby(a_lobbyType, 2);
    };
    SteamAPICall_t _CreateGroupLobbyImpl(ISteamMatchmaking* a_mm, ELobbyType a_lobbyType) {
        isNetworkActive = player.IsLobbyOpen ? true : false;
        return a_mm->CreateLobby(a_lobbyType, 8);
    };
    SteamAPICall_t _LeaveLobbyImpl(ISteamMatchmaking* a_mm, CSteamID a_lobby) {
        a_mm->LeaveLobby(a_lobby);
    }
    ConnectedPlayer _ConstructConnectedPlayer() {

    }
    Pool _ConstructPool() {
        pool.ID = "Skyrim DND";
        for (auto idx = pool.Players.begin(); idx != pool.Players.end(); ++idx) {
            pool.Players.push_back(_ConstructConnectedPlayer());
        }
    }
    Pool _GetPoolImpl() {
        return pool;
    }
    Player ConstructPlayerInformation() {
        return []() -> Player {
            player.PlayerName = RE::PlayerCharacter::GetSingleton()->GetName();
            player.PlayerLevel = RE::PlayerCharacter::GetSingleton()->GetLevel();
            //player.PlayerInventory = RE::PlayerCharacter::GetSingleton()->GetInventory();
            player.PlayerLocation = RE::PlayerCharacter::GetSingleton()->currentLocation;
            player.IsLobbyOpen = false;
            return player;
        }();
    }
    RE::NiAVObject* _GetPlayer3DImpl() {
        auto TD = RE::PlayerCharacter::GetSingleton()->GetCurrent3D();
        auto rigidBody = TD->AsBhkRigidBody();
        //rigidBody->GetWorld()->CastRay(); // lol
        return RE::PlayerCharacter::GetSingleton()->GetCurrent3D();
        auto inv = RE::PlayerCharacter::GetSingleton()->GetInventory();
        for (auto idx = inv.begin(); idx != inv.end(); ++idx) {



        }
        //for (auto idx : inv) {

        //}
    }

    // members
    static Player player;
    static Pool pool;

protected:
    // members

};

class SkyrimOnlineService_DM :
    public SkyrimOnlineService_Host {

    enum class CharacterSlot : std::uint64_t {
        kCharacter0 = 0,
        kCharacter1,
        kCharacter2,
        kCharacter3,
        kCharacter4,
        kCharacter5,
        kCharacter6,
        kCharacter7,
        kCharacter8,
    };

    struct ConnectedCharacter {
    public:
        CharacterSlot charSlot;
        RE::Actor* actor;
        RE::TESFaction* faction;
        float hp;
    };

public:
    // OVERRIDE
    virtual SteamAPICall_t CreateGroupLobby(ISteamMatchmaking* a_mm, ELobbyType a_lobbyType) { return _CreateGroupLobbyImpl(a_mm, a_lobbyType); };
    virtual SteamAPICall_t LeaveLobby(ISteamMatchmaking* a_mm, CSteamID a_lobby) { };
    // ADD
    virtual ConnectedCharacter GetCharInfo(CharacterSlot a_slot) { return GetCharacterInformation(a_slot); };

private:
    SteamAPICall_t _CreateGroupLobbyImpl(ISteamMatchmaking* a_mm, ELobbyType a_lobbyType) {
        isNetworkActive = this->GetPlayer().IsLobbyOpen ? true : false;
        return a_mm->CreateLobby(a_lobbyType, 9);
    };
    SteamAPICall_t _LeaveLobbyImpl(ISteamMatchmaking* a_mm, CSteamID a_lobby) {
        a_mm->LeaveLobby(a_lobby);
    }
    ConnectedCharacter _ConstructCharacter(CharacterSlot a_slot) {
        return [a_slot]() -> ConnectedCharacter {
            for (auto idx = characters.begin(); idx != characters.end(); ++idx) {
                idx->charSlot = a_slot;
                idx->actor = skyrim_cast<RE::Actor*>(RE::PlayerCharacter::GetSingleton());
                idx->faction = idx->actor->GetCrimeFaction();
                idx->hp = idx->actor->GetActorValue(RE::ActorValue::kHealth);
                //characters.push_back(&idx.operator==());
            };
        }();
    }
    ConnectedCharacter GetCharacterInformation(CharacterSlot a_slot) {
        return [a_slot]() -> ConnectedCharacter {
            switch (a_slot) {
            case CharacterSlot::kCharacter0:
                for (auto idx : characters) {
                    if (idx.charSlot == CharacterSlot::kCharacter0) {
                        return idx;
                    }
                }

            case CharacterSlot::kCharacter1:
                for (auto idx : characters) {
                    if (idx.charSlot == CharacterSlot::kCharacter1) {
                        return idx;
                    }
                }
            }

        }();
    }
    static std::list<ConnectedCharacter> characters;

};

void func(SkyrimOnlineService_DM* a_dm) {
    //a_dm->CreateGroupLobby();
}

struct Player {

public:
    std::string                         PlayerName = RE::PlayerCharacter::GetSingleton()->GetName();
    std::uint16_t                       PlayerLevel = RE::PlayerCharacter::GetSingleton()->GetLevel();
    RE::TESObjectREFR::InventoryItemMap PlayerInventory = RE::PlayerCharacter::GetSingleton()->GetInventory();
    RE::BGSLocation* PlayerLocation = RE::PlayerCharacter::GetSingleton()->currentLocation;
    //RE::Actor::GetCurrent3D();
    bool IsLobbyOpen = false;

};

struct ConnectedPlayer {

public:
    std::string   PlayerInstance = {};
    std::uint64_t SteamID = {};
    std::string   Token = {};
    Player* player;

};

struct Pool {

public:
    std::string                ID = {};
    //std::list<ConnectedPlayer> Players = ConnectedPlayer;

};

struct Scope {

public:
    std::string     App;
    //std::list<Pool> Pools = Pool;

};
*/


static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message) {

    switch (message->type) {
    case SKSE::MessagingInterface::kNewGame:
    case SKSE::MessagingInterface::kPostLoadGame: {
        break;
    }
    case SKSE::MessagingInterface::kPostLoad: {
        break;
    }
    default:
        break;
    }

}



class Loki_Climbing {

public:
    enum ClimbStartType : int32_t {

        kSmallVault = 0,
        kMediumVault = 1,
        kLargeVault = 2,
        kClimbFromGround = 3,
        kClimbFromAir = 4,

    };

    enum ClimbEndType : int32_t {

        kStepOut = 0,
        kFallOut = 1,
        kJumpForwards = 2,
        kJumpBackwards = 3,

    };

    float rayCastDist;
    float rayCastLowVaultDist, rayCastMediumVaultDist, rayCastLargeVaultDist, rayCastClimbDist;
    float rayCastLowVaultHeight, rayCastMediumVaultHeight, rayCastLargeVaultHeight, rayCastClimbHeight;
    Loki_Climbing() {
        CSimpleIniA ini;
        ini.SetUnicode();
        auto filename = L"Data/SKSE/Plugins/loki_Climbing.ini";
        SI_Error rc = ini.LoadFile(filename);

        this->rayCastDist = ini.GetDoubleValue("SETTINGS", "fRayCastDistance", -1.00f);
        return;
    }
    ~Loki_Climbing() {
    }
    static Loki_Climbing* GetSingleton() {
        static Loki_Climbing* singleton = new Loki_Climbing();
        return singleton;
    }
    static void* CodeAllocation(Xbyak::CodeGenerator& a_code, SKSE::Trampoline* t_ptr)
    {
        auto result = t_ptr->allocate(a_code.getSize());
        std::memcpy(result, a_code.getCode(), a_code.getSize());
        return result;

    }
    static void InstallUpdateHook() {

        REL::Relocation<std::uintptr_t> ActorUpdate{ REL::ID(39375) };

        auto& trampoline = SKSE::GetTrampoline();
        _Update = trampoline.write_call<5>(ActorUpdate.address() + 0x8AC, Update);

        logger::info("Actor Update hook injected");

    }
    static void InstallSimulateClimbingHook() {

        REL::Relocation<std::uintptr_t> ClimbSim{ REL::ID(78195) };

        auto& trampoline = SKSE::GetTrampoline();
        trampoline.write_branch<5>(ClimbSim.address(), &bhkCharacterStateClimbing_SimPhys);

        logger::info("Climbing Simulation hook injected");

    }
    static void InstallClimbSimHook() {

        REL::Relocation<std::uintptr_t> ClimbSim{ REL::ID(78195/*e1d520*/) };
        REL::Relocation<std::uintptr_t> subroutine{ REL::ID(76440/*dc08e0*/) };

        struct Patch : Xbyak::CodeGenerator {
            Patch(std::uintptr_t a_sub) {
                Xbyak::Label l1;
                Xbyak::Label l2;

                cmp(dword[rdx + 0x21C], 0x0B); // wantState
                jz(l1);
                mov(rcx, rdx);
                jmp(ptr[rip + l2]);

                L(l1);
                or_(dword[rdx + 0x218], 0x400); // set kCanJump
                xorps(xmm2, xmm2);
                //xorps  (xmm1, xmm1); //og code
                xorps(xmm1, xmm1); // custom code
                movss(xmm0, dword[rdx + 0xB4]); // velocityMod[1] ogcode Y velocity
                movaps(xmm2, ptr[rdx + 0xB0]); //->
                  // xmm2[0] = (velModX)
                  // xmm2[1] = (velModY)
                  // xmm2[2] = (velModZ)
                  // xmm2[3] = (?)
                unpcklps(xmm2, xmm0); //->
                  // xmm2[0] = xmm2[0] (velModX)
                  // xmm2[1] = xmm0[0] (velModY)
                  // xmm2[2] = xmm2[1] (velModY)
                  // xmm2[3] = xmm0[1] 0
                xorps(xmm0, xmm0);
                unpcklps(xmm2, xmm0); //->
                  // xmm2[0] = xmm2[0] (velModX)
                  // xmm2[1] = xmm0[0] 0
                  // xmm2[2] = xmm2[1] (velModY)
                  // xmm2[3] = xmm0[1] 0
                mulss(xmm2, ptr[rdx + 0xDC]); // mul outVelocity X by rotCenter X
                movaps(ptr[rdx + 0x90], xmm2); // outVelocity
                ret();

                L(l2);
                dq(a_sub);
            }
        };
        // X = X
        // Y = 0
        // Z = Y
        // unpckhps xmm1, xmm2
        // xmm1[0] = xmm2[2]
        // xmm1[1] = 

        Patch patch(subroutine.address());
        patch.ready();

        auto& trampoline = SKSE::GetTrampoline();
        trampoline.write_branch<5>(ClimbSim.address(), Loki_Climbing::CodeAllocation(patch, &trampoline));

    }

private:
    static void ControllerSubroutine(RE::bhkCharacterController* a_controller) {

        using func_t = decltype(ControllerSubroutine);
        REL::Relocation<func_t> func{ REL::ID(76440) }; /*dc08e0*/
        return func(a_controller);

    }
    static void bhkCharacterStateClimbing_SimPhys(RE::bhkCharacterStateClimbing* a_climbing, RE::bhkCharacterController* a_controller) {

        __m128 z128 = { 0.0f,0.0f,0.0f,0.0f };

        __m128 vMod = a_controller->velocityMod.quad;
        __m128 vModY = { a_controller->velocityMod.quad.m128_f32[1], 0.0f, 0.0f, 0.0f };

        if (a_controller->wantState == RE::hkpCharacterStateType::kClimbing) {
            a_controller->flags.set(RE::CHARACTER_FLAGS::kCanJump);
            RE::hkVector4 hkVelocityMod = {};
            hkVelocityMod.quad = _mm_unpacklo_ps(_mm_unpacklo_ps(vMod, vModY), z128);
            hkVelocityMod.quad.m128_f32[0] *= a_controller->rotCenter.quad.m128_f32[0];
            hkVelocityMod.quad.m128_f32[1] *= a_controller->rotCenter.quad.m128_f32[1];
            hkVelocityMod.quad.m128_f32[2] *= a_controller->rotCenter.quad.m128_f32[2];
            a_controller->outVelocity = hkVelocityMod;
        } else {
            ControllerSubroutine(a_controller);
        }
        return;

        /**
        if (a_controller->wantState == ðŸ˜ˆ::kClimbing) {
            a_controller->flags.set(RE::CHARACTER_FLAGS::kCanJump);
            __m128 velocityMod = { a_controller->velocityMod.quad.m128_f32[1], 0.0f, 0.0f, 0.0f };
            a_controller->outVelocity.quad = _mm_unpacklo_ps(_mm_unpacklo_ps(z128, velocityMod), z128);
        } else {
            ControllerSubroutine(a_controller);
        }
        return;
        */

    }
    static void SendClimbingEvent(RE::Actor* a_actor, RE::BSFixedString a_type, ClimbStartType a_startType) {
        a_actor->SetGraphVariableInt(a_type, a_startType);
    }
    static bool DoRayCast(RE::Actor* a_actor, RE::hkVector4 a_from, RE::hkVector4 a_to) {

        RE::hkpWorldRayCastInput input = { a_from, a_to, false, 0 };
        RE::hkpWorldRayCastOutput output = {};

        uint32_t collisionLayerMark = 0xFFFFFFFF - 0x1F;
        auto collisionResult = 0 + (collisionLayerMark & input.filterInfo);
        input.filterInfo = collisionResult;

        auto bhkWorld = a_actor->parentCell->GetbhkWorld();
        auto hkpWorld = bhkWorld->GetWorld();
        hkpWorld->CastRay(input, output);
        return output.HasHit();

    }
    static RE::NiPoint3 GetForwardVector(RE::NiPoint3 eulerIn)
    {

        return [eulerIn]() -> RE::NiPoint3 {

            float pitch = eulerIn.x;
            float yaw = eulerIn.z;
            return RE::NiPoint3(sin(yaw) * cos(pitch), cos(yaw) * cos(pitch), sin(pitch));

        }();


        float pitch = eulerIn.x;
        float yaw = eulerIn.z;

        return RE::NiPoint3(
            sin(yaw) * cos(pitch),
            cos(yaw) * cos(pitch),
            sin(pitch));
    }
    static RE::NiPoint3 RotateVector(RE::NiQuaternion quatIn, RE::NiPoint3 vecIn)
    {
        float num = quatIn.x * 2.0f;
        float num2 = quatIn.y * 2.0f;
        float num3 = quatIn.z * 2.0f;
        float num4 = quatIn.x * num;
        float num5 = quatIn.y * num2;
        float num6 = quatIn.z * num3;
        float num7 = quatIn.x * num2;
        float num8 = quatIn.x * num3;
        float num9 = quatIn.y * num3;
        float num10 = quatIn.w * num;
        float num11 = quatIn.w * num2;
        float num12 = quatIn.w * num3;
        RE::NiPoint3 result;
        result.x = (1.0f - (num5 + num6)) * vecIn.x + (num7 - num12) * vecIn.y + (num8 + num11) * vecIn.z;
        result.y = (num7 + num12) * vecIn.x + (1.0f - (num4 + num6)) * vecIn.y + (num9 - num10) * vecIn.z;
        result.z = (num8 - num11) * vecIn.x + (num9 + num10) * vecIn.y + (1.0f - (num4 + num5)) * vecIn.z;
        return result;
    }
    static RE::NiPoint3 GetForwardVector(RE::NiQuaternion quatIn)
    {
        // rotate Skyrim's base forward vector (positive Y forward) by quaternion

        return RotateVector(quatIn, RE::NiPoint3(0.0f, 1.0f, 0.0f));
    }
    static void Update(RE::Actor* a_actor) {

        auto ptr = Loki_Climbing::GetSingleton();
        using StartType = Loki_Climbing::ClimbStartType;
        using EndType = Loki_Climbing::ClimbEndType;

        if (a_actor->IsPlayerRef()) {
            auto controller = a_actor->GetCharController();
            controller->context.currentState = RE::hkpCharacterStateType::kClimbing;
            static bool isClimbing = false;

            /**
            RE::PlayerCharacter::GetSingleton()->questLog;


            auto CalculateForwardRaycast = [a_actor, ptr](float _dist, float _height) -> RE::hkVector4 {
                auto center = a_actor->GetCurrent3D()->worldBound.center;
                auto forwardvec = GetForwardVector(center);
                forwardvec.x *= _dist;
                forwardvec.y *= _dist;
                forwardvec.z += _height;
                auto normalized = forwardvec / forwardvec.Length();
                RE::hkVector4 hkv = { normalized.x, normalized.y, normalized.z, 0.00f };
                return hkv;
            };


            auto center = a_actor->GetCurrent3D()->worldBound.center;
            center.z += ptr->rayCastLowVaultHeight;
            RE::hkVector4 start = { center.x, center.y, center.z, 0.00f };
            auto forwardvec = GetForwardVector(center);
            forwardvec.x *= ptr->rayCastLowVaultDist;
            forwardvec.y *= ptr->rayCastLowVaultDist;
            forwardvec.z = 0.00f;
            auto normalized = forwardvec / forwardvec.Length();
            RE::hkVector4 end = { normalized.x, normalized.y, normalized.z, 0.00f };


            if (Loki_Climbing::DoRayCast(a_actor, start, end)) {
                bool jmp;
                a_actor->GetGraphVariableBool("CanJump", jmp);
                if (jmp) { // if jump input
                    a_actor->SetGraphVariableInt("climb_ClimbStartType", StartType::kSmallVault);
                    a_actor->NotifyAnimationGraph("climb_ClimbStart");  // start climb
                    //isClimbing = true;
                }
            }



            if (Loki_Climbing::DoRayCast(a_actor, [a_actor, ptr]() -> RE::hkVector4 {
                auto center = a_actor->GetCurrent3D()->worldBound.center;
                center.z += ptr->rayCastLowVaultHeight;
                RE::hkVector4 start = { center.x, center.y, center.z, 0.00f };
                return start;
            }(), CalculateForwardRaycast(ptr->rayCastLowVaultDist, ptr->rayCastLowVaultHeight))) {
                bool jmp;
                a_actor->GetGraphVariableBool("CanJump", jmp);
                if (jmp) {
                    a_actor->SetGraphVariableInt("climb_ClimbStartType", StartType::kSmallVault);
                    a_actor->NotifyAnimationGraph("climb_ClimbStart");  // start vault
                }
            } else if (Loki_Climbing::DoRayCast(a_actor, [a_actor, ptr]() -> RE::hkVector4 {
                auto center = a_actor->GetCurrent3D()->worldBound.center;
                center.z += ptr->rayCastMediumVaultHeight;
                RE::hkVector4 start = { center.x, center.y, center.z, 0.00f };
                return start;
            }(), CalculateForwardRaycast(ptr->rayCastMediumVaultDist, ptr->rayCastMediumVaultHeight))) {
                bool jmp;
                a_actor->GetGraphVariableBool("CanJump", jmp);
                if (jmp) {
                    a_actor->SetGraphVariableInt("climb_ClimbStartType", StartType::kMediumVault);
                    a_actor->NotifyAnimationGraph("climb_ClimbStart");  // start vault
                }
            } else if (Loki_Climbing::DoRayCast(a_actor, [a_actor, ptr]() -> RE::hkVector4 {
                auto center = a_actor->GetCurrent3D()->worldBound.center;
                center.z += ptr->rayCastLargeVaultHeight;
                RE::hkVector4 start = { center.x, center.y, center.z, 0.00f };
                return start;
            }(), CalculateForwardRaycast(ptr->rayCastLargeVaultDist, ptr->rayCastLargeVaultHeight))) {
                bool jmp;
                a_actor->GetGraphVariableBool("CanJump", jmp);
                if (jmp) {
                    a_actor->SetGraphVariableInt("climb_ClimbStartType", StartType::kLargeVault);
                    a_actor->NotifyAnimationGraph("climb_ClimbStart");  // start vault
                }
            } else if (Loki_Climbing::DoRayCast(a_actor, [a_actor, ptr]() -> RE::hkVector4 {
                auto center = a_actor->GetCurrent3D()->worldBound.center;
                center.z += ptr->rayCastClimbHeight;
                RE::hkVector4 start = { center.x, center.y, center.z, 0.00f };
                return start;
            }(), CalculateForwardRaycast(ptr->rayCastClimbDist, ptr->rayCastClimbHeight))) {
                bool jmp;
                a_actor->GetGraphVariableBool("CanJump", jmp);
                if (jmp) {
                    auto context = a_actor->GetCharController()->context;
                    if (isClimbing && a_actor->actorState1.sprinting) {
                        a_actor->SetGraphVariableInt("climb_ClimbEndType", EndType::kJumpBackwards);
                        if (a_actor->NotifyAnimationGraph("climb_ClimbEnd")) {
                            context.currentState = RE::hkpCharacterStateType::kJumping;
                            isClimbing = false;
                        }// end climb
                    } else if (isClimbing) {
                        a_actor->SetGraphVariableInt("climb_ClimbEndType", EndType::kJumpForwards);
                        if (a_actor->NotifyAnimationGraph("climb_ClimbEnd")) {
                            context.currentState = RE::hkpCharacterStateType::kJumping;
                            isClimbing = false;
                        }// end climb
                    } else if (context.currentState = RE::hkpCharacterStateType::kInAir) {
                        a_actor->SetGraphVariableInt("climb_ClimbStartType", StartType::kClimbFromAir);
                        if (a_actor->NotifyAnimationGraph("climb_ClimbStart")) {
                            context.currentState = RE::hkpCharacterStateType::kClimbing;
                            isClimbing = true;
                        }// start climb
                    } else {
                        a_actor->SetGraphVariableInt("climb_ClimbStartType", StartType::kClimbFromGround);
                        if (a_actor->NotifyAnimationGraph("climb_ClimbStart")) {
                            context.currentState = RE::hkpCharacterStateType::kClimbing;
                            isClimbing = true;
                        }// start climb
                    }
                }
            }

            if (Loki_Climbing::DoRayCast(a_actor, start, end)) {
                bool jmp;
                a_actor->GetGraphVariableBool("CanJump", jmp);
                if (jmp) { // if jump input
                    if (isClimbing) { // if we're already climbing
                        if (a_actor->actorState1.sprinting) {
                            a_actor->SetGraphVariableInt("climb_ClimbEndType", EndType::kJumpBackwards);
                            a_actor->NotifyAnimationGraph("climb_ClimbEnd");
                            a_actor->GetCharController()->context.currentState = RE::hkpCharacterStateType::kJumping;
                            bool isClimbing = false;
                        }
                        a_actor->SetGraphVariableInt("climb_ClimbEndType", EndType::kJumpForwards);
                        a_actor->NotifyAnimationGraph("climb_ClimbEnd");  // end climb
                        a_actor->GetCharController()->context.currentState = RE::hkpCharacterStateType::kJumping;
                        bool isClimbing = false;
                    } else { // if we're not climbing
                        if (a_actor->GetCharController()->context.currentState == RE::hkpCharacterStateType::kInAir) {
                            a_actor->SetGraphVariableInt("climb_ClimbStartType", StartType::kClimbFromAir);
                            if (a_actor->NotifyAnimationGraph("climb_ClimbStart")) {  // start climb
                                bool isClimbing = true;
                            }
                        } else { // if in air above, if on ground below
                            a_actor->SetGraphVariableInt("climb_ClimbStartType", StartType::kClimbFromGround);
                            a_actor->NotifyAnimationGraph("climb_ClimbStart");  // start climb
                            bool isClimbing = true;
                        }
                    }
                }
            }

            */

            /**
            RE::hkVector4 start;
            controller->GetPositionImpl(start, false);
            start.quad.m128_f32[2] += 5.00f;
            auto from = start;

            RE::NiPoint3 pos = a_actor->data.location;
            RE::NiPoint3 angl = a_actor->data.angle;
            auto forward = controller->forwardVec;
            for (int i = 0; i < 3; i++) {
                forward.quad.m128_f32[i] += 10.00f;
            }


            RE::hkVector4 end;
            float dist = ptr->rayCastDist;
            end.quad.m128_f32[0] = from.quad.m128_f32[0] + (cos(from.quad.m128_f32[3]) * dist); // x
            end.quad.m128_f32[1] = from.quad.m128_f32[1] + (sin(from.quad.m128_f32[3]) * dist); // y
            end.quad.m128_f32[2] = from.quad.m128_f32[2]; // z
            end.quad.m128_f32[3] = from.quad.m128_f32[3]; // a
            auto to = end;
            */

            /*
            if (Loki_Climbing::DoRayCast(a_actor, from, forward)) {
                RE::ConsoleLog::GetSingleton()->Print("Raycast has hit collision object");
                a_actor->SetGraphVariableBool("IsClimbing", true);
                a_actor->SetGraphVariableInt("IsClimb", 1);
                controller->context.currentState = RE::hkpCharacterStateType::kClimbing;
                controller->pitchAngle = -1.50f;

            } else {
                bool isClimbing = FALSE;
                a_actor->GetGraphVariableBool("IsClimbing", isClimbing);
                if (isClimbing) {
                    a_actor->SetGraphVariableBool("IsClimbing", false);
                    controller->context.currentState = controller->context.previousState;
                }

                //a_actor->SetGraphVariableBool("IsClimbing", false);
                //a_actor->SetGraphVariableInt("IsClimb", 0);
                //a_actor->GetCharController()->context.currentState = RE::hkpCharacterStateType::kJumping;
                //RE::hkpCharacterStateType::kInAir    could be good for shield surfing?
            }
            */

        }
        return _Update(a_actor);

    }
    static inline REL::Relocation<decltype(Update)> _Update;
    //static inline REL::Relocation<decltype(bhkCharacterStateClimbing_SimPhys)> _SimulateClimbPhysics;

};

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface * a_skse)
{
    logger::info("Climbing loaded");
    SKSE::Init(a_skse);
    SKSE::AllocTrampoline(128);

    Loki_Climbing::InstallUpdateHook();
    //Loki_Climbing::InstallSimulateClimbingHook();
    Loki_Climbing::InstallClimbSimHook();

    return true;
}

// TODO: Climbing in Holds is illegal
// TODO: Particles for material
// TODO: Difficulty climbing in harsh weather





/*
bool IAnimationGraphManagerHolder::SetGraphVariableFloat(const BSFixedString& a_variableName, float a_in)
{
    using func_t = decltype(&IAnimationGraphManagerHolder::SetGraphVariableFloat);
    REL::Relocation<func_t> func{ REL::ID(32143) };
    return func(this, a_variableName, a_in);
}
*/