DROP VIEW targets_images;
DROP VIEW targets_noimages;
DROP VIEW targets_imgcount;

CREATE VIEW targets_noimages AS 
SELECT targets.tar_id, 0 AS img_count 
	FROM targets 
	WHERE (NOT (EXISTS 
		(SELECT * FROM images, observations 
			WHERE ((observations.tar_id = targets.tar_id) AND 
			(observations.obs_id = images.obs_id)))));

CREATE VIEW targets_imgcount AS
SELECT targets.tar_id, count(*) AS img_count
	FROM targets, observations, images 
	WHERE ((images.obs_id = observations.obs_id) AND 
		(observations.tar_id = targets.tar_id))
	GROUP BY targets.tar_id ORDER BY count(*) DESC;

CREATE VIEW targets_images AS 
SELECT tar_id, img_count 
	FROM targets_imgcount 
UNION SELECT tar_id, img_count 
	FROM targets_noimages;

CREATE VIEW targets_altaz AS
SELECT targets.*, 
	obj_alt (tar_ra, tar_dec, 240000, mount_long, mount_lat)
	AS alt, 
	obj_az (tar_ra, tar_dec, 240000, mount_long, mount_lat) 
	AS az
	FROM mounts, targets
	WHERE mount_name = "T1";
