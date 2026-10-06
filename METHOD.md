# How the 49,409-HP witness was found

The search began with a complete, original-game-verified route at 49,401 HP.
We represented the route as typed interaction steps, then used the native C++
engine to rebuild walking, doors, collections, purchases, and repairs after
route edits. The search evaluated edits from the unchanged initial state and
retained only complete True Ending routes with normal healing.

The final route-level LNS search found a legal timing change on MT49. We leave
the corner guard at `(5,2)` until after the fake-boss event. That event removes
the guard and supplies a red key, saving 8 HP while giving up 100 gold. The
route still funds fifteen shop purchases and finishes with 538 gold. The
search retained the 49,409-HP candidate and its 4,438 original game inputs.

The verification boundary is deliberate:

1. The typed native replay passed continuously from one initialization with all
   4,438 actions and ended in `特性版-TE（生命）` at 49,409 HP.
2. A fresh replay in the original game passed with the same packed route and no
   replay errors.
3. The promotion receipt recorded the exact input match and no leaderboard
   submission.

The checked record and the target are separate from these checks. This package
contains evidence for one verified witness, not a proof that it is globally
best. The native world and route input are derived from the pinned public game
snapshot; see `NOTICE.md` before redistributing them.
