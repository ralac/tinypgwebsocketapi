CREATE DATABASE api_test;

CREATE TABLE public.users
(
  id serial NOT NULL,
  username text,
  password text,
  CONSTRAINT users_pkey PRIMARY KEY (id)
);

CREATE TABLE public.authorazations
(
  id uuid NOT NULL,
  user_id integer NOT NULL,
  session_id text NOT NULL,
  CONSTRAINT authorazations_pkey PRIMARY KEY (id)
);

ALTER TABLE public.authorazations
  ADD CONSTRAINT authorization_user_fk FOREIGN KEY (user_id)
      REFERENCES public.users (id) MATCH SIMPLE
      ON UPDATE NO ACTION ON DELETE NO ACTION;

CREATE TABLE public.clients
(
  id serial NOT NULL,
  authorazation_id integer NOT NULL,
  cgi_host text,
  cgi_pid bigint,
  last_message datetime,
  CONSTRAINT clients_pkey PRIMARY KEY (id)
);

ALTER TABLE public.clients
  ADD CONSTRAINT client_authorization_fk FOREIGN KEY (authorazation_id)
      REFERENCES public.authorazations (id) MATCH SIMPLE
      ON UPDATE NO ACTION ON DELETE NO ACTION;

CREATE SCHEMA api;

CREATE TYPE api.dict AS (
    key   text,
    value text
);
