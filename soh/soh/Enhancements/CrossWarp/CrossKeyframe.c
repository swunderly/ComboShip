#ifdef COMBO_BUILD
/**
 * @file c_keyframe.c
 *
 * This file implements a skeletal animation system supporting all of scale, rotation and translation on all joints. It
 * uses keyframe data and interpolates intermediate values via cubic Hermite splines.
 */
#include "global.h"
#include "CrossWarp.h"
#include "CrossKeyframe.h"
#define CB(x) ((x) * (x) * (x))
// MM z64math.h DEG_TO_BINANG: truncate through s32, so angles past 180 degrees wrap.
#define CW_DEG_TO_BINANG(degrees) ((s16)(s32)((degrees) * (0x8000 / 180.0f)))
#include "soh/ResourceManagerHelpers.h"

#define FMOD(x, mod) ((x) - ((s32)((x) * (1.0f / (mod))) * (f32)(mod)))

/**
 * @note Original name: cKF_FrameControl_zeroClera
 */
void CWFrameCtrl_Reset(CWFrameControl* frameCtrl) {
    frameCtrl->frameCount = 0.0f;
    frameCtrl->curTime = 0.0f;
    frameCtrl->speed = 0.0f;
    frameCtrl->end = 0.0f;
    frameCtrl->start = 0.0f;
    frameCtrl->animMode = KEYFRAME_ANIM_ONCE;
}

/**
 * @note Original name: cKF_FrameControl_ct
 */
void CWFrameCtrl_Init(CWFrameControl* frameCtrl) {
    CWFrameCtrl_Reset(frameCtrl);
}

/**
 * @note Original name: cKF_FrameControl_setFrame
 */
void CWFrameCtrl_SetProperties(CWFrameControl* frameCtrl, f32 startTime, f32 endTime, f32 frameCount, f32 t, f32 speed,
                               s32 animMode) {
    frameCtrl->start = startTime;
    frameCtrl->end = (endTime < 1.0f) ? frameCount : endTime;
    frameCtrl->frameCount = frameCount;
    frameCtrl->speed = speed;
    frameCtrl->curTime = t;
    frameCtrl->animMode = animMode;
}

/**
 * @note Original name: cKF_FrameControl_passCheck
 */
s32 CWFrameCtrl_PassCheck(CWFrameControl* frameCtrl, f32 t, f32* remainingTime) {
    f32 curTime;
    f32 speed;

    *remainingTime = 0.0f;
    curTime = frameCtrl->curTime;

    if (t == curTime) {
        return false;
    }

    speed = ((frameCtrl->start < frameCtrl->end) ? frameCtrl->speed : -frameCtrl->speed) * (30.0f / 20.0f);

    if (((speed >= 0.0f) && (curTime < t) && (t <= curTime + speed)) ||
        ((speed < 0.0f) && (t < curTime) && (curTime + speed <= t))) {

        *remainingTime = curTime + speed - t;
        return true;
    }
    return false;
}

/**
 * Updates a CWFrameControl structure whose mode is KEYFRAME_ANIM_ONCE
 *
 * @note Original name: cKF_FrameControl_stop_proc
 */
s32 CWFrameCtrl_UpdateOnce(CWFrameControl* frameCtrl) {
    f32 remainingTime;

    if (frameCtrl->curTime == frameCtrl->end) {
        // If the current time is at the end time, the animation is done.
        return KEYFRAME_DONE_ONCE;
    }
    if (CWFrameCtrl_PassCheck(frameCtrl, frameCtrl->end, &remainingTime)) {
        frameCtrl->curTime = frameCtrl->end;
        return KEYFRAME_DONE_ONCE;
    }
    if (CWFrameCtrl_PassCheck(frameCtrl, frameCtrl->start, &remainingTime)) {
        frameCtrl->curTime = frameCtrl->end;
        return KEYFRAME_DONE_ONCE;
    }
    return KEYFRAME_NOT_DONE;
}

/**
 * Updates a CWFrameControl structure whose mode is KEYFRAME_ANIM_LOOP
 *
 * @note Original name: cKF_FrameControl_repeat_proc
 */
