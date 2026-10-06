# Attribution and redistribution notice

This is a curated research snapshot. It does not include a project-wide
software license.

The simulator adapts the route model from
[`luohuan02/mota-web-explorer`](https://github.com/luohuan02/mota-web-explorer),
pinned in the original work at commit `fa7181dfd243c5204ddcbdc9c244354e3e86388b`.
The inspected upstream checkout did not contain a license file, so this release
makes no new license grant for code or data derived from that implementation.

`cpp/third_party/nlohmann` contains nlohmann/json 3.12.0 under its MIT license.
The selected Boost headers are accompanied by Boost's license in
`cpp/third_party/LICENSE_1_0.txt`.

`world/` and the native input preserve data and event-function strings exported
from the public [H5 Mota tower-51 page](https://h5mota.com/games/51/). The original game and its assets remain
owned by their respective authors. The packed route is provided for research
and replay; check the game's terms and obtain any permission needed before
redistributing derived game data or hosting a public service.
