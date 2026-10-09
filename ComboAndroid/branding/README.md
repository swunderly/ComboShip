# Combo Ship artwork

The app uses a stylized blue ocarina based on the user's supplied reference. The artwork was generated with the built-in imagegen tool, without the CLI fallback.

Final asset: [`combo_ocarina.png`](../app/src/main/res/drawable-nodpi/combo_ocarina.png). This is the original 1254 x 1254 RGBA output, with its transparency preserved. Android scales it when drawing; no generated bitmap is referenced outside this source tree.

The Android adaptive icon adds a dark navy background and a 20% foreground inset. The setup screen uses the transparent emblem directly. Layout follows the [Android adaptive-icon guidance](https://developer.android.com/develop/ui/compose/system/icon_design_adaptive).

## Generation prompt

```text
Use case: logo-brand.
Asset type: Android launcher icon foreground for an app called Combo Ship.
Input image: the attached blue ocarina is a subject and silhouette reference.
Primary request: create one beautifully stylized blue ocarina emblem based on this reference, suitable for a polished Android app icon. Preserve the recognizable diagonal teardrop body with its point facing upper right, mouthpiece pointing upper left, dark round finger holes, silver collar, and small golden three-triangle emblem on the collar.
Style: bold clean illustrated shapes with tasteful cel-shaded depth, cobalt and periwinkle ceramic, restrained icy highlights, dark navy outlines, a brushed silver collar and warm gold accent. Simplify reflections and surface texture so the silhouette and holes read at small icon sizes. No gritty texture, no photorealistic scratches.
Composition: square 1024 by 1024 canvas. Center the entire ocarina and keep it within the central 74 percent of the canvas, with balanced transparent padding all around. No part may touch the edge. Single object only.
Background: genuinely transparent alpha, no backdrop, no circular badge, no square tile, no floor, no drop shadow outside the silhouette.
Text: none. No lettering, watermark, frame, extra objects or game characters.
```
