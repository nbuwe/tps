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

struct Tps_Operator_Defines {
	const char*	_name;
	const char*	_body;
}; 

/* safe operators defined in system dict */
extern const struct Tps_Operator_Defines textdefs[];

/* unsafe operators defined in system dict */
extern const struct Tps_Operator_Defines utextdefs[];

#if OO
/* OO operators defined in system dict */
extern const struct Tps_Operator_Defines ootextdefs[];
#endif
