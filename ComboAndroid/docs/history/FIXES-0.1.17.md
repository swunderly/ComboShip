# Combo Ship 0.1.17 — MM seen-item visibility

MM's check tracker previously tested whether an item's position projected into the camera, but did not test obstacles. A freestanding reward could therefore become Seen through a Deku Palace maze wall.

The visual observation gate now uses the drawn model's position, checks the current viewport and near/far clipping, rejects zero-scale hidden models, waits until scene transitions and pause overlays are finished, and tests the camera-to-item path against static and moving collision geometry. Native rewards, progressive tiers, trap disguises and successfully rendered foreign rewards use this gate. Explicit item-reveal dialogue still records knowledge separately.

Install this APK over the existing app. Saves, seed placements, collected checks and previous observations are preserved. Previously saved false observations cannot be distinguished from legitimate ones, so this patch does not erase them automatically. The larger next-patch backlog remains deferred.

## Check on your phone

Use a check that has not already been marked Seen. Enter or look toward a maze section while its reward is hidden by a wall: the reward should remain unknown. Bring the item into a clear view: its displayed reward should appear as Seen without increasing the collected count. Save and reload to confirm the observation is retained.

This is conservative camera/frustum and collision-based visibility, not a GPU pixel-occlusion query. Non-colliding decorative meshes and partial visibility at model edges are not fully modeled.
