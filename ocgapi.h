/*
 * interface.h
 *
 *  Created on: 2010-4-28
 *      Author: Argon
 */

#ifndef OCGAPI_H_
#define OCGAPI_H_

#include "common.h"
#include <vector>  
#include "card.h"
#include "buffer.h"

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#ifdef _WIN32
#define OCGCORE_API EXTERN_C __declspec(dllexport)
#else
#define OCGCORE_API EXTERN_C __attribute__ ((visibility ("default")))
#endif

#define SEED_COUNT	8

#define LEN_FAIL	0
#define LEN_EMPTY	4
#define LEN_HEADER	8
#define TEMP_CARD_ID	0

struct card_data;
struct lua_State;

typedef byte* (*script_reader)(const char* script_name, int* len);
typedef uint32_t (*card_reader)(uint32_t code, card_data* data);
typedef uint32_t (*card_reader_random)(std::vector<int>& data,uint32_t count, uint32_t type,bool is_include);
typedef uint32_t (*message_handler)(intptr_t pduel, uint32_t msg_type);

OCGCORE_API void set_script_reader(script_reader f);
OCGCORE_API void set_card_reader(card_reader f);
OCGCORE_API void set_card_reader_random(card_reader_random f);
OCGCORE_API void set_message_handler(message_handler f);
void card_data_copy(card* new_card, card* from_card, uint32_t playerid,uint32_t target_playerid,std::map<int, effect*> effects_map);
void effect_data_copy(effect* new_effect, effect* peffect, uint32_t playerid, uint32_t target_playerid);
card* find_card(duel*pduel, card* pcard, uint32_t playerid);

byte* read_script(const char* script_name, int* len);
uint32_t read_card(uint32_t code, card_data* data);
uint32_t read_card_random(std::vector<int>& data,uint32_t count, uint32_t type,bool is_include);
uint32_t handle_message(void* pduel, uint32_t message_type);
void copy_field_data(intptr_t source_pduel, intptr_t spduel, uint32_t location, uint32_t playerid,uint32_t target_playerid);
void sync_used_xyz(card* pcard ,card* mat,int32_t playerid);
// static int rebind_lua_function_between_states(lua_State* srcL, int srcref, lua_State* dstL);

OCGCORE_API intptr_t create_duel(uint_fast32_t seed);
OCGCORE_API intptr_t create_duel_v2(uint32_t seed_sequence[]);
OCGCORE_API intptr_t create_duel_v3();
OCGCORE_API void change_lua_duel(intptr_t pduel);
OCGCORE_API int32_t force_to_battle(intptr_t pduel);
OCGCORE_API void set_player_state(intptr_t pduel, intptr_t player, intptr_t player2);
OCGCORE_API void start_duel(intptr_t pduel, uint32_t options);
OCGCORE_API void end_duel(intptr_t pduel);
OCGCORE_API void set_player_info(intptr_t pduel, int32_t playerid, int32_t lp, int32_t startcount, int32_t drawcount);
OCGCORE_API void copy_duel_data(intptr_t source_pduel, intptr_t spduel1,intptr_t spduel2,uint32_t opt);
OCGCORE_API void get_log_message(intptr_t pduel, char* buf);
OCGCORE_API int32_t get_message(intptr_t pduel, byte* buf);
OCGCORE_API uint32_t process(intptr_t pduel);
OCGCORE_API void new_card(intptr_t pduel, uint32_t code, uint8_t owner, uint8_t playerid, uint8_t location, uint8_t sequence, uint8_t position);
OCGCORE_API void new_tag_card(intptr_t pduel, uint32_t code, uint8_t owner, uint8_t location);
OCGCORE_API int32_t query_card(intptr_t pduel, uint8_t playerid, uint8_t location, uint8_t sequence, int32_t query_flag, byte* buf, int32_t use_cache);
OCGCORE_API int32_t query_field_count(intptr_t pduel, uint8_t playerid, uint8_t location);
OCGCORE_API int32_t query_field_card(intptr_t pduel, uint8_t playerid, uint8_t location, uint32_t query_flag, byte* buf, int32_t use_cache);
OCGCORE_API int32_t query_field_info(intptr_t pduel, byte* buf);
OCGCORE_API void reload_field_info(intptr_t pduel);
OCGCORE_API void set_responsei(intptr_t pduel, int32_t value);
OCGCORE_API void set_responseb(intptr_t pduel, byte* buf);
OCGCORE_API int32_t preload_script(intptr_t pduel, const char* script_name);
OCGCORE_API byte* default_script_reader(const char* script_name, int* len);

#endif /* OCGAPI_H_ */
