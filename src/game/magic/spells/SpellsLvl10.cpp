/*
 * Copyright 2014 Arx Libertatis Team (see the AUTHORS file)
 *
 * This file is part of Arx Libertatis.
 *
 * Arx Libertatis is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Arx Libertatis is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Arx Libertatis.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "game/magic/spells/SpellsLvl10.h"

#include "core/Application.h"
#include "core/Config.h"
#include "core/GameTime.h"
#include "game/Damage.h"
#include "game/Entity.h"
#include "game/EntityManager.h"
#include "game/NPC.h"
#include "game/Player.h"
#include "game/Spells.h"
#include "game/effect/Quake.h"
#include "game/spell/Cheat.h"

#include "graphics/particle/ParticleEffects.h"

#include "graphics/Draw.h"
#include "graphics/Renderer.h"

#include "scene/GameSound.h"
#include "scene/Interactive.h"

extern Rect g_size;

MassLightningStrikeSpell::MassLightningStrikeSpell()
	: m_pos(Vec3f_ZERO)
	, m_soundEffectPlayed(false)
	, m_light(LightHandle::Invalid)
{
}

MassLightningStrikeSpell::~MassLightningStrikeSpell() {
	
	for(std::vector<CLightning *>::iterator it = pTab.begin(); it != pTab.end(); ++it) {
		delete *it;
	}
	pTab.clear();
}

void MassLightningStrikeSpell::Launch()
{
	spells.endByType(SPELL_MASS_LIGHTNING_STRIKE);
	
	m_duration = 5000; // TODO probably never read
	m_soundEffectPlayed = false;
	
	float beta;
	if(m_caster == PlayerEntityHandle) {
		m_pos = player.pos + Vec3f(0.f, 150.f, 0.f);
		beta = player.angle.getPitch();
	} else {
		Entity * io = entities[m_caster];
		m_pos = io->pos + Vec3f(0.f, -20.f, 0.f);
		beta = io->angle.getPitch();
	}
	m_pos += angleToVectorXZ(beta) * 500.f;
	
	long minDuration = long(500 * m_level);
	long maxDuration = 0;
	
	int number = glm::clamp(int(m_level), 1, 10);
	float ft = 360.0f / (float)number;
	
	for(int i = 0; i < number; i++) {
		Vec3f target = m_pos + angleToVectorXZ(i * ft) * 500.0f;
		long duration = minDuration + Random::get(0, 5000);
		maxDuration = std::max(maxDuration, duration);
		
		CLightning * lightning = new CLightning();
		lightning->m_isMassLightning = true;
		lightning->m_fDamage = 2;
		lightning->Create(m_pos, target);
		lightning->SetDuration(duration);
		pTab.push_back(lightning);
	}
	
	m_duration = maxDuration + 1000;
	
	m_light = GetFreeDynLight();
	if(lightHandleIsValid(m_light)) {
		EERIE_LIGHT * light = lightHandleGet(m_light);
		
		light->intensity = 1.8f;
		light->fallend = 850.f;
		light->fallstart = 500.f;
		light->rgb = Color3f::red; // Color3f(1.f, 0.75f, 0.75f);
		light->pos = m_pos;
	}
	
	ARX_SOUND_PlaySFX(SND_SPELL_LIGHTNING_START);
	m_snd_loop = ARX_SOUND_PlaySFX(SND_SPELL_LIGHTNING_LOOP, &m_pos, 1.f, ARX_SOUND_PLAY_LOOPED);
	
	// Draws White Flash on Screen
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	EERIEDrawBitmap(Rectf(g_size), 0.00009f, NULL, Color::white);
	GRenderer->SetRenderState(Renderer::AlphaBlending, false);
}

void MassLightningStrikeSpell::End() {
	
	endLightDelayed(m_light, 200);
	
	ARX_SOUND_Stop(m_snd_loop);
	ARX_SOUND_PlaySFX(SND_SPELL_LIGHTNING_END);
	
	for(std::vector<CLightning *>::iterator it = pTab.begin(); it != pTab.end(); ++it) {
		delete *it;
	}
	pTab.clear();
}

void MassLightningStrikeSpell::Update(float timeDelta) {
	
	for(size_t i = 0; i < pTab.size(); i++) {
		CLightning * lightning = pTab[i];
		
		lightning->m_caster = m_caster;
		lightning->m_level = m_level;
		
		lightning->Update(timeDelta);
	}
	
	for(size_t i = 0; i < pTab.size(); i++) {
		pTab[i]->Render();
	}
	
	Vec3f position;

	position = m_pos + randomVec(-250.f, 250.f);
	ARX_SOUND_RefreshPosition(m_snd_loop, position);
	ARX_SOUND_RefreshVolume(m_snd_loop, 1.f);
	ARX_SOUND_RefreshPitch(m_snd_loop, 0.8F + 0.4F * rnd());
	
	if(rnd() > 0.62f) {
		position = m_pos + randomVec(-250.f, 250.f);
		ARX_SOUND_PlaySFX(SND_SPELL_SPARK, &position, 0.8F + 0.4F * rnd());
	}
	
	if(rnd() > 0.82f) {
		position = m_pos + randomVec(-250.f, 250.f);
		ARX_SOUND_PlaySFX(SND_SPELL_ELECTRIC, &position, 0.8F + 0.4F * rnd());
	}
	
	if(0 > (long(m_duration) - 1800) && !m_soundEffectPlayed) {
		m_soundEffectPlayed = true;
		ARX_SOUND_PlaySFX(SND_SPELL_ELECTRIC, NULL, 0.8F + 0.4F * rnd());
	}

	if(lightHandleIsValid(m_light)) {
		EERIE_LIGHT * light = lightHandleGet(m_light);
		
		light->intensity = 1.3f + rnd() * 1.f;
	}	
}


bool ControlTargetSpell::CanLaunch()
{
	if(!ValidIONum(m_target)) {
		return false;
	}
	
	long tcount = 0;
	for(size_t ii = 1; ii < entities.size(); ii++) {
		const EntityHandle handle = EntityHandle(ii);
		Entity * ioo = entities[handle];
		
		if(!ioo || !(ioo->ioflags & IO_NPC)) {
			continue;
		}
		
		if(ioo->_npcdata->lifePool.current <= 0.f || ioo->show != SHOW_FLAG_IN_SCENE) {
			continue;
		}
		
		if(ioo->groups.find("demon") == ioo->groups.end()) {
			continue;
		}
		
		if(closerThan(ioo->pos, m_caster_pos, 900.f)) {
			tcount++;
		}
	}
	if(tcount == 0) {
		return false;
	}
	
	return true;
}

void ControlTargetSpell::Launch()
{
	// TODO copy-paste
	for(size_t ii = 1; ii < entities.size(); ii++) {
		const EntityHandle handle = EntityHandle(ii);
		Entity * ioo = entities[handle];
		
		if(!ioo || !(ioo->ioflags & IO_NPC)) {
			continue;
		}
		
		if(ioo->_npcdata->lifePool.current <= 0.f || ioo->show != SHOW_FLAG_IN_SCENE) {
			continue;
		}
		
		if(ioo->groups.find("demon") == ioo->groups.end()) {
			continue;
		}
		
		if(closerThan(ioo->pos, m_caster_pos, 900.f)) {
			std::ostringstream oss;
			oss << entities[m_target]->idString();
			oss << ' ' << long(m_level);
			SendIOScriptEvent(ioo, SM_NULL, oss.str(), "npc_control");
		}
	}
	
	ARX_SOUND_PlaySFX(SND_SPELL_CONTROL_TARGET);
	
	m_duration = 1000;
	
	eSrc = Vec3f_ZERO;
	eTarget = Vec3f_ZERO;
	fTrail = 0.f;
	
	tex_mm = TextureContainer::Load("graph/obj3d/textures/(fx)_ctrl_target");
	
	eSrc = player.pos;
	
	float fBetaRad = glm::radians(player.angle.getPitch());
	float fBetaRadCos = glm::cos(fBetaRad);
	float fBetaRadSin = glm::sin(fBetaRad);
	
	eTarget = eSrc + Vec3f(-fBetaRadSin * 1000.f, 100.f, fBetaRadCos * 1000.f);
	
	for(size_t i = 1; i < entities.size(); i++) {
		const EntityHandle handle = EntityHandle(i);
		Entity * e = entities[handle];
		
		if(e) {
			eTarget = e->pos;
		}
	}
	
	Vec3f h;
	Vec3f s = eSrc;
	Vec3f e = eSrc;
	int i = 0;
	while(Visible(s, e, NULL, &h) && i < 20) {
		e.x -= fBetaRadSin * 50;
		e.z += fBetaRadCos * 50;
		i++;
	}
	
	pathways[0] = eSrc + Vec3f(0.f, 100.f, 0.f);
	pathways[9] = eTarget;
	Split(pathways, 0, 9, 150);
	
	for(int i = 0; i < 9; i++) {
		if(pathways[i].y >= eSrc.y + 150) {
			pathways[i].y = eSrc.y + 150;
		}
	}
	
	fTrail = 0;
}

void ControlTargetSpell::End() {
	
}

void ControlTargetSpell::Update(float timeDelta) {
	
	ulCurrentTime += timeDelta;
	
	GRenderer->SetCulling(Renderer::CullNone);
	GRenderer->SetRenderState(Renderer::DepthWrite, false);
	GRenderer->SetRenderState(Renderer::AlphaBlending, true);
	GRenderer->SetBlendFunc(Renderer::BlendOne, Renderer::BlendOne);
	GRenderer->SetTexture(0, tex_mm);

	// -------------------
	fTrail += 1;

	if (fTrail >= 300) fTrail = 0;

	int n = BEZIERPrecision;
	float delta = 1.0f / n;

	float fOneOnDuration = 1.f / (float)(m_duration);
	fTrail = (ulCurrentTime * fOneOnDuration) * 9 * (n + 2);

	Vec3f v;
	
	Vec3f newpos = Vec3f_ZERO;
	Vec3f lastpos = pathways[0];
	
	for(int i = 0; i < 9; i++) {
		int kp		= i;
		int kpprec	= (i > 0) ? kp - 1 : kp ;
		int kpsuiv	= kp + 1 ;
		int kpsuivsuiv = (i < (9 - 2)) ? kpsuiv + 1 : kpsuiv;

		for(int toto = 1; toto < n; toto++) {
			if(fTrail < i * n + toto)
				break;

			float t = toto * delta;

			float t1 = t;
			float t2 = t1 * t1 ;
			float t3 = t2 * t1 ;
			float f0 = 2.f * t3 - 3.f * t2 + 1.f ;
			float f1 = -2.f * t3 + 3.f * t2 ;
			float f2 = t3 - 2.f * t2 + t1 ;
			float f3 = t3 - t2 ;

			float val = pathways[kpsuiv].x;
			float p0 = 0.5f * (val - pathways[kpprec].x) ;
			float p1 = 0.5f * (pathways[kpsuivsuiv].x - pathways[kp].x) ;
			v.x = f0 * pathways[kp].x + f1 * val + f2 * p0 + f3 * p1 ;

			val = pathways[kpsuiv].y ;
			p0 = 0.5f * (val - pathways[kpprec].y) ;
			p1 = 0.5f * (pathways[kpsuivsuiv].y - pathways[kp].y) ;
			v.y = f0 * pathways[kp].y + f1 * val + f2 * p0 + f3 * p1 ;

			val = pathways[kpsuiv].z ;
			p0 = 0.5f * (val - pathways[kpprec].z) ;
			p1 = 0.5f * (pathways[kpsuivsuiv].z - pathways[kp].z) ;
			v.z = f0 * pathways[kp].z + f1 * val + f2 * p0 + f3 * p1 ;
			
			newpos = v;
			
			if(fTrail - (i * n + toto) <= 70) {
				float c = 1.0f - (fTrail - (i * n + toto)) / 70.0f;
				PARTICLE_DEF * pd = createParticle();
				if(pd) {
					pd->ov = lastpos;
					pd->siz = 5 * c;
					pd->tolive = Random::get(10, 110);
					pd->tc = tex_mm;
					pd->special = FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION | DISSIPATING;
					pd->fparam = 0.0000001f;
					pd->rgb = Color3f::gray(c);
				}
			}
			
			std::swap(lastpos, newpos);
			
			PARTICLE_DEF * pd = createParticle();
			if(pd) {
				pd->ov = lastpos;
				pd->siz = 5;
				pd->tolive = Random::get(10, 110);
				pd->tc = tex_mm;
				pd->special = FADE_IN_AND_OUT | ROTATING | MODULATE_ROTATION | DISSIPATING;
				pd->fparam = 0.0000001f;
				pd->rgb = Color3f::gray(0.1f);
			}
		}
	}
}


extern float GLOBAL_SLOWDOWN;

FreezeTimeSpell::FreezeTimeSpell()
	: m_slowdown(0.f)
{
	
}

bool FreezeTimeSpell::CanLaunch()
{
	return !spells.ExistAnyInstanceForThisCaster(m_type, m_caster);
}

void FreezeTimeSpell::Launch()
{
	ARX_SOUND_PlaySFX(SND_SPELL_FREEZETIME);
	
	float max_slowdown = std::max(0.f, GLOBAL_SLOWDOWN - 0.01f);
	m_slowdown = glm::clamp(m_level * 0.08f, 0.f, max_slowdown);
	GLOBAL_SLOWDOWN -= m_slowdown;
	
	m_duration = (m_launchDuration > -1) ? m_launchDuration : 200000;
	m_hasDuration = true;
	m_fManaCostPerSecond = 30.f * m_slowdown;
}

void FreezeTimeSpell::End()
{
	GLOBAL_SLOWDOWN += m_slowdown;
	ARX_SOUND_PlaySFX(SND_SPELL_TELEKINESIS_END, &entities[m_caster]->pos);
}

void MassIncinerateSpell::Launch()
{
	ARX_SOUND_PlaySFX(SND_SPELL_MASS_INCINERATE);
	
	m_duration = 20000;
	
	long nb_targets=0;
	for(size_t ii = 0; ii < entities.size(); ii++) {
		const EntityHandle handle = EntityHandle(ii);
		Entity * tio = entities[handle];
		
		if(long(ii) == m_caster || !tio || !(tio->ioflags & IO_NPC)) {
			continue;
		}
		
		if(tio->_npcdata->lifePool.current <= 0.f || tio->show != SHOW_FLAG_IN_SCENE) {
			continue;
		}
		
		if(fartherThan(tio->pos, entities[m_caster]->pos, 500.f)) {
			continue;
		}
		
		tio->sfx_flag |= SFX_TYPE_YLSIDE_DEATH | SFX_TYPE_INCINERATE;
		tio->sfx_time = (unsigned long)(arxtime);
		nb_targets++;
		m_targets.push_back(tio->index());
	}
	
	if(nb_targets) {
		m_snd_loop = ARX_SOUND_PlaySFX(SND_SPELL_INCINERATE_LOOP, &m_caster_pos, 1.f, ARX_SOUND_PLAY_LOOPED);
	} else {
		m_snd_loop = audio::INVALID_ID;
	}
}

void MassIncinerateSpell::End()
{
	m_targets.clear();
	ARX_SOUND_Stop(m_snd_loop);
	ARX_SOUND_PlaySFX(SND_SPELL_INCINERATE_END);
}

void MassIncinerateSpell::Update(float timeDelta)
{
	ARX_UNUSED(timeDelta);
	
	if(ValidIONum(m_caster)) {
		ARX_SOUND_RefreshPosition(m_snd_loop, entities[m_caster]->pos);
	}	
}

float LASTTELEPORT = 0.0F;

bool TeleportSpell::CanLaunch()
{
	return !spells.ExistAnyInstanceForThisCaster(m_type, m_caster);
}

void TeleportSpell::Launch()
{
	m_duration = 7000;
	
	ARX_SOUND_PlaySFX(SND_SPELL_TELEPORT, &m_caster_pos);
	
	if(m_caster == PlayerEntityHandle) {
		LASTTELEPORT = 0.f;
	}
}

void TeleportSpell::End()
{
	ARX_SOUND_PlaySFX(SND_MAGIC_FIZZLE, &m_caster_pos);
}

extern Vec3f lastteleport;

void TeleportSpell::Update(float timeDelta)
{
	ARX_UNUSED(timeDelta);
	
	const unsigned long tim = (unsigned long)(arxtime);
	
	float TELEPORT = (float)(((float)tim-(float)m_timcreation)/(float)m_duration);

	if(LASTTELEPORT < 0.5f && TELEPORT >= 0.5f) {
		Vec3f pos = lastteleport;
		lastteleport = player.pos;
		player.pos = pos;
		LASTTELEPORT = 32.f;
		ARX_SOUND_PlaySFX(SND_SPELL_TELEPORTED, &player.pos);
	} else {
		LASTTELEPORT = TELEPORT;
	}	
}
