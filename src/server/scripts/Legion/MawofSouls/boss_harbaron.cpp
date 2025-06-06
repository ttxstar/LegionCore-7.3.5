/*
    Dungeon : Maw of Souls 100-110
    Encounter: Harbaron
    Normal: 100%, Heroic: 100%, Mythic: 100%
*/

#include "maw_of_souls.h"
#include "ScriptPCH.h"

enum Says
{
    SAY_INTRO                       = 0,
    SAY_AGGRO                       = 1,
    SAY_SUMMON                      = 2,
    SAY_FRAGMENT                    = 3,
    SAY_EMOTE_FRAGMENT              = 4,
    SAY_DEATH                       = 5,
}; 

enum Spells
{
    SPELL_COSMIC_SCYTHE             = 205330,
    SPELL_COSMIC_SCYTHE_VIS         = 198580,
    SPELL_COSMIC_SCYTHE_VIS_2       = 194667,
    SPELL_COSMIC_SCYTHE_DMG         = 194218,
    SPELL_SUM_SHACKLED_SERVITOR     = 194231,
    SPELL_FRAGMENT                  = 194325,

    //Heroic
    SPELL_NETHER_RIP_AURA           = 194668,

    //Summons
    SPELL_SHACKLED_SERVITOR         = 194259,
    SPELL_VOID_SNAP                 = 194266,
};

enum eEvents
{
    EVENT_COSMIC_SCYTHE             = 1,
    EVENT_SUM_SHACKLED_SERVITOR     = 2,
    EVENT_FRAGMENT                  = 3,
    EVENT_NETHER_RIP                = 4,
};

enum eMisc
{
    DATA_ACHIEVEMENT                = 0
};

//96754
struct boss_harbaron : public BossAI
{
    boss_harbaron(Creature* creature) : BossAI(creature, DATA_HARBARON) {}

    std::list<ObjectGuid> listGuid;
    std::list<ObjectGuid> trashGUID;
    bool _introDone = false;
    bool achievement = false;
    uint8 scytheRange = 0;

    void Reset() override
    {
        _Reset();
        achievement = true;
        trashGUID.clear();
    }

    void EnterCombat(Unit* /*who*/) override
    {
        Talk(SAY_AGGRO);
        _EnterCombat();
        me->SetReactState(REACT_AGGRESSIVE);

        events.RescheduleEvent(EVENT_COSMIC_SCYTHE, 4000);
        events.RescheduleEvent(EVENT_SUM_SHACKLED_SERVITOR, 7000);
        events.RescheduleEvent(EVENT_FRAGMENT, 18000);

        if (GetDifficultyID() != DIFFICULTY_NORMAL)
            events.RescheduleEvent(EVENT_NETHER_RIP, 13000);
    }

    void EnterEvadeMode()
    {
        DespawnSouls();
        BossAI::EnterEvadeMode();
    }

    void JustDied(Unit* /*killer*/) override
    {
        Talk(SAY_DEATH);
        DespawnSouls();
        _JustDied();

        if (auto skjal = instance->instance->GetCreature(instance->GetGuidData(DATA_SKJAL)))
            skjal->AI()->DoAction(ACTION_1);

        if (auto valkyra = me->FindNearestCreature(104906, 200.f, true))
            if (valkyra->IsAIEnabled)
                valkyra->AI()->DoAction(ACTION_2);

        if (CheckAura())
        {
            instance->DoAddAuraOnPlayers(213413);
            instance->DoRemoveAurasDueToSpellOnPlayers(213441);
        }
    }

    uint32 GetData(uint32 type) const override
    {
        if (type == DATA_ACHIEVEMENT)
            return achievement;

        return 0;
    }

    void DoAction(int32 const action) override
    {
        if (action == ACTION_1)
            achievement = false;
    }

    bool CheckAura()
    {
        bool check = true;

        instance->instance->ApplyOnEveryPlayer([&](Player* player)
        {
            if (!player->HasAura(213441))
                check = false;
        });

        return check;
    }

    void DespawnSouls()
    {
        for (auto const& guid : trashGUID)
            if (auto soul = Creature::GetCreature(*me, guid))
                if (soul->IsInWorld())
                    soul->DespawnOrUnsummon();
    }