s32 CWFrameCtrl_UpdateLoop(CWFrameControl* frameCtrl) {
    f32 remainingTime;

    if (CWFrameCtrl_PassCheck(frameCtrl, frameCtrl->end, &remainingTime)) {
        frameCtrl->curTime = frameCtrl->start + remainingTime;
        return KEYFRAME_DONE_LOOP;
    }
    if (CWFrameCtrl_PassCheck(frameCtrl, frameCtrl->start, &remainingTime)) {
        frameCtrl->curTime = frameCtrl->end + remainingTime;
        return KEYFRAME_DONE_LOOP;
    }
    return KEYFRAME_NOT_DONE;
}

/**
 * Check if the animation has finished playing and update the animation frame number.
 *
 * @note Original name: cKF_FrameControl_play
 */
s32 CWFrameCtrl_Update(CWFrameControl* frameCtrl) {
    s32 result;
    f32 speed;

    // Check if the animation is done, possibly updating curTime
    if (frameCtrl->animMode == KEYFRAME_ANIM_ONCE) {
        result = CWFrameCtrl_UpdateOnce(frameCtrl);
    } else {
        result = CWFrameCtrl_UpdateLoop(frameCtrl);
    }

    if (result == KEYFRAME_NOT_DONE) {
        // Animation is not done, step curTime by (speed * (30.0f / 20.0f)), adjusting the sign if the animation is
        // playing in reverse (end <= start)
        speed = (frameCtrl->start < frameCtrl->end) ? frameCtrl->speed : -frameCtrl->speed;
        frameCtrl->curTime = frameCtrl->curTime + speed * (30.0f / 20.0f);
    }

    // Adjust time for looping
    if (frameCtrl->curTime < 1.0f) {
        // Wrap from the start to the end of the animation
        frameCtrl->curTime = (frameCtrl->curTime - 1.0f) + frameCtrl->frameCount;
    } else if (frameCtrl->frameCount < frameCtrl->curTime) {
        // Wrap from the end to the start of the animation
        frameCtrl->curTime = (frameCtrl->curTime - frameCtrl->frameCount) + 1.0f;
    }

    return result;
}

/**
 * @note Original name unknown
 */
void CWKeyframe_ResetFlex(CWKFSkelAnimeFlex* kfSkelAnime) {
    kfSkelAnime->skeleton = NULL;
    kfSkelAnime->animation = NULL;
    kfSkelAnime->jointTable = NULL;
    kfSkelAnime->transformCallbacks = NULL;
    kfSkelAnime->morphTable = NULL;
    kfSkelAnime->morphFrames = 0.0f;
}

/**
 * Initializes a flex-type keyframe skeleton. The initial animation type is KEYFRAME_ANIM_ONCE.
 *
 * @param skeleton Skeleton to animate
 * @param animation Initial animation to use
 * @param jointTable Joint table to store limb transformations. Should have enough space to store a root translation
 *                   plus a limb rotation for all limbs in the skeleton.
 * @param morphTable Joint table to store morph interpolation values. Should have enough space to store a root
 *                   translation plus a limb rotation for all limbs in the skeleton.
 * @param transformCallbacks Array of limb transformation callbacks that will be called when drawing a particular limb.
 *                           The limb data contains the index to select which callback to run.
 *
 * @note Original name unknown
 */
void CWKeyframe_InitFlex(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameFlexSkeleton* skeleton,
                         CWKeyFrameAnimation* animation, Vec3s* jointTable, Vec3s* morphTable,
                         KeyframeTransformCallback* transformCallbacks) {
    if (ResourceMgr_OTRSigCheck(skeleton)) {
        skeleton = (CWKeyFrameSkeleton*)Combo_SOHWarpResource(skeleton);
    }
    if (ResourceMgr_OTRSigCheck(animation)) {
        animation = (CWKeyFrameAnimation*)Combo_SOHWarpResource(animation);
    }

    CWKeyframe_ResetFlex(kfSkelAnime);
    CWFrameCtrl_Init(&kfSkelAnime->frameCtrl);
    kfSkelAnime->skeleton = SEGMENTED_TO_VIRTUAL(skeleton);
    kfSkelAnime->animation = SEGMENTED_TO_VIRTUAL(animation);
    kfSkelAnime->jointTable = jointTable;
    kfSkelAnime->morphTable = morphTable;
    kfSkelAnime->transformCallbacks = transformCallbacks;
}

