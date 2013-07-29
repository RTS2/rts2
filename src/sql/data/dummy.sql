-- load tables so dummy environment can be run, with executor writing images to the database

insert into mounts values ('T0', 10, 15, 1000, 'Dummy mount');

insert into cameras values ('C0', 'dummy camera');

-- filters

COPY filters (filter_id, standart_name, medium_wl, width) FROM stdin;
0	AA	800	10
1	B	810	20
2	C	830	5
3	D	750	15
4	E	720	25
\.
