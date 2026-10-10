// OoTMM Player.c 41a2b7c0d7dbbb53fed928652f8271bc336c113f, MIT.
// Native names replace overlay addresses; only the WIND action is imported.
#ifdef COMBO_BUILD
static AnimSfxEntry sComboWindStartSfx[]={
 {0x83D,(1<<11)|20},{0x6800,(4<<11)|20},{0,-((6<<11)|26)}};
static AnimSfxEntry sComboWindCastSfx[]={
 {0,(8<<11)|20},{0x6817,-((4<<11)|30)}};
static void Combo_MMFaroreCasting(Player* this,PlayState* play) {
 if(PlayerAnimation_Update(play,&this->skelAnime)) {
  if(this->av1.actionVar1<0) {
   if(gSaveContext.magicState==MAGIC_STATE_IDLE) {func_808369F4(this,play);Player_StopCutscene(this);}
  } else {
   if(this->av2.actionVar2==0) {
    PlayerAnimation_PlayOnceSetSpeed(play,&this->skelAnime,(PlayerAnimationHeader*)gPlayerAnim_link_magic_kaze1,0.83f);
    if(Actor_Spawn(&play->actorCtx,play,ACTOR_COMBO_MAGIC_WIND,this->actor.world.pos.x,this->actor.world.pos.y,
       this->actor.world.pos.z,0,0,0,0)!=NULL) {
      this->stateFlags1|=0x30000000;
      if(gSaveContext.respawn[RESPAWN_MODE_HUMAN].data<=0)gSaveContext.magicState=MAGIC_STATE_CONSUME_SETUP;
    } else Magic_Reset(play);
   } else {
    PlayerAnimation_PlayLoopSetSpeed(play,&this->skelAnime,(PlayerAnimationHeader*)gPlayerAnim_link_magic_kaze2,0.83f);
    this->av2.actionVar2=-10;
   }
   this->av2.actionVar2++;
  }
 } else {
  if(this->av2.actionVar2<0) {
   if(++this->av2.actionVar2==0) {Combo_MMFaroreStorePoint(play);this->av2.actionVar2=2;}
  } else if(this->av1.actionVar1>=0) {
   if(this->av2.actionVar2==0)Player_PlayAnimSfx(this,sComboWindStartSfx);
   else if(this->av2.actionVar2==1)Player_PlayAnimSfx(this,sComboWindCastSfx);
   else if(70<this->av2.actionVar2++) {
    PlayerAnimation_PlayOnceSetSpeed(play,&this->skelAnime,(PlayerAnimationHeader*)gPlayerAnim_link_magic_kaze3,0.83f);
    this->yaw=this->actor.shape.rot.y;this->av1.actionVar1=-1;
   }
  }
 }
 Player_DecelerateToZero(this);func_8083249C(this);
}
static void Combo_MMFaroreText(Player* this,PlayState* play) {
 this->stateFlags3|=0x80;
 PlayerAnimation_Update(play,&this->skelAnime);Player_UpdateUpperBody(this,play);
 if(this->av2.actionVar2==0) {
  static const char text[]="\x17" "You cast Farore's Wind!\x11\x02\xc3"
   "     Return to the Warp Point\x11     Dispel the Warp Point\x11     Exit\xbf";
  Message_StartTextbox(play,0x3B,&this->actor);
  play->msgCtx.font.msgBuf.schar[2]=0xFE;
  memcpy(play->msgCtx.font.msgBuf.schar+11,text,sizeof(text));
  this->av2.actionVar2=1;return;
 }
 if(play->msgCtx.msgMode==0x42&&Message_ShouldAdvance(play)) {
  Audio_PlaySfx_MessageDecide();Message_CloseTextbox(play);
  if(play->msgCtx.choiceIndex==0) {if(Combo_MMFaroreReturn(play))return;}
  if(play->msgCtx.choiceIndex==1)Combo_MMFaroreDispel();
  func_8085B384(this,play);Player_StopCutscene(this);
 }
}
void Combo_MMFaroreUseItem(Player* this,PlayState* play) {
 if(!Combo_MMFaroreEnabled()||Combo_MMFaroreAge()<0||this->transformation!=PLAYER_FORM_HUMAN||
    play->interfaceCtx.restrictions.songOfSoaring||!Combo_MMFarorePrepareResources()) {Audio_PlaySfx(NA_SE_SY_ERROR);return;}
 if(Combo_MMFaroreHasValidPoint()||(gSaveContext.magicCapacity!=0&&gSaveContext.magicState==MAGIC_STATE_IDLE&&
    gSaveContext.save.saveInfo.playerData.magic>=12)) {
  this->unk_AA5=PLAYER_UNKAA5_5;this->itemAction=PLAYER_IA_COMBO_FARORES_WIND;
 } else Audio_PlaySfx(NA_SE_SY_ERROR);
}
void Combo_MMFaroreStart(Player* this,PlayState* play) {
 if(!Combo_MMFaroreEnabled()||Combo_MMFaroreAge()<0||!Combo_MMFarorePrepareResources()) {Audio_PlaySfx(NA_SE_SY_ERROR);func_8082DAD4(this);return;}
 if(!Combo_MMFaroreHasValidPoint()) {
  if(Magic_Consume(play,12,MAGIC_CONSUME_WAIT_PREVIEW)) {
   Player_SetAction_PreserveItemAction(play,this,Combo_MMFaroreCasting,0);this->av1.actionVar1=0;
   PlayerAnimation_PlayOnceSetSpeed(play,&this->skelAnime,(PlayerAnimationHeader*)gPlayerAnim_link_magic_tame,0.83f);
   this->csId=CS_ID_GLOBAL_TALK;func_8083249C(this);this->stateFlags3|=0x80;
  }
 } else {
  Player_SetAction(play,this,Combo_MMFaroreText,1);this->stateFlags1|=0x30000000;
  PlayerAnimation_PlayOnce(play,&this->skelAnime,Player_GetIdleAnim(this));
  this->csId=CS_ID_GLOBAL_TALK;func_8083249C(this);
 }
 func_8082DAD4(this);
}
static void Combo_MMFaroreSpawning(Player* this,PlayState* play) {
 if(this->av2.actionVar2>20) {
  this->actor.draw=Player_Draw;this->actor.world.pos.y+=60.0f;func_80833AA0(this,play);return;
 }
 if(this->av2.actionVar2++==20) {
  gSaveContext.respawn[RESPAWN_MODE_HUMAN].data++;
  int age=Combo_MMFaroreAge();
  if(age>=0)Audio_PlaySfx_AtPos(&gSaveContext.save.shipSaveInfo.faroresWind.point[age].pos,NA_SE_PL_MAGIC_WIND_WARP);
 }
}
void Combo_MMFaroreAfterInit(Player* this,PlayState* play) {
 Combo_MMFaroreInitPointer(play);
 if(gSaveContext.respawnFlag==8) {
  this->actor.draw=NULL;Player_SetAction(play,this,Combo_MMFaroreSpawning,0);this->stateFlags1|=PLAYER_STATE1_20000000;
 }
}
#endif

#ifdef COMBO_BUILD
void Combo_MMFaroreCancelReturn(PlayState* play) {
 Player* player=GET_PLAYER(play);
 play->transitionTrigger=TRANS_TRIGGER_OFF;
 func_8085B384(player,play);Player_StopCutscene(player);Audio_PlaySfx(NA_SE_SY_ERROR);
}
#endif