/**
 * Destroys a flex-type keyframe skeleton.
 *
 * @note Original name unknown
 */
void CWKeyframe_DestroyFlex(CWKFSkelAnimeFlex* kfSkelAnime) {
}

void CWKeyframe_FlexChangeAnim(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameFlexSkeleton* skeleton,
                               CWKeyFrameAnimation* animation, f32 startTime, f32 endTime, f32 t, f32 speed,
                               f32 morphFrames, s32 animMode);

/**
 * Immediately changes to an animation that plays once from start to end at the default speed.
 *
 * @param animation Animation data to switch to
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexPlayOnce(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameAnimation* animation) {
    CWKeyframe_FlexChangeAnim(kfSkelAnime, kfSkelAnime->skeleton, animation, 1.0f,
                              ((CWKeyFrameAnimation*)SEGMENTED_TO_VIRTUAL(animation))->frameCount, 1.0f, 1.0f, 0.0f,
                              KEYFRAME_ANIM_ONCE);
}

/**
 * Immediately changes to an animation that plays once from start to end at the specified speed.
 *
 * @param animation Animation data to switch to
 * @param speed Playback speed
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexPlayOnceSetSpeed(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameAnimation* animation, f32 speed) {
    CWKeyframe_FlexChangeAnim(kfSkelAnime, kfSkelAnime->skeleton, animation, 1.0f,
                              ((CWKeyFrameAnimation*)SEGMENTED_TO_VIRTUAL(animation))->frameCount, 1.0f, speed, 0.0f,
                              KEYFRAME_ANIM_ONCE);
}

/**
 * Smoothly transitions to an animation that plays once from start to end at the default speed, specifying the number of
 * frames for the transition.
 *
 * @param animation Animation data to switch to
 * @param morphFrames Number of frames to take to transition from the previous pose to the new animation. Positive morph
 *                    frames morph from the current pose to the start pose of the new animation, then start the new
 *                    animation. Negative morph frames start the new animation immediately, modified by the pose
 *                    immediately before the animation change.
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexMorphToPlayOnce(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameAnimation* animation, f32 morphFrames) {
    CWKeyframe_FlexChangeAnim(kfSkelAnime, kfSkelAnime->skeleton, animation, 1.0f,
                              ((CWKeyFrameAnimation*)SEGMENTED_TO_VIRTUAL(animation))->frameCount, 1.0f, 1.0f,
                              morphFrames, KEYFRAME_ANIM_ONCE);
}

/**
 * Immediately changes to an animation that loops over start to end at the default speed.
 *
 * @param animation Animation data to switch to
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexPlayLoop(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameAnimation* animation) {
    CWKeyframe_FlexChangeAnim(kfSkelAnime, kfSkelAnime->skeleton, animation, 1.0f,
                              ((CWKeyFrameAnimation*)SEGMENTED_TO_VIRTUAL(animation))->frameCount, 1.0f, 1.0f, 0.0f,
                              KEYFRAME_ANIM_LOOP);
}

/**
 * Immediately changes to an animation that loops over start to end at the specified speed.
 *
 * @param animation Animation data to switch to
 * @param speed Playback speed
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexPlayLoopSetSpeed(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameAnimation* animation, f32 speed) {
    CWKeyframe_FlexChangeAnim(kfSkelAnime, kfSkelAnime->skeleton, animation, 1.0f,
                              ((CWKeyFrameAnimation*)SEGMENTED_TO_VIRTUAL(animation))->frameCount, 1.0f, speed, 0.0f,
                              KEYFRAME_ANIM_LOOP);
}

/**
 * Smoothly transitions to an animation that loops over start to end at the default speed, specifying the number of
 * frames for the transition.
 *
 * @param animation Animation data to switch to
 * @param morphFrames Number of frames to take to transition from the previous pose to the new animation. Positive morph
 *                    frames morph from the current pose to the start pose of the new animation, then start the new
 *                    animation. Negative morph frames start the new animation immediately, modified by the pose
 *                    immediately before the animation change.
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexMorphToPlayLoop(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameAnimation* animation, f32 morphFrames) {
    CWKeyframe_FlexChangeAnim(kfSkelAnime, kfSkelAnime->skeleton, animation, 1.0f,
                              ((CWKeyFrameAnimation*)SEGMENTED_TO_VIRTUAL(animation))->frameCount, 1.0f, 1.0f,
                              morphFrames, KEYFRAME_ANIM_LOOP);
}

/**
 * General way to set a new animation for flex-type skeletons, allowing choice of playback speed, start/end loop points,
 * start time, play mode, and number of transition frames.
 *
 * Time parameters are valid from 0 to the last frame of the animation.
 *
 * @param skeleton Skeleton that will be animated
 * @param animation Animation data to switch to
 * @param startTime Loop start time
 * @param endTime Loop end time, 0 indicates to use the animation length
 * @param t Playback start time
 * @param speed Playback speed
 * @param morphFrames Number of frames to take to transition from the previous pose to the new animation. Positive morph
 *                    frames morph from the current pose to the start pose of the new animation, then start the new
 *                    animation. Negative morph frames start the new animation immediately, modified by the pose
 *                    immediately before the animation change.
 * @param animMode Animation play mode, see CWKeyFrameAnimMode enum
 *
 * @see CWKeyFrameAnimMode
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexChangeAnim(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameFlexSkeleton* skeleton,
                               CWKeyFrameAnimation* animation, f32 startTime, f32 endTime, f32 t, f32 speed,
                               f32 morphFrames, s32 animMode) {
    if (ResourceMgr_OTRSigCheck(skeleton)) {
        skeleton = Combo_SOHWarpResource(skeleton);
    }
    if (ResourceMgr_OTRSigCheck(animation)) {
        animation = Combo_SOHWarpResource(animation);
        // Overwrite the end time after fetching the animation data as the parent caller most likely
        // passed in garbage data from the "fake" animation pointer (casted from resource string)
        endTime = animation->frameCount;
    }

    kfSkelAnime->morphFrames = morphFrames;

    if (kfSkelAnime->skeleton != skeleton) {
        kfSkelAnime->skeleton = SEGMENTED_TO_VIRTUAL(skeleton);
    }
    kfSkelAnime->animation = SEGMENTED_TO_VIRTUAL(animation);

    CWFrameCtrl_SetProperties(&kfSkelAnime->frameCtrl, startTime, endTime, kfSkelAnime->animation->frameCount, t, speed,
                              animMode);
}

/**
 * Switches to a new animation without changing any of the playback parameters.
 *
 * @param animation The animation to switch to
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexChangeAnimQuick(CWKFSkelAnimeFlex* kfSkelAnime, CWKeyFrameAnimation* animation) {
    kfSkelAnime->animation = SEGMENTED_TO_VIRTUAL(animation);
    kfSkelAnime->frameCtrl.frameCount = kfSkelAnime->animation->frameCount;
}

/**
 * Compute a value on the cubic Hermite spline x(t) at a time `t` in the unit interval [0, 1]
 *
 * @param t Time parameter at which to sample the curve
 * @param delta Scales the rates of change of the curve endpoints v0 and v1
 * @param x0 Value on the curve at t=0
 * @param x1 Value on the curve at t=1
 * @param v0 Rate of change of the curve at t=0
 * @param v1 Rate of change of the curve at t=1
 *
 * @note Original name: cKF_HermitCalc
 */
