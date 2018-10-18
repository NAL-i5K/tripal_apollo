CREATE TABLE users (
  user_id serial,
  username text,
  password text
);

CREATE TABLE tracks (
  track_id serial,
  track_name text
);

CREATE TABLE permissions(
  track_id int,
  user_id int,
  permission text
);
