/**************************************************/

#define TPS_ENCODEVERSION "1.0"

/**************************************************/
EXTERNC Tps_Status AC(Tps_encode,(Tps_Interp*,Tps_Value,Tps_Stream*));

EXTERNC Tps_Status AC(Tps_encode_container,(Tps_Interp*,Tps_Value,Tps_Stream*));
EXTERNC Tps_Status AC(Tps_export,(Tps_Interp*,Tps_Stream*));

EXTERNC Tps_Status AC(Tps_decode,(Tps_Interp*,Tps_Value&,Tps_Stream*,Tps_Array&,Tps_Stream_String&));

EXTERNC Tps_Status AC(Tps_decode_section,(Tps_Interp*,Tps_Stream*,Tps_Array&,Tps_Stream_String&));
EXTERNC Tps_Status AC(Tps_decode_container,(Tps_Interp*,Tps_Value&,Tps_Stream*,Tps_Array&,Tps_Stream_String&));
EXTERNC Tps_Status AC(Tps_import,(Tps_Interp*,Tps_Stream*,boolean));

EXTERNC Tps_Status AC(Tps_export_exec,(Tps_Interp*, long&, Tps_Exec_Pass));
EXTERNC Tps_Status AC(Tps_import_exec,(Tps_Interp*, long, Tps_Exec_Pass));


