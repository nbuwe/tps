EXTERNC Tps_Status AC(Tps_bind1,(Tps_Value* ds, long dlen, Tps_Value* vp));
EXTERNC Tps_Status AC(Tps_cvts1,(Tps_Stream& strm, Tps_Value, boolean deep, long addrctr));
EXTERNC Tps_Status AC(Tps_cvts1_dict_deep,(Tps_Stream& accum, Tps_Dict* dict, boolean deep));
EXTERNC int AC(Tps_compare,(Tps_Value opl, Tps_Value opr));
EXTERNC Tps_Status AC(Tps_string_or_name,(Tps_Value v, char** sp, long* lenp));

EXTERNC Tps_Status AC(Tps_dictstack_lookup,(Tps_Value* ds, long dlen,
					Tps_Value key, long* where,
					Tps_Dictpair** pairp));
EXTERNC Tps_Status AC(Tps_dictstack_define,(Tps_Value* ds, long dlen,
					Tps_Dictpair& pair, long* where));
