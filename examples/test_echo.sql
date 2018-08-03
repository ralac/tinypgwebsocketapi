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
  id serial NOT NULL,
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

CREATE OR REPLACE FUNCTION api.handler(
    IN i_cgi_pid bigint,
    IN i_cgi_environment api.dict[],
    IN i_request_body text,
    IN i_websocket_message text)
  RETURNS TABLE(client_id integer, response_headers text[], response_body text, eventsource boolean, websocket boolean, trace text) AS
$BODY$
DECLARE
  w_session_id      text;
  w_cgi_service     text;
  w_method          text;
  w_cookies         api.dict[];
  w_query_string    api.dict[];
  w_fields          api.dict[];
  --
  w_item            text;
  w_items           text[];
  w_dict            api.dict;
  w_env_rec         record;
  w_env_cur CURSOR FOR
    SELECT
        trim((t.dict_col).key)    as key,
        trim((t.dict_col).value)  as value
      FROM (SELECT unnest(i_cgi_environment) dict_col) AS t
      ;
  w_cookies_rec     record;
  w_cookies_cur CURSOR FOR
    SELECT
        trim((t.dict_col).key)    as key,
        trim((t.dict_col).value)  as value
      FROM (SELECT unnest(w_cookies) dict_col) AS t
      ;
BEGIN
  eventsource := false;
  websocket := false;
  w_method := NULL;
  FOR w_env_rec IN w_env_cur LOOP
    IF w_env_rec.key = 'REQUEST_METHOD' THEN
      w_method := w_env_rec.value;
    ELSIF w_env_rec.key = 'QUERY_STRING' THEN
      FOR w_item IN SELECT unnest(regexp_split_to_array(w_env_rec.value, '&')) LOOP
        w_items := regexp_split_to_array(w_item,'=');
        IF trim(w_items[1]) != '' THEN
          w_dict.key   := trim(w_items[1]);
          w_dict.value := trim(w_items[2]);
          w_query_string := array_append(w_query_string, w_dict);
        END IF;
      END LOOP;
    ELSIF w_env_rec.key = 'HTTP_ACCEPT' THEN
      -- ** Event Source
      IF w_env_rec.value='text/event-stream' THEN
        eventsource := true;
      END IF;
    ELSIF w_env_rec.key = 'HTTP_CONNECTION' THEN
      -- ** WebSocket
      IF w_env_rec.value='Upgrade' THEN
        websocket := true;
      END IF;
    ELSIF w_env_rec.key = 'HTTP_COOKIE' THEN
      FOR w_item IN SELECT unnest(regexp_split_to_array(w_env_rec.value, ';')) LOOP
        w_items := regexp_split_to_array(w_item,'=');
        IF trim(w_items[1]) != '' THEN
          w_dict.key   := trim(w_items[1]);
          w_dict.value := trim(w_items[2]);
          w_cookies := array_append(w_cookies, w_dict);
        END IF;
      END LOOP;
    ELSIF w_env_rec.key = 'CGI_SERVICE' THEN
      w_cgi_service := w_env_rec.value;
    END IF;
  END LOOP;
  FOR w_cookies_var IN w_cookies_cur LOOP
    IF upper(w_cookies_var.key) = 'SESSION_ID' THEN
      w_session_id := w_cookies_var.value;
    END IF;
  END LOOP;
  IF w_method != 'GET' THEN
    eventsource := false;
    websocket := false;
  END IF;
  --
  trace := E'\n\t';
  --IN i_cgi_pid bigint,
  trace := trace || coalesce('i_cgi_pid: ' || i_cgi_pid || E'\n\t', '');
  --IN i_cgi_environment api.dict[],
  trace := trace || coalesce('i_cgi_environment: ' || E'\n' || array_to_json(i_cgi_environment, true) || E'\n\t', '');
  --w_session_id      text;
  trace := trace || coalesce('w_session_id: ' || w_session_id || E'\n\t', '');
  --w_cgi_service     text;
  trace := trace || coalesce('w_cgi_service: ' || w_cgi_service || E'\n\t', '');
  --w_method          text;
  trace := trace || coalesce('w_method: ' || w_method || E'\n\t', '');
  --w_cookies         api.dict[];
  trace := trace || coalesce('w_cookies: ' || E'\n' || array_to_json(w_cookies, true) || E'\n\t', '');
  --w_query_string    api.dict[];
  trace := trace || coalesce('w_query_string: ' || E'\n' || array_to_json(w_query_string, true) || E'\n\t', '');
  --w_fields          api.dict[];
  trace := trace || coalesce('w_fields: ' || E'\n' || array_to_json(w_fields, true) || E'\n\t', '');
  --
  RETURN NEXT;
  RETURN;
END
;$BODY$
  LANGUAGE plpgsql

CREATE OR REPLACE FUNCTION api.login(
    i_session_id text,
    i_username text,
    i_password text)
  RETURNS boolean AS
$BODY$
DECLARE
  w_user_id text;
BEGIN
  w_user_id := NULL;
  SELECT id INTO w_user_id
    FROM users
    WHERE username = i_username
      AND password = md5(i_password);
  IF FOUND THEN
    BEGIN
      INSERT INTO public.authorazations(
        user_id,
      session_id
      ) VALUES (
        w_user_id, 
        i_session_id
      );
      RETURN true;
    END;
  ELSE
    RETURN false;
  END IF;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
