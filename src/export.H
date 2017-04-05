/**************************************************/

#define TPS_ENCODEVERSION "1.0"

/**************************************************/
EXTERNC Tps_Status Tps_encode(Tps_Interp*, Tps_Value, Tps_Stream*);

EXTERNC Tps_Status Tps_encode_container(Tps_Interp*, Tps_Value, Tps_Stream*);
EXTERNC Tps_Status Tps_export(Tps_Interp*, Tps_Stream*);

EXTERNC Tps_Status Tps_decode(Tps_Interp*, Tps_Value&, Tps_Stream*, Tps_Array&, Tps_Stream_String&);

EXTERNC Tps_Status Tps_decode_section(Tps_Interp*, Tps_Stream*, Tps_Array&, Tps_Stream_String&);
EXTERNC Tps_Status Tps_decode_container(Tps_Interp*, Tps_Value&, Tps_Stream*, Tps_Array&, Tps_Stream_String&);
EXTERNC Tps_Status Tps_import(Tps_Interp*, Tps_Stream*, boolean);

EXTERNC Tps_Status Tps_export_exec(Tps_Interp*, long&, Tps_Exec_Pass);
EXTERNC Tps_Status Tps_import_exec(Tps_Interp*, long, Tps_Exec_Pass);