    void SpellHitTarget(Unit* target, const SpellInfo* spell) override
    {
        if (spell->Id == SPELL_COSMIC_SCYTHE)
        {
            Position pos;
            float angle = me->GetRelativeAngle(target);
            scytheRange = 0.0f;

            for (uint8 i = 0; i < 6; ++i)
            {
                scytheRange += 5;
                pos = me->GetNearPosition(scytheRange, angle);
                SummonCreatureDelay(i * 100, NPC_COSMIC_SCYTHE_2, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), angle * 2.0f);
            }
        }

        if (spell->Id == SPELL_FRAGMENT)
        {
            uint8 maxCount = GetDifficultyID() == DIFFICULTY_MYTHIC_DUNGEON ? 4 : 2;
            Position pos;
            float angle = frand(0.0f, 6.28f);

            for (uint8 i = 0; i < maxCount; ++i)
            {
                pos = target->GetFirstCollisionPosition(1.5f, angle);
                angle += 6.28f / maxCount;
                if (auto soul = target->SummonCreature(NPC_SOUL_FRAGMENT, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 5000))
                    trashGUID.push_back(soul->GetGUID());
            }
        }
    }

    void MoveInLineOfSight(Unit* who) override
    {  
        if (!who->IsPlayer())
            return;

        if (!_introDone && me->IsWithinDistInMap(who, 45.0f))
        {
            _introDone = true;
            Talk(SAY_INTRO);
        }
        BossAI::MoveInLineOfSight(who);
    }

    bool GetObjectData(ObjectGuid const& guid) override
    {
        bool find = false;

        for (auto targetGuid : listGuid)
            if (targetGuid == guid)
                find = true;

        if (!find)
            listGuid.push_back(guid);

        return find;
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (CheckHomeDistToEvade(diff, 3.0f, 0.0, 0.0f, 554.28f, true))
            return;

        if (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_COSMIC_SCYTHE:
                    listGuid.clear();
                    me->StopAttack(true);
                    me->SetReactState(REACT_AGGRESSIVE, 2000);
                    DoCast(SPELL_COSMIC_SCYTHE);
                    events.RescheduleEvent(EVENT_COSMIC_SCYTHE, 8000);
                    break;
                case EVENT_SUM_SHACKLED_SERVITOR:
                {
                    Talk(SAY_SUMMON);
                    Position pos = me->GetFirstCollisionPosition(15.0f, frand(0.0f, 6.28f));
                    me->CastSpell(pos, SPELL_SUM_SHACKLED_SERVITOR, false);
                    events.RescheduleEvent(EVENT_SUM_SHACKLED_SERVITOR, 23000);
                    break;
                }
                case EVENT_FRAGMENT:
                    Talk(SAY_FRAGMENT);
                    if (auto pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 60.0f, true))
                    {
                        Talk(SAY_EMOTE_FRAGMENT, pTarget->GetGUID());
                        DoCast(pTarget, SPELL_FRAGMENT);
                    }
                    events.RescheduleEvent(EVENT_FRAGMENT, 30000);
                    break;
                case EVENT_NETHER_RIP:
                    me->AddAura(SPELL_NETHER_RIP_AURA, me);
                    events.RescheduleEvent(EVENT_NETHER_RIP, 13000);
                    break;
            }
        }
        DoMeleeAttackIfReady();
    }
};

//100839
struct npc_harbaron_scythe : public ScriptedAI
{
    npc_harbaron_scythe(Creature* creature) : ScriptedAI(creature) 
    {
        me->SetReactState(REACT_PASSIVE);
    }

    EventMap events;

    void Reset() override
    {
        DoCast(me, SPELL_COSMIC_SCYTHE_VIS, true);
        events.RescheduleEvent(EVENT_1, 2000);
    }

    void UpdateAI(uint32 diff) override
    {
        events.Update(diff);

        if (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_1:
                    me->RemoveAurasDueToSpell(SPELL_COSMIC_SCYTHE_VIS);
                    me->CastSpell(me, SPELL_COSMIC_SCYTHE_VIS_2, true);
                    me->CastSpell(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ() - 5.0f, SPELL_COSMIC_SCYTHE_DMG);
                    me->DespawnOrUnsummon(500);
                    break;
            }
        }
    }
};

