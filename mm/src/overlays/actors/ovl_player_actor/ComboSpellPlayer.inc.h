/*
MIT License

Copyright (c) 2020-2022 OoTMM Team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
// OoTMM Player.c MIT, pinned 41a2b7c0d7dbbb53fed928652f8271bc336c113f.
#ifdef COMBO_BUILD
#include "2s2h/Rando/Spells.h"
#define ANIMSFX(type,frame) (((type)<<11)|((frame)&0x7ff))
static u8 sMagicSpellCosts[] =
{
    12,
    24,
    12,
};

static s16 sMagicSpellActorIds[] =
{
    ACTOR_COMBO_MAGIC_WIND,
    ACTOR_COMBO_MAGIC_DARK,
    ACTOR_COMBO_MAGIC_FIRE,
};


static AnimSfxEntry sMagicSpellStartSfx[] =
{
    { 0x83D, ANIMSFX(1, 20) }, /* NA_SE_PL_SKIP */
    { 0x6800, ANIMSFX(4, 20) }, /* NA_SE_VO_LI_SWORD_N */
    { 0, -ANIMSFX(6, 26) },
};

static AnimSfxEntry sMagicSpellSfx[][2] =
{
    {
        { 0, ANIMSFX(8, 20) },
        { 0x6817, -ANIMSFX(4, 30) }, /* NA_SE_VO_LI_MAGIC_FROL */
    },
    {
        { 0, ANIMSFX(8, 20) },
        { 0x6815, -ANIMSFX(4, 44) }, /* NA_SE_VO_LI_MAGIC_NALE */
    },
    {
        { 0x681C, ANIMSFX(4, 20) }, /* NA_SE_VO_LI_MAGIC_ATTACK */
        { 0x1818, -ANIMSFX(1, 20) }, /* NA_SE_IT_SWORD_SWING_HARD */
    },
    {
        { 0x6801, ANIMSFX(4, 20) }, /* NA_SE_VO_LI_SWORD_L */
        { 0x1818, -ANIMSFX(1, 20) }, /* NA_SE_IT_SWORD_SWING_HARD */
    },
};

static PlayerAnimationHeader* sMagicSpellFirstAnimation[] =
{
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_kaze1, /* &gPlayerAnim_link_magic_kaze1, */
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_honoo1, /* &gPlayerAnim_link_magic_honoo1, */
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_tamashii1, /* &gPlayerAnim_link_magic_tamashii1, */
};

static PlayerAnimationHeader* sMagicSpellSecondAnimation[] =
{
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_kaze2, /* &gPlayerAnim_link_magic_kaze2, */
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_honoo2, /* &gPlayerAnim_link_magic_honoo2, */
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_tamashii2, /* &gPlayerAnim_link_magic_tamashii2, */
};

static PlayerAnimationHeader* sMagicSpellThirdAnimation[] =
{
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_kaze3, /* &gPlayerAnim_link_magic_kaze3, */
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_honoo3, /* &gPlayerAnim_link_magic_honoo3, */
    (PlayerAnimationHeader*)gPlayerAnim_link_magic_tamashii3, /* &gPlayerAnim_link_magic_tamashii3, */
};

static u8 sMagicSpellLoopTimer[] = { 70, 10, 10 };

