COPY users (usr_login, usr_passwd, usr_tmp, usr_email, usr_id) FROM stdin;
petr	test	1	petr@example.com	1
alberto	test	2	alberto@example.com	2
mates	test	3	mates@example.com	3
\.

COPY targets_users (tar_id, usr_id, event_mask) FROM stdin;
100	1	3
100	2	1
100	3	2
\.
