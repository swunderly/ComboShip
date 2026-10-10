# 0.2.2 request audit

| Requested change | Implementation |
| --- | --- |
| MM Starting inventory should look like OoT, without a small top scroll region. | Stable grouped icon grid, faded unselected items, progressive tier selection and inline resource controls. No nested child panels in the inventory renderer. |
| Move randomizer difficulty into Gameplay rules. | One Gameplay rules destination for both games' supported logic rules. Removed the separate Difficulty sidebar and its search link. |
| Bremen count belongs only under Enhancements Difficulty. | Runtime-only classification, slider and search destination. Older saved counts migrate; obsolete logic fields normalize to zero. The actual Bremen Mask and Cucco Shack access requirements remain unchanged. |
| Move OoT Minigames into Enhancements Difficulty. | Routes the original native controls and callbacks into the common Difficulty page. |
| Camera belongs under Video & Audio. | Camera page and search results moved for both games, including OoT's original camera controls from the controller editor. Original callbacks and values are preserved. |

Validation and APK metadata are in [VALIDATION-0.2.2.md](VALIDATION-0.2.2.md). Historical 0.2.0 and 0.2.1 audits retain their original results; this audit supersedes their page locations and Bremen classification.
