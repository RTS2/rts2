COPY types FROM stdin;
O	Opportunity targets
G	Gamma Ray Burst
S	Sky survey (new one)
E	Elliptical targets
P	Galatic Plate Scan
H	HETE FOV
t	technical observation
T	terrestial (fixed ra+dec) target
W	Swift FOV
I	Integral FOV
d	Dark target
f	Flat target
o	fOcusing target
m	Modeling (have optional parameters in target_model)
\.

COPY epoch FROM stdin;
1	'1-1-1970'	'1-1-2032'
\.

COPY medias FROM stdin;
0	'/images'	t
\.

-- standart targets (tar_id < 100)

COPY targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment, tar_enabled, tar_priority, tar_bonus, tar_bonus_time) FROM stdin;
1	d	Dark frames	0	0	Used to produce darks	t	0	0	\N
2	f	Flat frames	0	0	Used to produce flatfields	t	0	0	\N
3	o	Focusing frames	0	0	Used to focusc cameras	t	0	0	\N
4	m	Default model	0	0	Used to model get enought pictures for mount model	t	0	0	\N
10	W	Swift FOV	0	0	Swift Field of View, based on GCN	t	0	0	\N
11	I	Integral FOV	0	0	Integral Field of View, based on GCN	t	0	0	\N
\.

COPY target_model (tar_id, alt_start, alt_stop, alt_step, az_start, az_stop, az_step, noise, step) FROM stdin;
4	40	80	20	0	360	20	4	0
\.
