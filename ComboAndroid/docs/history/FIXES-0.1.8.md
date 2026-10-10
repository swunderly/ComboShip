# Combo Ship 0.1.8 — invisible touch sticks

Install `ComboShip-Android-0.1.8-dev-arm64.apk` over the existing app. The installation identity and signing key are the same, so your ROMs, saves and settings remain in place.

The button layout stays where you put it. Both sticks are now invisible by default, including while held:

- Touch an empty spot anywhere on the left half and drag to move.
- Touch an empty spot anywhere on the right half and drag to use the right stick. It follows your existing camera/controller settings.
- Each stick starts at the point where that finger touches. You can use both together and press other buttons at the same time.
- Buttons take priority over the stick areas. Each stick stays with its original finger until you lift it, even if you cross the middle of the screen.

Hold **Menu → Control sticks** to choose **Invisible sticks (touch either side)**, **Visible fixed sticks**, or **Visible floating left stick**. Existing button positions, layout, opacity and size are preserved. If you previously hid the entire touch controller, choose **Show touch controls** first; that setting still controls the whole pad.

## Validation

- Android assembly and lint passed with zero errors.
- Installed over 0.1.7 in the Android 15 emulator and verified that the installed APK matches the delivered file.
- Passed 36 checks against the actual Android touch overlay using real multi-pointer MotionEvents and isolated test preferences. These cover simultaneous sticks, button priority, finger ownership, independent release, cancellation, menu suppression, lifecycle release, saved layouts and all three stick modes.
- Compared rendered overlay pixels before and during simultaneous stick input: neither stick appears while held.
- Visually verified the default button layout and new Control sticks dialog. Loaded an existing OoT save and moved Link using a drag that began away from the old stick position. Opened the native menu, selected a tab on the left and closed it using the right-side close button.
- Byte-verified that all eight native libraries and the support manifest match 0.1.7. Verified the APK signature, packaged runtime and ROM exclusion checks.
- Existing emulator saves and touch preferences remained byte-identical; restored and byte-verified the original game settings after testing. No crash was recorded.

These checks used an emulator. In-game camera response and the feel of simultaneous thumbs still need your physical-phone check.

## Phone check

1. Confirm the Move and Camera circles have disappeared while the other buttons stay in their usual places.
2. Start movement from several empty spots on the left, and use the right stick from several spots on the right. Test with your usual camera settings in both games.
3. Hold both sticks, press A or another button, then lift one thumb. The other stick should keep working. Lift both and confirm input stops.
4. Open and close Menu, then background and resume the app. Check for stuck movement and confirm menu controls remain tappable.
5. If desired, select a visible stick mode under **Hold Menu → Control sticks** and relaunch to check that your choice persists.

APK size: 45,057,247 bytes.

SHA-256: `68e493d408db42d088a0f469f035448d02fb88996be3809e875cbceb687cee92`