f32 CWKeyframe_Interpolate(f32 t, f32 delta, f32 x0, f32 x1, f32 v0, f32 v1) {
    f32 px1 = 3.0f * SQ(t) - 2.0f * CB(t);
    f32 px0 = 1.0f - px1;
    f32 pv0 = CB(t) - 2.0f * SQ(t) + t;
    f32 pv1 = CB(t) - SQ(t);

    return px0 * x0 + px1 * x1 + (pv0 * v0 + pv1 * v1) * delta;
}

/**
 * Computes an output value at time `t` based on interpolation of the provided keyframes.
 * Interpolation between keyframes is performed via cubic Hermite splines.
 *
 * @param kfStart Index of the first keyframe to consider
 * @param kfNum Number of keyframes following the first keyframe to consider
 * @param keyFrames Array of all keyframes
 * @param t Time at which to sample the interpolated curve
 *
 * @return The interpolated value
 *
 * @note Original name: cKF_KeyCalc
 */
s16 CWKeyframe_KeyCalc(s16 kfStart, s16 kfNum, KeyFrame* keyFrames, f32 t) {
    KeyFrame* keyFramesOffset = &keyFrames[kfStart];
    f32 delta;
    s16 kf1;
    s16 kf2;

    if (t <= keyFramesOffset->frame) {
        return keyFramesOffset->value;
    }
    if (keyFramesOffset[kfNum - 1].frame <= t) {
        return keyFramesOffset[kfNum - 1].value;
    }

    kf1 = 0;
    kf2 = 1;
    while (true) {
        // Search for the keyframes kf1 and kf2 such that kf1.frame <= t < kf2.frame
        if (t < keyFramesOffset[kf2].frame) {
            delta = keyFramesOffset[kf2].frame - keyFramesOffset[kf1].frame;

            if (!IS_ZERO(delta)) {
                // Between two keyframes, interpolate a value and round to nearest integer
                return nearbyint(CWKeyframe_Interpolate((t - keyFramesOffset[kf1].frame) / delta, delta * (1.0f / 30),
                                                        keyFramesOffset[kf1].value, keyFramesOffset[kf2].value,
                                                        keyFramesOffset[kf1].velocity, keyFramesOffset[kf2].velocity));
            } else {
                // Close enough to a keyframe, take the specified value with no interpolation
                return keyFramesOffset[kf1].value;
            }
        }
        kf1++;
        kf2++;
    }
}

