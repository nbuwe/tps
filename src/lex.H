/* Lexical scanner entries */

EXTERNC Tps_Status AC(Tps_get_token,(Tps_Stream_String& tokenbuf,
				Tps_Stream* f,
				Tps_Value* vp,
				int escapes));

EXTERNC Tps_Status AC(Tps_get_atom,(Tps_Stream_String&  tokenbuf,
				Tps_Stream* f,
				Tps_Value* vp,
				int escapes));
