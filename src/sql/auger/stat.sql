-- Prints statistics of Auger observations.
--
-- You probably want to run this as
--
-- psql -A -f stat.sql -o test.dat -F, stars
--
-- Petr Kubanek, Institute of Physics <kubanek@fzu.cz>

select auger.auger_t3id,eye,run,event,energy,x_max,dghchi2improv,auger_observation.obs_id,auger.auger_date,obs_slew,obs_start,obs_end,cut,obs_images(auger_observation.obs_id,'WF') as wf,obs_images_astr(auger_observation.obs_id,'WF') as wf_astro,obs_images(auger_observation.obs_id,'NF') as nf,obs_images_astr(auger_observation.obs_id,'NF') as nf_astr,obs_images(auger_observation.obs_id,'NF2') as nf2,obs_images_astr(auger_observation.obs_id,'NF2') as nf2_astr from auger,auger_observation,observations where auger.auger_t3id = auger_observation.auger_t3id and auger_observation.obs_id = observations.obs_id order by auger.auger_t3id asc;
