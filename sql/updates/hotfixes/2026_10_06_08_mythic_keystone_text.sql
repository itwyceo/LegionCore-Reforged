-- Mythic Keystone (138019): the server's own ItemSparse row (the item is not in the 7.3.5 client files) carried a
-- machine translation ("Put the key in the Power Cup...") and had no French text at all.
UPDATE `item_sparse` SET `Display` = 'Mythic Keystone',
    `Description` = 'Place within the Font of Power inside the dungeon on Mythic difficulty.' WHERE `ID` = 138019;

DELETE FROM `item_sparse_locale` WHERE `ID` = 138019 AND `locale` = 'frFR';
INSERT INTO `item_sparse_locale` (`ID`, `locale`, `Display_lang`, `Display1_lang`, `Display2_lang`, `Display3_lang`, `Description_lang`) VALUES
(138019, 'frFR', 'Clé mythique', '', '', '', 'À placer dans la Fontaine de puissance du donjon, en difficulté mythique.');

DELETE FROM `hotfix_data` WHERE `Id` = 9009104;
INSERT INTO `hotfix_data` (`Id`, `TableHash`, `RecordID`, `Timestamp`, `Deleted`) VALUES
(9009104, 2442913102, 138019, 0, 0);

UPDATE `world_legion`.`version` SET `hotfix_cache_id` = 437;
