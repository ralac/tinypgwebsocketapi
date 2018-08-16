/*
select pg_get_functiondef( 'api.handler'::regproc );
*/
CREATE OR REPLACE FUNCTION api.handler(i_cgi_pid bigint, i_cgi_environment api.dict[], i_request_body bytea, i_websocket_message bytea, i_trace boolean)
 RETURNS TABLE(client_id integer, response_status text, response_headers bytea[], response_body bytea, eventsource boolean, websocket boolean, trace text)
 LANGUAGE plpgsql
AS $function$

DECLARE
  w_session_id      text;
  w_cgi_service     text;
  w_method          text;
  w_cookies         api.dict[];
  w_query_string    api.dict[];
  w_fields          api.dict[];
  --
  w_server_name     				text;
  w_path_info							  text;
  w_script_name             text;
  w_http_origin     				text;
  w_http_sec_websocket_key	text;
  w_sec_websocket_accept		text;
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
  websocket 	:= false;
  w_method 		:= NULL;
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
    ELSIF w_env_rec.key = 'SERVER_NAME' THEN
				w_server_name := trim(w_env_rec.value);
    ELSIF w_env_rec.key = 'HTTP_ORIGIN' THEN
				w_http_origin := trim(w_env_rec.value);
    ELSIF w_env_rec.key = 'HTTP_SEC_WEBSOCKET_KEY' THEN
				w_http_sec_websocket_key := trim(w_env_rec.value);
    ELSIF w_env_rec.key = 'PATH_INFO' THEN
				w_path_info := trim(w_env_rec.value);
    ELSIF w_env_rec.key = 'SCRIPT_NAME' THEN
				w_script_name := trim(w_env_rec.value);
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
  IF w_method = 'GET' THEN
		IF websocket THEN
			response_status := 'HTTP/1.1 101 Switching Protocols';
			response_headers := array_append(response_headers, 'Upgrade: websocket'::bytea);
			response_headers := array_append(response_headers, 'Connection: Upgrade'::bytea);
			w_sec_websocket_accept := encode(
        public.digest(
          w_http_sec_websocket_key || '258EAFA5-E914-47DA-95CA-C5AB0DC85B11',
          'sha1'::text
      ), 'base64');
			response_headers := array_append(response_headers, ('Sec-WebSocket-Accept: ' || w_sec_websocket_accept)::bytea);
			response_headers := array_append(response_headers, ('Server: ' || w_server_name)::bytea);
			response_headers := array_append(response_headers, ('Access-Control-Allow-Origin: ' || w_http_origin)::bytea);
			response_headers := array_append(response_headers, ('Access-Control-Allow-Credentials: true')::bytea);
		END IF;
  ELSE                                     
    eventsource := false;
    websocket := false;
		IF w_method = 'POST' THEN
      IF COALESCE(w_path_info, w_script_name) LIKE '%/login' THEN
        response_status := 'HTTP/1.1 200 OK';
        response_headers := array_append(response_headers, ('Content-Type: text/text')::bytea);
        response_body := 'YAYA'::bytea;                                       
		  END IF;
	  END IF;
  END IF;
  --
  IF i_trace THEN
	  --
    trace := E'\n\t';
    --IN i_cgi_pid bigint,
    trace := trace || coalesce('i_cgi_pid: ' || i_cgi_pid || E'\n\t', '');
    --IN i_cgi_environment api.dict[],
    trace := trace || coalesce('i_cgi_environment: ' || E'\n' || array_to_json(i_cgi_environment, true) || E'\n\t', '');
    --IN i_request_body bytea,
    trace := trace || coalesce('i_request_body: ' || E'\n' || encode(i_request_body, 'escape') || E'\n\t', '');
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
    --OUT response_body bytea,
    trace := trace || coalesce('response_body: ' || E'\n' || encode(response_body, 'escape') || E'\n\t', '');
	  --
  END IF;
  --
  -- PERFORM pg_sleep(60);
  --
  RETURN NEXT;
  RETURN;
END;

$function$