/**
 * Morph interpolator for rotation.
 *
 * Linearly interpolates between `rot1` and `rot2` with weight `t`, choosing either signed angles or unsigned angles
 * based on whichever choice has the smaller distance between the two.
 *
 * @note Original name: cKF_SkeletonInfo_subRotInterpolation
 */
void CWKeyframe_MorphInterpolateRotation(f32 t, s16* out, s16 rot1, s16 rot2) {
    u16 urot1 = rot1;
    s32 pad;
    u16 urot2 = rot2;
    f32 rot1f = rot1;
    f32 signedDiff = rot2 - rot1f;
    f32 urot1f = urot1;
    f32 unsignedDiff = urot2 - urot1f;

    if (fabsf(signedDiff) < fabsf(unsignedDiff)) {
        *out = rot1f + signedDiff * t;
    } else {
        *out = urot1f + unsignedDiff * t;
    }
}

/**
 * Morph interpolator for translation and scale.
 *
 * Linearly interpolates between `jointData` and `morphData` with weight `t`, storing the result back into `jointData`.
 *
 * @note Original name: cKF_SkeletonInfo_morphST
 */
void CWKeyframe_MorphInterpolateLinear(s16* jointData, s16* morphData, f32 t) {
    s32 i;

    for (i = 0; i < 3; i++) {
        if (*jointData != *morphData) {
            f32 f1 = *jointData;
            f32 f2 = *morphData;
            *jointData = f1 + (f2 - f1) * t;
        }
        jointData++;
        morphData++;
    }
}

/**
 * Apply morph interpolation for the provided skeleton. Morph interpolation seeks to provide interpolation between
 * a previous animation and a new animation over a fixed period of time (morphFrames)
 *
 * @note Original name unknown
 */
