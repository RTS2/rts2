-- table for events, recorded by eventd
CREATE TABLE message (
	message_time	timestamp,
	message_oname	varchar(8),
	message_type	integer,
	message_string	varchar(200)
);

CREATE INDEX message_time ON message (message_time);

CREATE INDEX message_oname ON message (message_oname);

CREATE INDEX message_type ON message (message_type);
