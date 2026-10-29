-- Timewalking: each expansion's marker becomes a visible buff (icon, name, text), like the Sign of the Warrior.
DELETE FROM `spell` WHERE `ID` IN (193101, 193102, 201001, 233495);
INSERT INTO `spell` (`ID`, `Name`, `NameSubtext`, `Description`, `AuraDescription`, `VerifiedBuild`) VALUES
(193101, 'Timewalking: The Burning Crusade', '', 'The Burning Crusade Timewalking dungeons are open this week.', 'The Burning Crusade Timewalking dungeons are open this week.', 26972),
(193102, 'Timewalking: Wrath of the Lich King', '', 'Wrath of the Lich King Timewalking dungeons are open this week.', 'Wrath of the Lich King Timewalking dungeons are open this week.', 26972),
(201001, 'Timewalking: Cataclysm', '', 'Cataclysm Timewalking dungeons are open this week.', 'Cataclysm Timewalking dungeons are open this week.', 26972),
(233495, 'Timewalking: Mists of Pandaria', '', 'Mists of Pandaria Timewalking dungeons are open this week.', 'Mists of Pandaria Timewalking dungeons are open this week.', 26972);

DELETE FROM `spell_locale` WHERE `ID` IN (193101, 193102, 201001, 233495) AND `locale` = 'frFR';
INSERT INTO `spell_locale` (`ID`, `locale`, `Name_lang`, `NameSubtext_lang`, `Description_lang`, `AuraDescription_lang`, `VerifiedBuild`) VALUES
(193101, 'frFR', 'Marcheurs du temps : The Burning Crusade', '', 'Les donjons des Marcheurs du temps de The Burning Crusade sont ouverts cette semaine.', 'Les donjons des Marcheurs du temps de The Burning Crusade sont ouverts cette semaine.', 26972),
(193102, 'frFR', 'Marcheurs du temps : Wrath of the Lich King', '', 'Les donjons des Marcheurs du temps de Wrath of the Lich King sont ouverts cette semaine.', 'Les donjons des Marcheurs du temps de Wrath of the Lich King sont ouverts cette semaine.', 26972),
(201001, 'frFR', 'Marcheurs du temps : Cataclysm', '', 'Les donjons des Marcheurs du temps de Cataclysm sont ouverts cette semaine.', 'Les donjons des Marcheurs du temps de Cataclysm sont ouverts cette semaine.', 26972),
(233495, 'frFR', 'Marcheurs du temps : Mists of Pandaria', '', 'Les donjons des Marcheurs du temps de Mists of Pandaria sont ouverts cette semaine.', 'Les donjons des Marcheurs du temps de Mists of Pandaria sont ouverts cette semaine.', 26972);

-- shown like Sign of the Warrior (225787): its attributes, endless, with the Timewalking aura's icon
DELETE FROM `spell_misc` WHERE `ID` IN (167161, 167162, 175010, 207373);
INSERT INTO `spell_misc` (`ID`, `CastingTimeIndex`, `DurationIndex`, `RangeIndex`, `SchoolMask`, `SpellIconFileDataID`, `Speed`, `ActiveIconFileDataID`, `LaunchDelay`, `DifficultyID`, `Attributes1`, `Attributes2`, `Attributes3`, `Attributes4`, `Attributes5`, `Attributes6`, `Attributes7`, `Attributes8`, `Attributes9`, `Attributes10`, `Attributes11`, `Attributes12`, `Attributes13`, `Attributes14`, `SpellID`, `VerifiedBuild`) VALUES
(167161, 1, 21, 1, 1, 237538, 0, 0, 0, 0, -1451229184, 1056, 268976133, 1245184, 8388736, 393224, 12292, 0, 0, 0, 0, 0, 0, 512, 193101, 26972),
(167162, 1, 21, 1, 1, 237538, 0, 0, 0, 0, -1451229184, 1056, 268976133, 1245184, 8388736, 393224, 12292, 0, 0, 0, 0, 0, 0, 512, 193102, 26972),
(175010, 1, 21, 1, 1, 237538, 0, 0, 0, 0, -1451229184, 1056, 268976133, 1245184, 8388736, 393224, 12292, 0, 0, 0, 0, 0, 0, 512, 201001, 26972),
(207373, 1, 21, 1, 1, 237538, 0, 0, 0, 0, -1451229184, 1056, 268976133, 1245184, 8388736, 393224, 12292, 0, 0, 0, 0, 0, 0, 512, 233495, 26972);

DELETE FROM `hotfix_data` WHERE `Id` BETWEEN 9009197 AND 9009207;
INSERT INTO `hotfix_data` (`Id`, `TableHash`, `RecordID`, `Timestamp`, `Deleted`) VALUES
(9009197, 3776013982, 193101, 0, 0),
(9009198, 3776013982, 193102, 0, 0),
(9009199, 3776013982, 201001, 0, 0),
(9009200, 3776013982, 233495, 0, 0),
(9009202, 3322146344, 167161, 0, 0),
(9009203, 3322146344, 167162, 0, 0),
(9009204, 3322146344, 175010, 0, 0),
(9009205, 3322146344, 207373, 0, 0);

UPDATE `world_legion`.`version` SET `hotfix_cache_id` = `hotfix_cache_id` + 1;
