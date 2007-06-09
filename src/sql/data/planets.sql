-- importing planets is possible from version 0.7 up

COPY targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment, tar_enabled, tar_priority, tar_bonus, tar_bonus_time) FROM stdin;
190	L	Sun	0	0	Sun (planet)	t	0	0	\N
191	L	Mercury	0	0	Mercury (planet)	t	0	0	\N
192	L	Venus	0	0	Venus (planet)	t	0	0	\N
193	L	Moon	0	0	Moon (Earth)	t	0	0	\N
194	L	Mars	0	0	Mars (planet)	t	0	0	\N
195	L	Jupiter	0	0	Jupiter (planet)	t	0	0	\N
196	L	Saturn	0	0	Saturn (planet)	t	0	0	\N
197	L	Uranus	0	0	Uran (planet)	t	0	0	\N
198	L	Neptune	0	0	Neptun (planet)	t	0	0	\N
199	L	Pluto	0	0	Pluto (planet)	t	0	0	\N
\.
