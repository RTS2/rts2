SELECT 
	observations.tar_id, observations.obs_id, observations.obs_start, observations.obs_duration, targets.tar_ra, targets.tar_dec, targets.tar_name, targets.type_id, observations_images.img_count
FROM  targets, observations, observations_images
WHERE targets.tar_id = observations.tar_id AND observations.obs_id = observations_images.obs_id
AND obs_start > now () - interval '1 day'
ORDER BY obs_start DESC;
