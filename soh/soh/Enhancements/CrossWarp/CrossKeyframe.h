#ifndef COMBO_CROSS_KEYFRAME_H
#define COMBO_CROSS_KEYFRAME_H

#include "global.h"
#include "z64math.h"

struct PlayState;

struct CWKFSkelAnimeFlex;

typedef s32 (*CWOverrideKeyframeDrawScaled)(struct PlayState* play, struct CWKFSkelAnimeFlex* kfSkelAnime,
                                            s32 limbIndex, Gfx** dList, u8* flags, void* arg, Vec3f* scale, Vec3s* rot,
                                            Vec3f* pos);
typedef s32 (*CWPostKeyframeDrawScaled)(struct PlayState* play, struct CWKFSkelAnimeFlex* kfSkelAnime, s32 limbIndex,
                                        Gfx** dList, u8* flags, void* arg, Vec3f* scale, Vec3s* rot, Vec3f* pos);

typedef s32 (*KeyframeTransformCallback)(struct PlayState* play, struct CWKFSkelAnimeFlex* kfSkelAnime, s32 limbIndex,
                                         Gfx** dList, u8* flags, void* arg);

// These flags are mutually exclusive, if XLU is set OPA is not set and vice-versa.
#define KEYFRAME_DRAW_OPA (0 << 0)
#define KEYFRAME_DRAW_XLU (1 << 0)

typedef enum {
    /* 0 */ KEYFRAME_NOT_DONE,
    /* 1 */ KEYFRAME_DONE_ONCE,
    /* 2 */ KEYFRAME_DONE_LOOP
} CWKeyFrameDoneType;

#define KF_CALLBACK_INDEX_NONE 0xFF

typedef struct {
    /* 0x00 */ Gfx* dList;     // Display list for this limb
    /* 0x04 */ u8 numChildren; // Number of children for this limb
    /* 0x05 */ u8
        drawFlags; // KEYFRAME_DRAW_*, determines which render layer the matrix + display list will be appended to
    /* 0x06 */ u8 callbackIndex; // Transform callback function index, KF_CALLBACK_INDEX_NONE indicates no callback
} CWKeyFrameFlexLimb;            // size = 0x8

typedef struct {
    /* 0x00 */ u8 limbCount;  // Number of limbs in this skeleton
    /* 0x01 */ u8 dListCount; // Number of limbs with a non-NULL display list, used to know how many matrices to
                              // allocate for drawing
    /* 0x04 */ CWKeyFrameFlexLimb* limbs; // Pointer to flex limb array
} CWKeyFrameFlexSkeleton;                 // Size = 0x8

typedef struct {
    /* 0x00 */ s16 frame;    // Frame number for this keyframe
    /* 0x02 */ s16 value;    // Value (any of translation, rotation, scale)
    /* 0x04 */ s16 velocity; // The instantaneous rate of change of the value
} KeyFrame;                  // Size = 0x6

typedef struct {
    // Array of bitflags for each limb indicating whether to do keyframe interpolation
    // or pull from fixed values that do not change throughout the animation.
    union {
        // For standard the bit layout in each array element is:
        // [5] X Translation (root limb only)
        // [4] Y Translation (root limb only)
        // [3] Z Translation (root limb only)
        // [2] X Rotation (all limbs)
        // [1] Y Rotation (all limbs)
        // [0] Z Rotation (all limbs)
        /* 0x00 */ u8* standard;
        // For flex the bit layout in each array element is:
        // [8] X Scale
        // [7] Y Scale
        // [6] Z Scale
        // [5] X Rotation
        // [4] Y Rotation
        // [3] Z Rotation
        // [2] X Translation
        // [1] Y Translation
        // [0] Z Translation
        /* 0x00 */ u16* flex;
    } bitFlags;
    /* 0x04 */ KeyFrame* keyFrames; // Array of keyframes determining the motion, grouped by limb
    /* 0x08 */ s16* kfNums;         // Array containing how many keyframes belong to each limb
    /* 0x0C */ s16*
        fixedValues; // Array of fixed rotation (standard skeleton) or scale/rotation/translation (flex skeleton) values
    /* 0x10 */ UNK_TYPE2 unk_10;
    /* 0x12 */ s16 frameCount; // Length of the animation in 30fps frames
} CWKeyFrameAnimation;         // Size = 0x14