//98693
struct npc_harbaron_shackled_servitor : public ScriptedAI
{
    npc_harbaron_shackled_servitor(Creature* creature) : ScriptedAI(creature) 
    {
        SetCombatMovement(false);
    }

    EventMap events;

    void Reset() override {}

    void IsSummonedBy(Unit* summoner) override
    {
        DoZoneInCombat(me, 100.0f);
        DoCast(me, SPELL_SHACKLED_SERVITOR, true);
    }

    void EnterCombat(Unit* /*who*/) override
    {
        events.RescheduleEvent(EVENT_1, 2000);
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_1:
                    DoCast(SPELL_VOID_SNAP);
                    events.RescheduleEvent(EVENT_1, 6000);
                    break;
            }
        }
    }
};

//98761
struct npc_harbaron_soul_fragment : public ScriptedAI
{
    npc_harbaron_soul_fragment(Creature* creature) : ScriptedAI(creature) 
    {
        me->SetReactState(REACT_PASSIVE);
    }

    void Reset() override {}

    void IsSummonedBy(Unit* summoner) override
    {
        summoner->CastSpell(me, 194345, true); //Clone Caster
        me->CastSpell(summoner, 194344, true); //Dmg tick summoner
        me->CastSpell(me, 194381, true); //Scale
        me->SetHomePosition(2933.75f, 683.98f, 553.48f, 0.0f);
        me->GetMotionMaster()->MoveRandom(10.0f);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (auto summoner = me->GetAnyOwner())
        {
            if (auto aura = summoner->GetAura(194344, me->GetGUID()))
                aura->Remove();

            if (!summoner->HasAura(194344))
                summoner->RemoveAurasDueToSpell(194327);
        }
    }

    void UpdateAI(uint32 diff) override {}
};

//104906
struct npc_harbaron_captured_valkyr : public ScriptedAI
{
    npc_harbaron_captured_valkyr(Creature* creature) : ScriptedAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetCanFly(true);
    }

    uint32 timer = 0;

    void DoAction(int32 const action) override
    {
        if (action == ACTION_1)
            timer = 240000;

        if (action == ACTION_2)
        {
            timer = 0;
            Talk(2);
            me->DespawnOrUnsummon(5000);
        }
    }

    void Reset() override {}

    void UpdateAI(uint32 diff) override 
    {
        if (timer)
        {
            if (timer <= diff)
            {
                timer = 240000;

                Talk(1);
                if (auto boss = me->FindNearestCreature(96754, 250.f))
                    if (boss->IsAIEnabled)
                        boss->AI()->DoAction(ACTION_1);

                me->DespawnOrUnsummon(2000);
            }
            else
                timer -= diff;
        }
    }
};

//98919
struct npc_mos_seacursed_swiftblade : public ScriptedAI
{
    npc_mos_seacursed_swiftblade(Creature* creature) : ScriptedAI(creature) {}

    EventMap events;
    uint8 healtPct = 45;
    uint32 removeFlagsTimer = 0;

    void Reset() override
    {
        events.Reset();
        me->SetReactState(REACT_AGGRESSIVE);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
    }

    void EnterCombat(Unit* /*who*/) override
    {
        events.RescheduleEvent(EVENT_1, urandms(3, 4));
        events.RescheduleEvent(EVENT_2, urandms(7, 9));
    }

    void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType dmgType) override
    {
        if (HealthBelowPct(healtPct))
        {
            healtPct = 0;
            DoCast(194615);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!UpdateVictim())
            return;

        events.Update(diff);

        if (removeFlagsTimer)
        {
            if (removeFlagsTimer <= diff)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
            }
            else
                removeFlagsTimer -= diff;
        }

        if (me->HasUnitState(UNIT_STATE_CASTING))
            return;

        if (uint32 eventId = events.ExecuteEvent())
        {
            switch (eventId)
            {
                case EVENT_1:
                    DoCastVictim(199327);
                    events.RescheduleEvent(EVENT_1, urandms(9, 11));
                    break;
                case EVENT_2:
                {
                    me->StopAttack(true);
                    me->SetReactState(REACT_AGGRESSIVE, 4300);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                    removeFlagsTimer = 4300;
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM))
                    {
                        DoCast(199250);
                        DoCast(185325);
                        me->CastSpell(target, 199232);
                    }
                    events.RescheduleEvent(EVENT_2, urandms(17, 20));
                    break;
                }
            }
        }
        DoMeleeAttackIfReady();
    }
};

