create table objects (
	obj_id         integer PRIMARY KEY,
	obj_ra         float8 NOT NULL,
	obj_dec        float8 NOT NULL,
	obj_gsc        varchar(20)
);

CREATE INDEX objects_ra on objects (obj_ra);
CREATE INDEX objects_dec on objects (obj_dec);

create table standards (
	obj_id         integer REFERENCES objects(obj_id),
	std_filter     integer REFERENCES filters(filter_id),
	std_mag        float NOT NULL,
	std_quality    integer
);

create table measurements (
	obj_id         integer REFERENCES objects(obj_id),
	obs_id         integer NOT NULL,
	img_id         integer NOT NULL,
	meas_airmass   float NOT NULL,
	meas_x         float NOT NULL,
	meas_x_err     float,
	meas_y         float NOT NULL,
	meas_y_err     float,
	meas_ell       float,
	meas_flux      float NOT NULL,
	meas_flux_err  float,
	meas_mag       float NOT NULL,
	meas_mag_err   float
);

alter table measurements
	add constraint foreign_measurements
	foreign key (obs_id,img_id) REFERENCES images(obs_id,img_id);

create sequence objects_obj_id;
