-- Assault on Violet Hold: Lyndras' Prison Barrier (gameobject 247005) opens with the Violet Hold Prison Key (135556)
-- only. Lock 2823 asks for the key, so the barrier's tooltip shows "Requires Violet Hold Prison Key" as on
-- lockpickable chests; the key gets the usual key spell (Opening, 3366) to open it. The old lock (93, Open Tinkering)
-- let anyone open the barrier.
DELETE FROM `lock` WHERE `ID` = 2823;
INSERT INTO `lock` (`ID`, `Index1`, `Index2`, `Index3`, `Index4`, `Index5`, `Index6`, `Index7`, `Index8`, `Skill1`, `Skill2`, `Skill3`, `Skill4`,
    `Skill5`, `Skill6`, `Skill7`, `Skill8`, `Type1`, `Type2`, `Type3`, `Type4`, `Type5`, `Type6`, `Type7`, `Type8`, `Action1`, `Action2`, `Action3`,
    `Action4`, `Action5`, `Action6`, `Action7`, `Action8`) VALUES
(2823, 135556, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

DELETE FROM `item_effect` WHERE `ID` = 89540;
INSERT INTO `item_effect` (`ID`, `SpellID`, `CoolDownMSec`, `CategoryCoolDownMSec`, `Charges`, `SpellCategoryID`, `ChrSpecializationID`, `LegacySlotIndex`,
    `TriggerType`, `ItemID`) VALUES
(89540, 3366, 0, 0, 0, 0, 0, 0, 0, 135556);

DELETE FROM `hotfix_data` WHERE `Id` IN (9009102, 9009103);
INSERT INTO `hotfix_data` (`Id`, `TableHash`, `RecordID`, `Timestamp`, `Deleted`) VALUES
(9009102, 3921595171, 2823, 0, 0),
(9009103, 1073915313, 89540, 0, 0);

UPDATE `world_legion`.`version` SET `hotfix_cache_id` = 436;
