-- Assault on Violet Hold, Millificent Manastorm: the Image of Millhouse Manastorm (102040) never spoke. His seven lines
-- were all stored in group 3 with a broken Russian text, while boss_millificent_manastorm.cpp asks for groups 0-9.
-- Each line takes the group of its place in the dialogue, with the official English text (French comes from the
-- broadcast text); type and sound as before.
DELETE FROM `creature_text` WHERE `CreatureID` = 102040;
INSERT INTO `creature_text` (`CreatureID`, `GroupID`, `ID`, `Text`, `Type`, `Language`, `Probability`, `Emote`, `Duration`, `Sound`, `BroadcastTextID`, `comment`)
SELECT 102040, g.GroupID, 0, b.Text, g.Type, 0, 100, 0, 0, g.Sound, g.BroadcastTextID, 'Image of Millhouse Manastorm'
FROM (SELECT 0 AS GroupID, 104412 AS BroadcastTextID, 12 AS Type, 57474 AS Sound
      UNION ALL SELECT 1, 104414, 14, 57472      -- Oh buttons, what have you done?
      UNION ALL SELECT 2, 104417, 14, 57467      -- rocket chickens
      UNION ALL SELECT 3, 104418, 12, 57480      -- make fun of her hair
      UNION ALL SELECT 6, 104424, 12, 57485      -- Listen Milly, I can explain!
      UNION ALL SELECT 9, 104426, 14, 57483      -- That's not even my middle name!
      UNION ALL SELECT 8, 104427, 12, 57482) g   -- Thanks a lot. (once she is beaten)
JOIN `hotfixes_legion`.`broadcast_text` b ON b.ID = g.BroadcastTextID;