Actor* Combo_MMSpawnSpellActor(PlayState* play, Player* this, s8 magicSpell)
{
    return Actor_Spawn(&play->actorCtx, play, sMagicSpellActorIds[magicSpell], this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, 0, 0, 0, 0);
}
void Combo_MMSpellCasting(Player* this, PlayState* play)
{

    if (PlayerAnimation_Update(play, &this->skelAnime))
    {
        if (this->av1.actionVar1 < 0)
        {
            if ((this->itemAction == PLAYER_IA_COMBO_NAYRUS_LOVE) || (gSaveContext.magicState == MAGIC_STATE_IDLE))
            {

                func_808369F4(this, play);

                Player_StopCutscene(this);
            }
        }
        else
        {
            if (this->av2.actionVar2 == 0)
            {
                PlayerAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sMagicSpellFirstAnimation[this->av1.actionVar1], 0.83f);

                if (Combo_MMSpawnSpellActor(play, this, this->av1.actionVar1) != NULL)
                {
                    this->stateFlags1 |= 0x30000000; /* PLAYER_STATE1_20000000 | PLAYER_STATE1_10000000 */
                    if ((this->av1.actionVar1 != 0) || (gSaveContext.respawn[RESPAWN_MODE_HUMAN].data <= 0))
                    {
                        gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
                    }
                }
                else
                {
                    Magic_Reset(play);
                }
            }
            else
            {
                PlayerAnimation_PlayLoopSetSpeed(play, &this->skelAnime, sMagicSpellSecondAnimation[this->av1.actionVar1], 0.83f);

                if (this->av1.actionVar1 == 0)
                {
                    this->av2.actionVar2 = -10;
                }
            }

            this->av2.actionVar2++;
        }
    }
    else
    {
        if (this->av2.actionVar2 < 0)
        {
            this->av2.actionVar2++;

            if (this->av2.actionVar2 == 0)
            {
                Combo_MMFaroreStorePoint(play);
                this->av2.actionVar2 = 2;
            }
        }
        else if (this->av1.actionVar1 >= 0)
        {

            if (this->av2.actionVar2 == 0)
            {
                Player_PlayAnimSfx(this, sMagicSpellStartSfx);
            }
            else if (this->av2.actionVar2 == 1)
            {
                s8 spellSfxIndex = this->av1.actionVar1;
                if (spellSfxIndex == 2 && Combo_MM_IsLinkAdult())
                {
                    spellSfxIndex = 3;
                }
                Player_PlayAnimSfx(this, sMagicSpellSfx[spellSfxIndex]);
                if ((this->av1.actionVar1 == 2) && PlayerAnimation_OnFrame(&this->skelAnime, 30.0f))
                {
                    this->stateFlags1 &= ~0x30000000; /* PLAYER_STATE1_20000000 | PLAYER_STATE1_10000000 */
                }
            }
            else if (sMagicSpellLoopTimer[this->av1.actionVar1] < this->av2.actionVar2++)
            {
                PlayerAnimation_PlayOnceSetSpeed(play, &this->skelAnime, sMagicSpellThirdAnimation[this->av1.actionVar1], 0.83f);
                this->yaw = this->actor.shape.rot.y;
                this->av1.actionVar1 = -1;
            }
        }
    }

    Player_DecelerateToZero(this);

    func_8083249C(this);
}
void Combo_MMSpellUseItem(Player* this,PlayState* play,int spell){
 if(!Combo_MMSpellEnabled(spell)||this->transformation!=PLAYER_FORM_HUMAN||!Combo_MMSpellPrepareResources(spell)){Audio_PlaySfx(NA_SE_SY_ERROR);return;}
 if(gSaveContext.magicCapacity!=0&&gSaveContext.magicState==MAGIC_STATE_IDLE&&
    gSaveContext.save.saveInfo.playerData.magic>=sMagicSpellCosts[spell]){
  this->unk_AA5=PLAYER_UNKAA5_5;
  this->itemAction=spell==1?PLAYER_IA_COMBO_NAYRUS_LOVE:PLAYER_IA_COMBO_DINS_FIRE;
 }else Audio_PlaySfx(NA_SE_SY_ERROR);
}
void Combo_MMSpellStart(Player* this,PlayState* play,int spell){
 if(Combo_MMSpellEnabled(spell)&&Combo_MMSpellPrepareResources(spell)&&
    Magic_Consume(play,sMagicSpellCosts[spell],MAGIC_CONSUME_WAIT_PREVIEW)){
  Player_SetAction_PreserveItemAction(play,this,Combo_MMSpellCasting,0);this->av1.actionVar1=spell;
  PlayerAnimation_PlayOnceSetSpeed(play,&this->skelAnime,(PlayerAnimationHeader*)gPlayerAnim_link_magic_tame,0.83f);
  this->csId=CS_ID_GLOBAL_TALK;func_8083249C(this);this->stateFlags3|=0x80;
 }
 func_8082DAD4(this);
}
void Combo_MMSpellAfterInit(Player* this,PlayState* play){
 if(gSaveContext.nayrusLoveTimer!=0&&Combo_MMSpellEnabled(1)&&Combo_MMSpellPrepareResources(1)){
  gSaveContext.magicState=MAGIC_STATE_METER_FLASH_1;Combo_MMSpawnSpellActor(play,this,1);
 }
}
#undef ANIMSFX
#endif
