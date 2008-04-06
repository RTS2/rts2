
CREATE SEQUENCE user_id;

ALTER TABLE users ADD CONSTRAINT users_login UNIQUE (usr_login);