typedef enum {
    /* 0 */ KEYFRAME_ANIM_ONCE, // Play once and stop
    /* 1 */ KEYFRAME_ANIM_LOOP  // Play in a loop
} CWKeyFrameAnimMode;

typedef struct {
    /* 0x00 */ f32 start;      // Current animation start frame number
    /* 0x04 */ f32 end;        // Current animation end frame number
    /* 0x08 */ f32 frameCount; // Current animation total frame count
    /* 0x0C */ f32 speed;      // Current play speed
    /* 0x10 */ f32 curTime;    // Current play frame number
    /* 0x14 */ s32 animMode;   // Current play mode (see FrameAnimMode)
} CWFrameControl;              // Size = 0x18

typedef struct CWKFSkelAnimeFlex {
    /* 0x00 */ CWFrameControl frameCtrl;                      // Current play state
    /* 0x18 */ CWKeyFrameFlexSkeleton* skeleton;              // Skeleton to animate
    /* 0x1C */ CWKeyFrameAnimation* animation;                // Currently active animation
    /* 0x20 */ KeyframeTransformCallback* transformCallbacks; // Pointer to array of limb transform callbacks, indexed
                                                              // by callbackIndex in CWKeyFrameFlexLimb
    /* 0x24 */ f32
        morphFrames; // Number of frames in which to morph between the previous pose and the current animation
    /* 0x28 */ Vec3s* jointTable; // Array of data describing the current pose
                                  //    size = 3 * limbCount in order of (scale, rotation, translation) for each limb
    /* 0x2C */ Vec3s* morphTable; // Array of data describing the current morph pose to interpolate with
                                  //    size = 3 * limbCount in order of (scale, rotation, translation) for each limb
} CWKFSkelAnimeFlex;              // Size = 0x30

void CWFrameCtrl_Reset(CWFrameControl* frameCtrl);
void CWFrameCtrl_Init(CWFrameControl* frameCtrl);
void CWFrameCtrl_SetProperties(CWFrameControl* frameCtrl, f32 startTime, f32 endTime, f32 frameCount, f32 t, f32 speed,
                               s32 animMode);
s32 CWFrameCtrl_PassCheck(CWFrameControl* frameCtrl, f32 t, f32* remainingTime);
s32 CWFrameCtrl_UpdateOnce(CWFrameControl* frameCtrl);
s32 CWFrameCtrl_UpdateLoop(CWFrameControl* frameCtrl);
s32 CWFrameCtrl_Update(CWFrameControl* frameCtrl);

void CWKeyframe_ResetFlex(CWKFSkelAnimeFlex* kfSkelAnime);
void CWKeyframe_InitFlex(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameFlexSkeleton* skeleton,
                         CWKeyFrameAnimation* animation, Vec3s* jointTable, Vec3s* morphTable,
                         KeyframeTransformCallback* transformCallbacks);
void CWKeyframe_FlexPlayOnce(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameAnimation* animation);
void CWKeyframe_FlexChangeAnim(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameFlexSkeleton* skeleton,
                               CWKeyFrameAnimation* animation, f32 startTime, f32 endTime, f32 t, f32 speed,
                               f32 morphFrames, s32 animMode);
f32 CWKeyframe_Interpolate(f32 t, f32 delta, f32 x0, f32 x1, f32 v0, f32 v1);
s16 CWKeyframe_KeyCalc(s16 kfStart, s16 kfNum, KeyFrame* keyFrames, f32 t);
void CWKeyframe_MorphInterpolateRotation(f32 t, s16* out, s16 rot1, s16 rot2);
void CWKeyframe_MorphInterpolateLinear(s16* jointData, s16* morphData, f32 t);
void CWKeyframe_FlexMorphInterpolation(CWKFSkelAnimeFlex* kfSkelAnime);
s32 CWKeyframe_UpdateFlex(CWKFSkelAnimeFlex* kfSkelAnime);
void CWKeyframe_DrawFlex(struct PlayState* play, CWKFSkelAnimeFlex* kfSkelAnime, Mtx* mtxStack,
                         CWOverrideKeyframeDrawScaled overrideKeyframeDraw, CWPostKeyframeDrawScaled postKeyframeDraw,
                         void* arg);

#endif
