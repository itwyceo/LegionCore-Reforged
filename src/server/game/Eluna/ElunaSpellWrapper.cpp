/*
* Copyright (C) 2010 - 2024 Eluna Lua Engine <https://elunaluaengine.github.io/>
* This program is free software licensed under GPL version 3
* Please see the included DOCS/LICENSE.md for more information
*/
#include "ElunaSpellWrapper.h"
#include "ElunaIncludes.h"
#include "ElunaTemplate.h"
#include <memory>
ElunaProcInfo::ElunaProcInfo(Unit* actor, Unit* actionTarget, uint32 typeMask,
    uint32 spellTypeMask, uint32 spellPhaseMask, uint32 hitMask,
    Spell* spell, SpellInfo const* spellInfo, SpellSchoolMask schoolMask, Map* map)
    : _actor(actor), _actionTarget(actionTarget), _typeMask(typeMask), _spellTypeMask(spellTypeMask), _spellPhaseMask(spellPhaseMask)
    , _hitMask(hitMask), _spell(spell), _spellInfo(spellInfo), _schoolMask(schoolMask), _damage(0)
    , _damageType(DIRECT_DAMAGE), _attackType(BASE_ATTACK), _damageAbsorb(0), _resist(0), _block(0)
    , _heal(0), _effectiveHeal(0), _healAbsorb(0), _map(map)
#ifdef TRACKABLE_PTR_NAMESPACE
    , m_scriptRef(this, NoopAuraDeleter())
#endif
{
}

ElunaProcInfo::ElunaProcInfo(ProcEventInfo& procInfo, Map* map)
    : _actor(procInfo.GetActor()), _actionTarget(procInfo.GetActionTarget()), _typeMask(procInfo.GetTypeMask()), _spellTypeMask(procInfo.GetSpellTypeMask()), _spellPhaseMask(procInfo.GetSpellPhaseMask())
    , _hitMask(procInfo.GetHitMask()), _spell(procInfo.GetSpell()), _spellInfo(procInfo.GetSpellInfo()), _schoolMask(procInfo.GetSchoolMask()), _damage(0)
    , _damageType(DIRECT_DAMAGE), _attackType(BASE_ATTACK), _damageAbsorb(0), _resist(0), _block(0)
    , _heal(0), _effectiveHeal(0), _healAbsorb(0), _map(map)
#ifdef TRACKABLE_PTR_NAMESPACE
    , m_scriptRef(this, NoopAuraDeleter())
#endif
{
    if (DamageInfo* damageInfo = procInfo.GetDamageInfo())
    {
        _damage = damageInfo->GetDamage();
        _damageType = damageInfo->GetDamageType();
        _attackType = damageInfo->GetAttackType();
        _damageAbsorb = damageInfo->GetAbsorb();
        _resist = damageInfo->GetResist();
        _block = damageInfo->GetBlock();

        if (damageInfo->GetSpellInfo())
        {
            _spellInfo = damageInfo->GetSpellInfo();
            _schoolMask = damageInfo->GetSchoolMask();
        }
    }

    if (HealInfo* healInfo = procInfo.GetHealInfo())
    {
        // Reforged 的 HealInfo 只暴露 GetHeal()/AbsorbHeal()，没有 effective/absorb/spellInfo 访问器
        _heal = healInfo->GetHeal();
        _effectiveHeal = healInfo->GetHeal();
        _healAbsorb = 0;

        if (!_spellInfo && _spell)
        {
            _spellInfo = _spell->GetSpellInfo();
            if (_spellInfo)
                _schoolMask = _spellInfo->GetSchoolMask();
        }
    }
}

SpellInfo const* ElunaProcInfo::GetSpellInfo() const
{
    if (_spellInfo)
        return _spellInfo;
    if (_spell)
        return _spell->GetSpellInfo();
    return nullptr;
}

void ElunaProcInfo::SetDamage(uint32 damage, DamageEffectType damageType, WeaponAttackType attackType)
{
    _damage = damage;
    _damageType = damageType;
    _attackType = attackType;
}

void ElunaProcInfo::SetHeal(uint32 heal)
{
    _heal = heal;
    _effectiveHeal = heal;
}

void ElunaProcInfo::ApplyToProcEventInfo(ProcEventInfo& procInfo) const
{
    if (DamageInfo* damageInfo = procInfo.GetDamageInfo())
    {
        if (HasDamage())
        {
            int32 damageDiff = static_cast<int32>(_damage) - static_cast<int32>(damageInfo->GetDamage());
            if (damageDiff != 0)
                damageInfo->ModifyDamage(damageDiff);

            uint32 currentAbsorb = damageInfo->GetAbsorb();
            uint32 currentResist = damageInfo->GetResist();
            uint32 currentBlock = damageInfo->GetBlock();

            uint32 absorbToAdd = (_damageAbsorb > currentAbsorb) ? (_damageAbsorb - currentAbsorb) : 0;
            uint32 resistToAdd = (_resist > currentResist) ? (_resist - currentResist) : 0;
            uint32 blockToAdd = (_block > currentBlock) ? (_block - currentBlock) : 0;

            if (absorbToAdd > 0)
                damageInfo->AbsorbDamage(absorbToAdd);
            if (resistToAdd > 0)
                damageInfo->ResistDamage(resistToAdd);
            if (blockToAdd > 0)
                damageInfo->BlockDamage(blockToAdd);
        }
    }

    if (HealInfo* healInfo = procInfo.GetHealInfo())
    {
        if (HasHeal())
        {
            // Reforged 的 HealInfo 只能通过 AbsorbHeal 下调治疗量，无法直接设置有效治疗
            uint32 currentHeal = healInfo->GetHeal();
            if (_heal < currentHeal)
                healInfo->AbsorbHeal(currentHeal - _heal);
        }
    }
}

ElunaSpellInfo::ElunaSpellInfo(uint32 spellId) : _spellInfo(sSpellMgr->GetSpellInfo(spellId))
{
#ifdef ELUNA_TRINITY
    if (_spellInfo)
        m_scriptRef = std::shared_ptr<ElunaSpellInfo>(this, NoopAuraDeleter());
#endif
}