void CWKeyframe_FlexMorphInterpolation(CWKFSkelAnimeFlex* kfSkelAnime) {
    Vec3s* jointTable = kfSkelAnime->jointTable;
    Vec3s* morphTable = kfSkelAnime->morphTable;
    f32 t = 1.0f / fabsf(kfSkelAnime->morphFrames);
    s32 limbIndex;

    for (limbIndex = 0; limbIndex < kfSkelAnime->skeleton->limbCount; limbIndex++) {
        Vec3s frameRot;
        Vec3s morphRot;

        // Interpolate scale
        CWKeyframe_MorphInterpolateLinear((s16*)jointTable, (s16*)morphTable, t);
        jointTable++;
        morphTable++;

        // Read rotation
        frameRot.x = jointTable->x;
        frameRot.y = jointTable->y;
        frameRot.z = jointTable->z;

        morphRot.x = morphTable->x;
        morphRot.y = morphTable->y;
        morphRot.z = morphTable->z;

        // Interpolate rotation
        if (frameRot.x != morphRot.x || frameRot.y != morphRot.y || frameRot.z != morphRot.z) {
            Vec3s frameRotInv;
            f32 norm1;
            f32 norm2;

            frameRotInv.x = 0x7FFF + frameRot.x;
            frameRotInv.y = 0x7FFF - frameRot.y;
            frameRotInv.z = 0x7FFF + frameRot.z;

            // Compute L1 norms
            norm1 = fabsf((f32)morphRot.x - frameRot.x) + fabsf((f32)morphRot.y - frameRot.y) +
                    fabsf((f32)morphRot.z - frameRot.z);
            norm2 = fabsf((f32)morphRot.x - frameRotInv.x) + fabsf((f32)morphRot.y - frameRotInv.y) +
                    fabsf((f32)morphRot.z - frameRotInv.z);

            if (norm1 < norm2) {
                // frameRot is closer to morphRot than frameRotInv, interpolate between these two
                CWKeyframe_MorphInterpolateRotation(t, &jointTable->x, frameRot.x, morphRot.x);
                CWKeyframe_MorphInterpolateRotation(t, &jointTable->y, frameRot.y, morphRot.y);
                CWKeyframe_MorphInterpolateRotation(t, &jointTable->z, frameRot.z, morphRot.z);
            } else {
                // frameRotInv is closer to morphRot than frameRot, interpolate between these two
                CWKeyframe_MorphInterpolateRotation(t, &jointTable->x, frameRotInv.x, morphRot.x);
                CWKeyframe_MorphInterpolateRotation(t, &jointTable->y, frameRotInv.y, morphRot.y);
                CWKeyframe_MorphInterpolateRotation(t, &jointTable->z, frameRotInv.z, morphRot.z);
            }
        }
        morphTable++;
        jointTable++;

        // Interpolate translation
        CWKeyframe_MorphInterpolateLinear((s16*)jointTable, (s16*)morphTable, t);
        jointTable++;
        morphTable++;
    }
}

/**
 * Advances the current animation and updates all frame tables for flex-type keyframe skeletons.
 *
 * @return s32
 *  KEYFRAME_NOT_DONE  : If the animation is still playing
 *  KEYFRAME_DONE_ONCE : If the animation was set to play once and has finished playing
 *  KEYFRAME_DONE_LOOP : If the animation was set to play in a loop and has finished a loop
 *
 * @note Original name unknown
 */
