#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "game.h"

#include "bmodels.h"

#include "mod_none.h"

#include "bot_include.h"

#include <algorithm>

namespace sv {

void CMod_None::CheckMapConditions()
{
	CBaseModDefault::CheckMapConditions();
	m_mapBombZones.clear();
	if (m_bMapHasBombZone)
	{
		CBaseEntity *pEntity = nullptr;

		while ((pEntity = UTIL_FindEntityByClassname(pEntity, "func_bomb_target")))
		{
			m_mapBombZones.emplace_back(pEntity, VecBModelOrigin(pEntity->pev));
		}
		// pEntity = nullptr;
		while ((pEntity = UTIL_FindEntityByClassname(pEntity, "info_bomb_target")))
		{
			m_mapBombZones.emplace_back(pEntity, pEntity->pev->origin);
		}

		using EVpair_t = decltype(m_mapBombZones)::value_type;
		std::sort(m_mapBombZones.begin(), m_mapBombZones.end(), [](const EVpair_t &a, const EVpair_t &b)
		{
			//auto get_bombsite_index = [](const Vector &vec) {
			//	const CCSBotManager::Zone* zone = TheCSBots()->GetZone(&vec);
			//	return zone ? zone->m_index : 0;
			//};
			return a.second.x > b.second.x;
		});
	}
}

void CMod_None::UpdateGameMode(CBasePlayer *pPlayer)
{
	MESSAGE_BEGIN(MSG_ONE, gmsgGameMode, NULL, pPlayer->edict());
	WRITE_BYTE(MOD_NONE);
	WRITE_BYTE(0); // Reserved. (weapon restriction? )
	WRITE_BYTE(maxrounds.value); // MaxRound (mp_roundlimit)
	WRITE_BYTE(0); // Reserved. (MaxTime?)

	WRITE_BYTE(m_mapBombZones.size());
	for (const auto& EV : m_mapBombZones)
	{
		const Vector &pos(EV.second);
		WRITE_COORD(pos[0]);
		WRITE_COORD(pos[1]);
		WRITE_COORD(pos[2]);
	}

	MESSAGE_END();
}

}

