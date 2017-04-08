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

EXTERNC Tps_Status Tps_bind1(Tps_Value* ds, long dlen, Tps_Value* vp);
EXTERNC Tps_Status Tps_cvts1(Tps_Stream& strm, Tps_Value, boolean deep, long addrctr);
EXTERNC Tps_Status Tps_cvts1_dict_deep(Tps_Stream& accum, const Tps_Dict* dict, boolean deep);
EXTERNC int Tps_compare(Tps_Value opl, Tps_Value opr);
EXTERNC Tps_Status Tps_string_or_name(Tps_Value v, const char** sp, long* lenp);

EXTERNC Tps_Status Tps_dictstack_lookup(Tps_Value* ds, long dlen,
					Tps_Value key, long* where,
					Tps_Dictpair** pairp);
EXTERNC Tps_Status Tps_dictstack_define(Tps_Value* ds, long dlen,
					Tps_Dictpair& pair, long* where);
