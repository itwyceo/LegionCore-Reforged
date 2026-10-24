/*
 * Hallow's End -- Throw Bucket (42340)
 *
 * Item 32971 (Water Bucket) creates a charged item that casts 42340 on use.
 * The spell can also be cast from the spell icon or a macro, which bypasses
 * the item entirely. This script makes both paths behave the same way:
 *
 *   - icon/macro cast: requires a bucket in the bags and consumes one;
 *   - item cast: the bucket is consumed after the throw, even when the item
 *     still has charges left (otherwise one bucket would be good for 20 throws).
 */

#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellScript.h"

enum HallowsEndThrowBucket
{
    ITEM_WATER_BUCKET = 32971,
};

// 42340 - Throw Bucket
class spell_hallows_end_throw_bucket : public SpellScript
{
    PrepareSpellScript(spell_hallows_end_throw_bucket);

    SpellCastResult CheckBucket()
    {
        _fromItem = GetCastItem() != nullptr;
        if (_fromItem)
            return SPELL_CAST_OK;

        // Cast from the spell icon or a macro: a bucket is still required
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return SPELL_CAST_OK;

        if (!player->HasItemCount(ITEM_WATER_BUCKET))
            return SPELL_FAILED_ITEM_NOT_FOUND;

        return SPELL_CAST_OK;
    }

    void HandleAfterCast()
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;

        if (_fromItem)
        {
            // Charged buckets survive TakeCastItem(): consume the whole bucket so one click = one bucket
            if (Item* castItem = GetCastItem())
                if (castItem->IsInWorld())
                {
                    uint32 count = 1;
                    player->DestroyItemCount(castItem, count, true);
                }
            return;
        }

        // Cast from the spell icon or a macro: consume one bucket from the bags
        player->DestroyItemCount(ITEM_WATER_BUCKET, 1, true);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_hallows_end_throw_bucket::CheckBucket);
        AfterCast += SpellCastFn(spell_hallows_end_throw_bucket::HandleAfterCast);
    }

    bool _fromItem = false;
};

void AddSC_hallows_end_throw_bucket()
{
    RegisterSpellScript(spell_hallows_end_throw_bucket);
}
