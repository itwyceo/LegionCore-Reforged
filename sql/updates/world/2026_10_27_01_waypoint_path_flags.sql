-- Optional per-path waypoint metadata.
--
-- Bit 0 (WAYPOINT_PATH_FLAG_FOLLOW_PATH_BACKWARDS_FROM_END, value 1):
--   after reaching the last node the creature walks the node list backwards to the first node
--   and then turns around again -- i.e. 0 1 2 3 2 1 0 1 2 3 ... -- instead of wrapping from the
--   last node straight back to node 0.
--
-- The table is optional by design: a path with no row here keeps the legacy looping behaviour,
-- so an empty table changes nothing at all for the existing paths.
--
-- The flag is only honoured by repeating generators (the natural patrol generator and every
-- MovePath(id, true) call). MovePath(id, false) still stops at the last node, because those
-- callers (minecarts, one-shot boss fire paths, summons flying in) must not walk back.
--
-- To make a single path ping-pong:
--   INSERT INTO `waypoint_path` (`PathId`, `Flags`, `Entry`)
--   VALUES (1234, 1, 37944);
--
-- `Entry` is the creature_template entry the path belongs to. It is documentation only: the core
-- selects `PathId, Flags` and never reads `Entry`, so a wrong or missing value changes nothing.

CREATE TABLE IF NOT EXISTS `waypoint_path` (
  `PathId` int unsigned NOT NULL,
  `Flags` int unsigned NOT NULL DEFAULT 0,
  `Entry` int unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`PathId`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Optional per-path waypoint metadata. Flags bit 0 = walk the path backwards from end to start (ping-pong).';
