COPY targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment, tar_enabled, tar_priority, tar_bonus, tar_bonus_time) FROM stdin;
200	S	Test 0	0	20	Test 2	t	10	0	\N
201	S	Test 1	20	20	Test 2	t	10	0	\N	
11      I       Integral FOV    0       0       Integral Field of View, based on GCN    t       0       0       \N
1       d       Dark frames     0       0       Used to produce darks   t       0       0       \N
2       f       Flat frames     0       0       Used to produce flatfields      t       0       0       \N
10      W       Swift FOV       0       0       Swift Field of View, based on GCN       t       0       0       \N
\.

