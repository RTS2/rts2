-- load tables so dummy environment can be run, with executor writing images to the database

insert into mounts values ('T0', 10, 15, 1000, 'Dummy mount');

insert into cameras values ('C0', 'dummy camera');

-- filters

COPY filters (filter_id, standart_name, description, medium_wl, width) FROM stdin;
AA	Filter A	Filter A	800	10
B	Filter B	Filter B	810	20
C	Filter C	Filter C	830	5
D	Filter D	Filter D	750	15
E	Filter E	Filter E	720	25
\.