//194668
class spell_harbaron_nether_rip : public AuraScript
{
    PrepareAuraScript(spell_harbaron_nether_rip);

    void PeriodicTick(AuraEffect const* aurEff)
    {
        if (!GetCaster())
            return;

        if (GetCaster()->GetMap()->GetDifficultyID() == DIFFICULTY_MYTHIC_DUNGEON || GetCaster()->GetMap()->GetDifficultyID() == DIFFICULTY_MYTHIC_KEYSTONE)
            if (aurEff->GetTickNumber() == 1 || aurEff->GetTickNumber() == 3)
                return;

        if (aurEff->GetTickNumber() % 2)
        {
            Position pos = GetCaster()->GetRandomNearPosition(10.0f);
            GetCaster()->CastSpell(pos, 199457, true); //Hack. 198726
        }
        else
            GetCaster()->CastSpell(GetCaster(), 198724, true);
        //and else 198727?
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_harbaron_nether_rip::PeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

//194218
class spell_harbaron_cosmic_scythe_filter : public SpellScript
{
    PrepareSpellScript(spell_harbaron_cosmic_scythe_filter);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        if (!GetCaster())
            return;

        targets.remove_if([this](WorldObject* object) -> bool
        {
            if (object == nullptr)
                return true;

            if (auto owner = GetCaster()->GetAnyOwner())
                if (owner->GetAI()->GetObjectData(object->GetGUID()))
                    return true;

            return false;
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_harbaron_cosmic_scythe_filter::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

//1756
class eo_mos_valkyr_data : public EventObjectScript
{
public:
    eo_mos_valkyr_data() : EventObjectScript("eo_mos_valkyr_data") {}

    bool meeted = false;

    bool IsTriggerMeets(Player* player, EventObject* eo) override
    {
        if (!meeted)
        {
            meeted = true;

            if (auto valkyr = player->FindNearestCreature(104906, 500.f, true))
            {
                if (valkyr->IsAIEnabled)
                    valkyr->AI()->DoAction(ACTION_1);

                ObjectGuid valkyrGUID = valkyr->GetGUID();
                if (valkyrGUID.IsEmpty())
                    return false;

                sCreatureTextMgr->SendChat(valkyr, 0, valkyrGUID, CHAT_MSG_MONSTER_EMOTE, LANG_UNIVERSAL, TEXT_RANGE_ZONE, 0, TEAM_OTHER, false);
            }
            return true;
        }

        return false;
    }
};

//29683
class achievement_helheim_hath_no_fury : public AchievementCriteriaScript
{
public:
    achievement_helheim_hath_no_fury() : AchievementCriteriaScript("achievement_helheim_hath_no_fury") { }

    bool OnCheck(Player* /*player*/, Unit* target) override
    {
        if (!target)
            return false;

        if (auto boss = target->ToCreature())
            if (boss->GetMap()->GetDifficultyID() == DIFFICULTY_MYTHIC_DUNGEON || boss->GetMap()->GetDifficultyID() == DIFFICULTY_MYTHIC_KEYSTONE)
                if (boss->GetAI()->GetData(DATA_ACHIEVEMENT))
                    return true;

        return false;
    }
};

void AddSC_boss_harbaron()
{
    RegisterCreatureAI(boss_harbaron);
    RegisterCreatureAI(npc_harbaron_scythe);
    RegisterCreatureAI(npc_harbaron_shackled_servitor);
    RegisterCreatureAI(npc_harbaron_soul_fragment);
    RegisterCreatureAI(npc_harbaron_captured_valkyr);
    RegisterCreatureAI(npc_mos_seacursed_swiftblade);
    RegisterAuraScript(spell_harbaron_nether_rip);
    RegisterSpellScript(spell_harbaron_cosmic_scythe_filter);
    new eo_mos_valkyr_data();
    new achievement_helheim_hath_no_fury();
}