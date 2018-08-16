/*
select pg_get_functiondef( 'api.login'::regproc );
*/
CREATE OR REPLACE FUNCTION api.login(
    i_session_id text,
    i_username text,
    i_password text)
  RETURNS TABLE(o_uuid text) AS
$BODY$
DECLARE
  w_user_id text;
  w_uuid uuid;
BEGIN
  w_user_id := NULL;
  SELECT id INTO w_user_id
    FROM users
    WHERE username = i_username
      AND password = md5(i_password);
  IF FOUND THEN
    BEGIN
      w_uuid = uuid_generate_v1();
      INSERT INTO public.authorazations(
        user_id,
        session_id
        id
      ) VALUES (
        w_user_id,
        i_session_id,
        w_uuid
      );
      o_uuid = w_uuid::text
      RETURN NEXT;
    END;
  ELSE
    RETURN;
  END IF;
END;
$BODY$
  LANGUAGE plpgsql VOLATILE
