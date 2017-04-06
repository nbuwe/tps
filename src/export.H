/*
 * Copyright (c) 1993 Dennis Heimbigner
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
