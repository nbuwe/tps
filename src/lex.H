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

/* Lexical scanner entries */

EXTERNC Tps_Status Tps_get_token(Tps_Stream_String& tokenbuf,
				Tps_Stream* f,
				Tps_Value* vp,
				int escapes);

EXTERNC Tps_Status Tps_get_atom(Tps_Stream_String&  tokenbuf,
				Tps_Stream* f,
				Tps_Value* vp,
				int escapes);
