ALTER TABLE queues ADD COLUMN check_target_length boolean NOT NULL default true;
ALTER TABLE queues_targets ADD COLUMN repeat_n integer NOT NULL default -1;
ALTER TABLE queues_targets ADD COLUMN repeat_separation float;

ALTER TABLE targets ALTER COLUMN tar_bonus_time TYPE timestamp with time zone;
ALTER TABLE targets ALTER COLUMN tar_next_observable TYPE timestamp with time zone;

-- fix timestamps; future database will be build with time zone data, but for old ones, we need to alter tables
-- this can be done on fly
ALTER TABLE grb ALTER COLUMN grb_date TYPE timestamp with time zone;
ALTER TABLE grb ALTER COLUMN grb_last_update TYPE timestamp with time zone;

ALTER TABLE grb_gcn ALTER COLUMN grb_update TYPE timestamp with time zone;

ALTER TABLE swift ALTER COLUMN swift_received TYPE timestamp with time zone;
ALTER TABLE swift ALTER COLUMN swift_time TYPE timestamp with time zone;

ALTER TABLE integral ALTER COLUMN integral_received TYPE timestamp with time zone;
ALTER TABLE integral ALTER COLUMN integral_time TYPE timestamp with time zone;

ALTER TABLE observations ALTER COLUMN obs_slew TYPE timestamp with time zone;
ALTER TABLE observations ALTER COLUMN obs_start TYPE timestamp with time zone;
ALTER TABLE observations ALTER COLUMN obs_end TYPE timestamp with time zone;

ALTER TABLE images ALTER COLUMN img_date TYPE timestamp with time zone;

ALTER TABLE queues_targets ALTER COLUMN time_start TYPE timestamp with time zone;
ALTER TABLE queues_targets ALTER COLUMN time_end TYPE timestamp with time zone;

ALTER TABLE message ALTER COLUMN message_time TYPE timestamp with time zone;

-- BB tables
ALTER TABLE observatory_schedules ALTER COLUMN created TYPE timestamp with time zone;
ALTER TABLE observatory_schedules ALTER COLUMN last_update TYPE timestamp with time zone;
ALTER TABLE observatory_schedules ALTER COLUMN sched_from TYPE timestamp with time zone;
ALTER TABLE observatory_schedules ALTER COLUMN sched_to TYPE timestamp with time zone;

ALTER TABLE observatory_observations ALTER COLUMN obs_slew TYPE timestamp with time zone;
ALTER TABLE observatory_observations ALTER COLUMN obs_start TYPE timestamp with time zone;
ALTER TABLE observatory_observations ALTER COLUMN obs_end TYPE timestamp with time zone;
