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

-- insert flats - those one are from http://www.ing.iac.es/~meteodat/blanks.htm
COPY targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment, tar_enabled, tar_priority, tar_bonus, tar_bonus_time) FROM stdin;
20	f	MAblank1	15	0.116666666666667	group of obj. at 2', faint st. at 7',10'	t	1	0	\N
21	f	CAblank1 	26.9	2.33416666666667	faint st. at 10',15'	t	1	0	\N
22	f	MAblank2 	33.75	-10	very blank, gal. at 10'	t	1	0	\N
23	f	AAOblank1 	42.5166666666667	-19.5636111111111	bright st. pair at 6', st. at 13',15'	t	1	0	\N
24	f	wfcblank3 	44.5	0.1	faint st. 1.5', st. at 12',15'	t	1	0	\N
25	f	MAblank3 	62.5	3.83333333333333	faint st at ~3'	t	1	0	\N
26	f	BLANK1	67.4375	54.26	centered on circular dark cloud 12' diameter	t	1	0	\N
27	f	MAblank4 	88.7	2.58333333333333	faint st. at 6',8',14'	t	1	0	\N
28	f	MAblank5 	89	4.15	very blank, st. at 12',16'	t	1	0	\N
29	f	MAblank6 	106.75	1.33333333333333	faint st. at 12'	t	1	0	\N
30	f	wfsblank2 	120.25	50	faint st. at 3', 8', 10'	t	1	0	\N
31	f	AAOblank2 	138	-7.84638888888889	faint st. at 6', st. at 13'	t	1	0	\N
32	f	CAblank2 	138.454166666667	46.2327777777778	very blank, faint st. at 13'	t	1	0	\N
33	f	AAOblank3 	151.745833333333	-2.56111111111111	faint gal. at 4', st. at 10'-15'	t	1	0	\N
34	f	MAblank7 	158.75	3.03333333333333	st. at 4', 16'	t	1	0	\N
35	f	MAblank8	164.791666666667	3.5	faint st. at 5', st. at 14'	t	1	0	\N
36	f	AAOblank4 	187.179166666667	-6.91777777777778	faint st. at 8',14',st. at 10'	t	1	0	\N
37	f	AAOblank5 	187.6625	-8.05777777777778	big (3') bright gal. at 7', st. at 2',9',13'	t	1	0	\N
38	f	AAOblank6 	194.3875	-2.38777777777778	faint st. at 3',9'	t	1	0	\N
39	f	BLANK2	196.733333333333	29.58	faint gal. at 9', st. pair at 12'	t	1	0	\N
40	f	CAblank3 	204.075	62.2363888888889	st. at 10',14'	t	1	0	\N
41	f	CAblank4 	206.925	5.62666666666667	faint st. at 10', st. at 15'	t	1	0	\N
42	f	MAblank9 	219.041666666667	4.66666666666667	faint st. at 3',st. at 7',13',14'	t	1	0	\N
43	f	wfcblank1 	225.5	29.9166666666667	faint st. at 1',6',13',14',16'	t	1	0	\N
44	f	AAOblank7 	228.95	0.713888888888889	faint st. at 5', 12'	t	1	0	\N
45	f	CAblank5 	246.1375	55.7330555555556	st. at 10'-15'	t	1	0	\N
46	f	BLANK3	252.683333333333	-15.38	dark band NE-SW 10' wide, st. at 10'	t	1	0	\N
47	f	AAOblank8 	253.1375	-15.4325	very blank, faint st. at 16'	t	1	0	\N
48	f	wfsblank1 	255	41	faint st. at 2.5',8',st. at 12',13'	t	1	0	\N
49	f	CAblank6 	269.933333333333	66.3552777777778	faint st. at 11', st. at 12'	t	1	0	\N
50	f	MAblank10 	271.5	0.5	faint st. at 6',9'	t	1	0	\N
51	f	BLANK4 	290.370833333333	12.4636111111111	dark band N-S 10' wide, faint st. at 8'	t	1	0	\N
52	f	MAblank11 	299.75	2.33333333333333	faint st. at 4',7',9'	t	1	0	\N
53	f	BLANK5 	322.391666666667	-8.64166666666667	st. at 7',10', faint st. at 13',15'	t	1	0	\N
54	f	wfcblank2 	344.5	0.0833333333333333	faint st. at 4',st. at 7',9',12'	t	1	0	\N
55	f	CAblank7 	348.95	11.4422222222222	st. at 11',17'	t	1	0	\N
56	f	AAOblank9 	357.083333333333	0.955833333333333	st. at 6',13',18'	t	1	0	\N
57	f	BLANK6	359.166666666667	59.75	dark band ESE-WNW 6' wide, st. at 4',9',14'	t	1	0	\N
\.
