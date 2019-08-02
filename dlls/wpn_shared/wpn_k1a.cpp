/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "wpn_k1a.h"

#ifdef CLIENT_DLL
namespace cl {
#else
namespace sv {
#endif

enum k1a_e
{
	K1A_IDLE1,
	K1A_RELOAD,
	K1A_DRAW,
	K1A_SHOOT1,
	K1A_SHOOT2,
	K1A_SHOOT3
};

LINK_ENTITY_TO_CLASS(weapon_k1a, CK1a)

const int K1A_MAXCLIP = 30;

void CK1a::Spawn(void)
{
	pev->classname = MAKE_STRING("weapon_k1a");

	Precache();
	m_iId = WEAPON_GALIL;
	SET_MODEL(ENT(pev), "models/w_k1a.mdl");

	m_iDefaultAmmo = K1A_MAXCLIP;

	FallInit();
}

void CK1a::Precache(void)
{
	PRECACHE_MODEL("models/p_k1a.mdl");
	PRECACHE_MODEL("models/v_k1a.mdl");
	PRECACHE_MODEL("models/w_k1a.mdl");

	PRECACHE_SOUND("weapons/k1a-1.wav");

	PRECACHE_SOUND("weapons/k1a_clipout.wav");
	PRECACHE_SOUND("weapons/k1a_clipin.wav");
	PRECACHE_SOUND("weapons/k1a_boltpull.wav");

	PRECACHE_SOUND("weapons/k1a_foley1.wav");

	m_iShell = PRECACHE_MODEL("models/rshell.mdl");
	m_usFireK1a = PRECACHE_EVENT(1, "events/k1a.sc");
}

int CK1a::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556Nato";
	p->iMaxAmmo1 = MAX_AMMO_556NATO;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = K1A_MAXCLIP;
	p->iSlot = 0;
	p->iPosition = 17;
	p->iId = m_iId = WEAPON_GALIL;
	p->iFlags = 0;
	p->iWeight = GALIL_WEIGHT;

	return 1;
}

BOOL CK1a::Deploy(void)
{
	m_flAccuracy = 0.2;
	m_iShotsFired = 0;
	iShellOn = 1;

	return DefaultDeploy("models/v_k1a.mdl", "models/p_k1a.mdl", K1A_DRAW, "ak47", UseDecrement() != FALSE);
}

void CK1a::PrimaryAttack(void)
{
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15s;
		return;
	}

	if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
		K1aFire(0.04 + (0.4) * m_flAccuracy, 0.0995s, FALSE);
	else if (m_pPlayer->pev->velocity.Length2D() > 140)
		K1aFire(0.04 + (0.125) * m_flAccuracy, 0.0995s, FALSE);
	else
		K1aFire((0.03) * m_flAccuracy, 0.0995s, FALSE);
}

void CK1a::K1aFire(float flSpread, duration_t flCycleTime, BOOL fUseAutoAim)
{
	m_bDelayFire = true;
	m_iShotsFired++;
	m_flAccuracy = ((float)(m_iShotsFired * m_iShotsFired * m_iShotsFired) / 200.0) + 0.35;

	if (m_flAccuracy > 1.75)
		m_flAccuracy = 1.75;

	if (m_iClip <= 0)
	{
		if (m_fFireOnEmpty)
		{
			PlayEmptySound();
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2s;
		}

		return;
	}

	m_iClip--;
	m_pPlayer->pev->effects |= EF_MUZZLEFLASH;
#ifndef CLIENT_DLL
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);
#endif

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = m_pPlayer->FireBullets3(vecSrc, gpGlobals->v_forward, flSpread, 8192, 2, BULLET_PLAYER_556MM, 30, 0.98, m_pPlayer->pev, FALSE, m_pPlayer->random_seed);

	int flags;
#ifdef CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usFireK1a, 0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, (int)(m_pPlayer->pev->punchangle.x * 10000000), (int)(m_pPlayer->pev->punchangle.y * 10000000), FALSE, FALSE);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flCycleTime;

#ifndef CLIENT_DLL
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
#endif
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.9s;

	if (m_pPlayer->pev->velocity.Length2D() > 0)
		KickBack(0.95, 0.4, 0.25, 0.03, 3.75, 2.75, 8);
	else if (!FBitSet(m_pPlayer->pev->flags, FL_ONGROUND))
		KickBack(1.2, 0.575, 0.3, 0.04, 3.75, 3.5, 7);
	else if (FBitSet(m_pPlayer->pev->flags, FL_DUCKING))
		KickBack(0.35, 0.175, 0.15, 0.0175, 1.75, 1.5, 11);
	else
		KickBack(0.4, 0.25, 0.2, 0.025, 2.5, 1.7, 10);
}

void CK1a::Reload(void)
{
	if (m_pPlayer->ammo_556nato <= 0)
		return;

	if (DefaultReload(K1A_MAXCLIP, K1A_RELOAD, 3.0s))
	{
#ifndef CLIENT_DLL
		m_pPlayer->SetAnimation(PLAYER_RELOAD);
#endif
		m_flAccuracy = 0.2;
		m_iShotsFired = 0;
		m_bDelayFire = false;
	}
}

void CK1a::WeaponIdle(void)
{
	ResetEmptySound();
	m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 20s;
	SendWeaponAnim(K1A_IDLE1, UseDecrement() != FALSE);
}

}
