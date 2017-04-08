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

EXTERNC void debugobject1(Tps_Value obj, Tps_Stream* accum, long deep, long ic);
EXTERNC const char* debugobject(Tps_Value);
EXTERNC const char* debugdict(const Tps_Dict*);
EXTERNC const char* debugdictall(const Tps_Dict*);
EXTERNC const char* debugstack0(Tps_Interp*);
EXTERNC const char* debugstack(Tps_Interp*);
EXTERNC const char* debugdstack0(Tps_Interp*);
EXTERNC const char* debugdstack(Tps_Interp*);
EXTERNC const char* debugdstacks(Tps_Interp*);
EXTERNC const char* debugexec0(Tps_Interp*, long depth);
EXTERNC const char* debugexec(Tps_Interp*);
