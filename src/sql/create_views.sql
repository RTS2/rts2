DROP VIEW targets_images;
DROP VIEW targets_noimages;
DROP VIEW targets_imgcount;
DROP VIEW targets_enabled;
DROP VIEW observations_images;
DROP VIEW observations_imgcount;
DROP VIEW observations_noimages;
DROP VIEW observations_nights;
DROP VIEW images_nights;
DROP VIEW targets_counts;
DROP VIEW images_path;

CREATE VIEW targets_noimages AS 
SELECT targets.tar_id, 0 AS img_count 
	FROM targets 
	WHERE (NOT (EXISTS 
		(SELECT * FROM targets_imgcount
			WHERE targets_imgcount.tar_id =	targets.tar_id)));

CREATE VIEW targets_imgcount AS
SELECT targets.tar_id, count(*) AS img_count
	FROM targets, observations, images 
	WHERE images.obs_id = observations.obs_id AND 
		observations.tar_id = targets.tar_id
	GROUP BY targets.tar_id ORDER BY count(*) DESC;

CREATE VIEW targets_images AS 
SELECT tar_id, img_count 
	FROM targets_imgcount 
UNION SELECT tar_id, img_count 
	FROM targets_noimages;

CREATE VIEW observations_noimages AS 
SELECT observations.obs_id, 0 AS img_count 
	FROM observations 
	WHERE (NOT (EXISTS 
		(SELECT * FROM images 
			WHERE observations.obs_id = images.obs_id)));

CREATE VIEW observations_imgcount AS
SELECT observations.obs_id, count(*) AS img_count
	FROM observations, images 
	WHERE images.obs_id = observations.obs_id
	GROUP BY observations.obs_id ORDER BY count(*) DESC;

CREATE VIEW observations_images AS 
SELECT obs_id, img_count 
	FROM observations_imgcount 
UNION SELECT obs_id, img_count 
	FROM observations_noimages;

CREATE VIEW observations_nights AS
SELECT observations.*, date_part('day', (obs_start - interval '12:00')) AS obs_night, 
	date_part('month', (obs_start - interval '12:00')) AS obs_month, 
	date_part('year', (observations.obs_start - interval '12:00')) AS obs_year 
	FROM observations;

CREATE VIEW images_nights AS
SELECT images.*, date_part('day', (timestamptz(images.img_date) - interval '12:00')) AS img_night, 
	date_part('month', (timestamptz(images.img_date) - interval '12:00')) AS img_month, 
	date_part('year', (timestamptz(images.img_date) - interval '12:00')) AS img_year 
	FROM images;

CREATE VIEW targets_enabled AS
SELECT
        tar_id,
        type_id,
        tar_name,
        tar_ra,
        tar_dec,
        tar_comment
FROM
        targets
WHERE
        tar_enabled = true;

CREATE VIEW targets_counts AS
SELECT
        targets.tar_id,
	counts.*
FROM
        targets, counts, observations
WHERE
        targets.tar_id = observations.tar_id
    AND observations.obs_id = counts.obs_id
ORDER BY
	count_date DESC;

CREATE VIEW images_path AS
SELECT
	img_id,
	imgpath (med_id, epoch_id, mount_name, camera_name, images.obs_id, tar_id, img_date) as img_path,
	img_date,
	round(img_exposure/100.0,2) as img_exposure_sec,
	round(img_temperature/10.0,2) as img_temperature_deg,
	img_filter,
	imgrange (astrometry) as img_range,
	images.obs_id,
	observations.tar_id,
	camera_name
FROM
	images,
	observations
WHERE
	images.obs_id = observations.obs_id
ORDER BY
	img_id;