s32 CWKeyframe_UpdateFlex(CWKFSkelAnimeFlex* kfSkelAnime) {
    s32 limbIndex;
    s32 pad[2];
    u16* bitFlags;
    s16* outputValues;
    s32 kfn = 0;
    s32 fixedValueIndex = 0;
    s32 kfStart = 0;
    s16* fixedValues;
    KeyFrame* keyFrames;
    s16* kfNums;
    u32 bit;
    s32 i;
    s32 j;

    // If there are morph frames to process, use the morph table
    if (kfSkelAnime->morphFrames != 0.0f) {
        outputValues = (s16*)kfSkelAnime->morphTable;
    } else {
        outputValues = (s16*)kfSkelAnime->jointTable;
    }

    // Array of preset values to pull from
    fixedValues = SEGMENTED_TO_VIRTUAL(kfSkelAnime->animation->fixedValues);

    // Array of number of keyframes belonging to each limb
    kfNums = SEGMENTED_TO_VIRTUAL(kfSkelAnime->animation->kfNums);

    // Array of keyframes, ordered by frame number
    keyFrames = SEGMENTED_TO_VIRTUAL(kfSkelAnime->animation->keyFrames);

    // The bitFlags array indicates whether a transformation on an axis should interpolate a value (if the bit is set)
    // or pull from an array of constant values (if the bit is unset) if the transformation on an axis does not change
    // during the animtion. For the flex-type keyframe skeletons the flags for each limb are contained in 16 bits.
    // The bitFlags layout for the flex-type keyframe skeletons is the same for all limbs:
    //  [8] : Scale x
    //  [7] : Scale y
    //  [6] : Scale z
    //  [5] : Rotate x
    //  [4] : Rotate y
    //  [3] : Rotate z
    //  [2] : Translate x
    //  [1] : Translate y
    //  [0] : Translate z
    bitFlags = SEGMENTED_TO_VIRTUAL(kfSkelAnime->animation->bitFlags.flex);

    // For each limb
    for (limbIndex = 0; limbIndex < kfSkelAnime->skeleton->limbCount; limbIndex++) {
        bit = 1 << (3 * 3 - 1);

        // 3 iter (scale, rotate, translate)
        for (i = 0; i < 3; i++) {
            // 3 iter (x, y, z)
            for (j = 0; j < 3; j++) {
                if (bitFlags[limbIndex] & bit) {
                    // If the bit is set, interpolate with keyframes
                    *outputValues = CWKeyframe_KeyCalc(kfStart, kfNums[kfn], keyFrames, kfSkelAnime->frameCtrl.curTime);
                    kfStart += kfNums[kfn];
                    kfn++;
                } else {
                    // If the bit is not set, pull from preset values
                    *outputValues = fixedValues[fixedValueIndex];
                    fixedValueIndex++;
                }
                bit >>= 1;

                if (i == 1) {
                    // For rotations, translate angle value from tenths of a degree to binang
                    *outputValues = CW_DEG_TO_BINANG(FMOD(*outputValues * 0.1f, 360));
                }
                outputValues++;
            }
        }
    }

    if (IS_ZERO(kfSkelAnime->morphFrames)) {
        // No morph, just play the animation
        return CWFrameCtrl_Update(&kfSkelAnime->frameCtrl);
    } else if (kfSkelAnime->morphFrames > 0.0f) {
        // Morph to first frame before playing the animation proper
        CWKeyframe_FlexMorphInterpolation(kfSkelAnime);
        kfSkelAnime->morphFrames -= 1.0f;
        if (kfSkelAnime->morphFrames <= 0.0f) {
            kfSkelAnime->morphFrames = 0.0f;
        }
        return KEYFRAME_NOT_DONE;
    } else {
        // Play the animation immediately, morphing as it plays
        CWKeyframe_FlexMorphInterpolation(kfSkelAnime);
        kfSkelAnime->morphFrames += 1.0f;
        if (kfSkelAnime->morphFrames >= 0.0f) {
            kfSkelAnime->morphFrames = 0.0f;
        }
        return CWFrameCtrl_Update(&kfSkelAnime->frameCtrl);
    }
}

/**
 * Draws the limb specified by `limbIndex` of type `CWKeyFrameFlexLimb` belonging to a flex-type keyframe skeleton to
 * the display buffer specified by the limb's drawFlags.
 *
 * @param limbIndex Pointer to the index of the limb to draw
 * @param overrideKeyframeDraw Callback for before submitting the limb to be drawn. The matrix state will not include
 *                             the transformation for the current limb.
 * @param postKeyframeDraw Callback for after submitting the limb to be drawn. The matrix state will include
 *                         the transformation for the current limb.
 * @param arg An arbitrary argument to pass to the callbacks.
 * @param mtxStack Matrix stack for limb transformations. Should have enough room for one matrix per limb.
 *
 * @note Original name unknown
 */
