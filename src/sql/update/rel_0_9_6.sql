ALTER TABLE queues ADD COLUMN check_target_length boolean NOT NULL default true;
ALTER TABLE queues_targets ADD COLUMN repeat_n integer NOT NULL default -1;
ALTER TABLE queues_targets ADD COLUMN repeat_separation float;
