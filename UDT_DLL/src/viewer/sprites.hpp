#pragma once


#define SPRITE_LIST(N) \
	N(ammo_gl, "ammo_gl") \
	N(ammo_lg, "ammo_lg") \
	N(ammo_mg, "ammo_mg") \
	N(ammo_pg, "ammo_pg") \
	N(ammo_rg, "ammo_rg") \
	N(ammo_rl, "ammo_rl") \
	N(ammo_sg, "ammo_sg") \
	N(armor_green, "armor_green") \
	N(armor_red, "armor_red") \
	N(armor_shard, "armor_shard") \
	N(armor_yellow, "armor_yellow") \
	N(dead_player, "dead_player") \
	N(explosion, "explosion") \
	N(explosion_0, "explosion_0") \
	N(explosion_1, "explosion_1") \
	N(explosion_2, "explosion_2") \
	N(explosion_3, "explosion_3") \
	N(explosion_4, "explosion_4") \
	N(explosion_5, "explosion_5") \
	N(explosion_6, "explosion_6") \
	N(explosion_7, "explosion_7") \
	N(flag_blue, "flag_blue") \
	N(flag_red, "flag_red") \
	N(gauntlet, "gauntlet") \
	N(gauntlet_firing, "gauntlet_firing") \
	N(gl, "gl") \
	N(gl_firing, "gl_firing") \
	N(health_large, "health_large") \
	N(health_medium, "health_medium") \
	N(health_mega, "health_mega") \
	N(health_small, "health_small") \
	N(lg, "lg") \
	N(lg_firing, "lg_firing") \
	N(mg, "mg") \
	N(mg_firing, "mg_firing") \
	N(pg, "pg") \
	N(pg_firing, "pg_firing") \
	N(powerup_bs, "powerup_bs") \
	N(powerup_flight, "powerup_flight") \
	N(powerup_haste, "powerup_haste") \
	N(powerup_invis, "powerup_invis") \
	N(powerup_quad, "powerup_quad") \
	N(powerup_regen, "powerup_regen") \
	N(rg, "rg") \
	N(rg_firing, "rg_firing") \
	N(rl, "rl") \
	N(rl_firing, "rl_firing") \
	N(rocket, "rocket") \
	N(sg, "sg") \
	N(sg_firing, "sg_firing") \
	N(weapon_gauntlet, "weapon_gauntlet") \
	N(weapon_gl, "weapon_gl") \
	N(weapon_lg, "weapon_lg") \
	N(weapon_mg, "weapon_mg") \
	N(weapon_pg, "weapon_pg") \
	N(weapon_rg, "weapon_rg") \
	N(weapon_rl, "weapon_rl") \
	N(weapon_sg, "weapon_sg") \
	N(impact_plasma, "impact_plasma") \
	N(impact_bullet_0, "impact_bullet_0") \
	N(impact_bullet_1, "impact_bullet_1") \
	N(impact_bullet_2, "impact_bullet_2") \
	N(projectile_plasma, "projectile_plasma")

#define ITEM(Enum, FileNameNoExt) Enum,
struct Sprite
{
	enum Id
	{
		SPRITE_LIST(ITEM)
		Count,
		ExplosionFrames = 8,
		BulletImpactFrames = 3
	};
};
#undef ITEM