void CWKeyframe_DrawFlexLimb(PlayState* play, CWKFSkelAnimeFlex* kfSkelAnime, s32* limbIndex,
                             CWOverrideKeyframeDrawScaled overrideKeyframeDraw,
                             CWPostKeyframeDrawScaled postKeyframeDraw, void* arg, Mtx** mtxStack) {
    CWKeyFrameFlexLimb* limb = SEGMENTED_TO_VIRTUAL(kfSkelAnime->skeleton->limbs);
    s32 i;
    Gfx* newDList;
    Gfx* limbDList;
    u8 drawFlags;
    Vec3f scale;
    Vec3s rot;
    Vec3f pos;
    Vec3s* jointData;

    OPEN_DISPS(play->state.gfxCtx);

    limb += *limbIndex;
    jointData = &kfSkelAnime->jointTable[*limbIndex * 3];

    scale.x = jointData->x * 0.01f;
    scale.y = jointData->y * 0.01f;
    scale.z = jointData->z * 0.01f;

    jointData++;

    rot.x = jointData->x;
    rot.y = jointData->y;
    rot.z = jointData->z;

    jointData++;

    pos.x = jointData->x;
    pos.y = jointData->y;
    pos.z = jointData->z;

    Matrix_Push();

    newDList = limbDList = limb->dList;
    drawFlags = limb->drawFlags;

    if (overrideKeyframeDraw == NULL ||
        (overrideKeyframeDraw != NULL &&
         overrideKeyframeDraw(play, kfSkelAnime, *limbIndex, &newDList, &drawFlags, arg, &scale, &rot, &pos))) {
        if ((kfSkelAnime->transformCallbacks == NULL) || (limb->callbackIndex == KF_CALLBACK_INDEX_NONE) ||
            (kfSkelAnime->transformCallbacks[limb->callbackIndex] == NULL) ||
            kfSkelAnime->transformCallbacks[limb->callbackIndex](play, kfSkelAnime, *limbIndex, &newDList, &drawFlags,
                                                                 arg)) {

            Matrix_TranslateRotateZYX(&pos, &rot);

            if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
                Matrix_Scale(scale.x, scale.y, scale.z, MTXMODE_APPLY);
            }

            if (newDList != NULL) {
                Matrix_ToMtx(*mtxStack, __FILE__, __LINE__);

                if (drawFlags & KEYFRAME_DRAW_XLU) {
                    gSPMatrix(POLY_XLU_DISP++, *mtxStack, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                    gSPDisplayList(POLY_XLU_DISP++, newDList);
                } else {
                    gSPMatrix(POLY_OPA_DISP++, *mtxStack, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                    gSPDisplayList(POLY_OPA_DISP++, newDList);
                }

                (*mtxStack)++;
            } else if (limbDList != NULL) {
                gSPMatrix(POLY_OPA_DISP++, *mtxStack, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
                Matrix_ToMtx(*mtxStack, __FILE__, __LINE__);

                (*mtxStack)++;
            }
        }
    }

    if (postKeyframeDraw != NULL) {
        postKeyframeDraw(play, kfSkelAnime, *limbIndex, &newDList, &drawFlags, arg, &scale, &rot, &pos);
    }

    (*limbIndex)++;

    for (i = 0; i < limb->numChildren; i++) {
        CWKeyframe_DrawFlexLimb(play, kfSkelAnime, limbIndex, overrideKeyframeDraw, postKeyframeDraw, arg, mtxStack);
    }

    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draws a flex-type keyframe skeleton in its current pose.
 *
 * @param mtxStack Matrix stack for limb transformations. Should have enough room for one matrix per limb.
 * @param overrideKeyframeDraw Callback for before submitting the limb to be drawn. The matrix state will not include
 *                             the transformation for the current limb.
 * @param postKeyframeDraw Callback for after submitting the limb to be drawn. The matrix state will include
 *                         the transformation for the current limb.
 * @param arg An arbitrary argument to pass to the callbacks.
 *
 * @note Original name unknown
 */
void CWKeyframe_DrawFlex(PlayState* play, CWKFSkelAnimeFlex* kfSkelAnime, Mtx* mtxStack,
                         CWOverrideKeyframeDrawScaled overrideKeyframeDraw, CWPostKeyframeDrawScaled postKeyframeDraw,
                         void* arg) {
    s32 limbIndex;

    if (mtxStack == NULL) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x0D, mtxStack);
    gSPSegment(POLY_XLU_DISP++, 0x0D, mtxStack);

    limbIndex = 0;
    CWKeyframe_DrawFlexLimb(play, kfSkelAnime, &limbIndex, overrideKeyframeDraw, postKeyframeDraw, arg, &mtxStack);

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * @note Original name: cKF_SkeletonInfo_R_zeroClear
 */

#endif
