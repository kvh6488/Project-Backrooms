# The Wilderness — Overworld / Underworld Integration Design

> **Living document** — the design for merging the wilderness survival game into Project Backrooms as a single, coherent title. Last updated: **04-09-2026**.
>
> This document does **not** replace [roadmap.md](roadmap.md). It defines the combined game's structure and lists, at the end, exactly which existing roadmap items it changes. Phase numbering and re-planning will follow separately.

---

## 1. Premise

The year is **2100**. You live alone with your wife in the wilderness — hunting, fishing, foraging, working the land. It is a beautiful, quiet, self-sufficient life, and the world above is **pristine**. There is no visible apocalypse, no ruin, no contamination on the surface. Whatever happened to the old world happened a long time ago and left the land clean.

**One day your wife goes missing.** That is the inciting incident, and the game starts there.

You have to keep surviving — the land does not care that she's gone — but now you have a second reason to range further than your camp: the seasons force you to move, resources force you to move, and so does the possibility that she is out there somewhere.

Eventually, you stumble across an entrance to something underneath.

---

## 2. The Two Worlds

The game is **hub and spoke**. The wilderness is the persistent hub; the maze is the dangerous spoke you dive into and retreat from.

| | **The Overworld** (Wilderness) | **The Underworld** (The Maze / Bunker Network) |
|---|---|---|
| **Tone** | Beautiful, peaceful, pristine. Never corrupted. | Liminal, hostile, claustrophobic, psychological horror. |
| **Threat** | The elements, starvation, injury, weather, predators. | Radiation, mobs, shifting layout, darkness, escalating waves. |
| **Time** | **Cyclical** — seasons wheel round, year after year. | **Monotonic** — only ever gets worse, forever. |
| **Tech level** | Primitive/classical bushcraft. Flint, bone, hide, cordage, wood, clay. | Old-world technology. Batteries, wiring, chemicals, electronics, salvage. |
| **Resources** | Renewable but slow. Process-heavy (drying, curing, tanning, smoking). Abundant, low value. | Scarcer and harder-won, but far more powerful. Some renewable — **but only producible down there** (chemical lab, hydroponics). |
| **Food** | The whole point of the surface — hunting, fishing, foraging, farming. | Almost none. Mushrooms only, and they won't sustain you. You eat what you carried in. |
| **Light** | Fire. Renewable, trivial, barely needed. | Batteries, made only in the maze's chemical lab, and **light is life**. |
| **Space** | Static. Terrain never moves. | Shifting. The maze regenerates and rearranges. |
| **Purpose** | Where you live. | Where you go to get what living requires. |

**Hard design rule: the surface never degrades.** No contamination, no spreading stain, no dying forest. The horror stays entirely underground. The wilderness looks safe forever — which is arguably scarier, and it protects the thing that makes the survival half worth playing.

---

## 3. Core Loop & Objective

**The core objective is to survive as many days as possible.** Everything else serves that.

1. **Survive on the surface.** Hunt, fish, forage, build shelter, manage hunger/thirst/temperature, prepare for the season turning.
2. **Range outward.** Seasons and resource depletion push you to keep moving and exploring rather than turtling in one spot.
3. **Prepare.** A dive has to be provisioned from the surface — food, water, light, gear. You cannot live off the maze.
4. **Find an entrance.** (Not before Day 20 — see §7.)
5. **Dive.** Take what you can carry, get what the surface cannot give you, get out before your light, food, or luck runs out.
6. **Return and reinvest.** Bring old-world tech up, and use it to survive longer above.
7. **Repeat, deeper.** The maze gets permanently worse every day. Your gear has to keep pace.

**Why you must eventually go down:** surviving alone in the wilderness is peaceful and far safer than the maze, but it is *notoriously* hard. The intent is that players will realise after a few runs that pure surface survival plateaus — **you almost have to go down the maze to survive past ~90 days**. The maze is not optional content; it is the only path to a long run.

**Your wife is the backstory, not the objective.** Lore drips in: who was down here before, what happened to them, occasional hints that she came this way. It gives the descent a personal weight and gives lore a reason to exist, but the scoreboard is days survived.

---

## 4. Time — One Counter, Two Readings

**There is a single day counter, and it starts at Day 0 when the run starts.** It never resets, never branches, and never runs at a different rate in one world than the other. It is the score, and it is what every unlock gate is measured against — including the Day 20 point at which maze entrances can begin to spawn. To the player it is always the same, simple, honest number: *how many days have I survived?*

What differs between the two worlds is **what each system does with that number**. The surface reads it cyclically. The maze doesn't read it directly at all — maze difficulty is anchored to what the *player* has done, not to the calendar (§4.2).

### 4.1 The Overworld Reading (cyclical)
The surface reads the day count **cyclically** — day within the year, wrapping. Seasons wheel: spring → summer → autumn → winter → spring. Drives temperature, animal populations, plant availability, water (melt/freeze/flood), and which entrances are open. It is predictable and it repeats. You can learn it and plan against it.

### 4.2 The Underworld Reading (monotonic, three stages)

Maze difficulty is **not** read off the run's day count. It is anchored to **player events**, and it escalates in three distinct stages:

| Stage | Begins | Escalation |
|---|---|---|
| **1 — Dormant** | Day 0 | **None.** Entrances cannot spawn before Day 20, and until you find one there is nothing running. |
| **2 — Discovered** | The day you **first find an entrance** | **Slow, linear.** The maze has noticed you. It stirs, gently, whether or not you go in. |
| **3 — Entered** | The day you **first go inside** | **Proper pace, exponential.** You are in it now, and it starts climbing in earnest. |

Escalation drives maze shift aggression, layout confusion, radiation strength and spread rate, and mob spawn rates — as already specified in [roadmap.md](roadmap.md) §1.

**Why this is better than anchoring to Day 20.** The clock starts when the *player* commits, not when the calendar says so, which means:

- **Discovery is a real decision with a cost.** Finding an entrance is not free information — it starts the slow clock. A player who deliberately avoids scouting for entrances is genuinely delaying the whole escalation curve, and that's a legitimate (if ultimately doomed) strategy.
- **Two-stage commitment.** You can find an entrance early and then take your time preparing, paying only the slow linear rate while you get your camp, food and gear in order. The moment you actually step inside, the curve turns over into its steep phase. That's a deliberate, legible, player-controlled escalation trigger.
- **No one is punished for taking the wilderness seriously.** A cautious player who spends 60 days learning to survive above doesn't get ambushed by a 40-day-old maze on their first dive. The maze they meet is always young when they meet it.

**Curve shape (continuous by construction):**

```
if not discovered:        escalation = 0
else if not entered:      escalation = A * (day - discoveryDay)                    // slow, linear
else:                     escalation = A * (entryDay - discoveryDay)               // banked linear
                                     + B * (exp(C * (day - entryDay)) - 1)         // exponential, starts at 0
```

The `- 1` matters: it makes the exponential term zero on the day you first enter, so the curve bends upward smoothly instead of jumping. The player feels the maze getting worse, never a sudden unexplained spike.

Once stage 2 begins, **the curve climbs whether or not you go back down.** A winter spent hunkered on the surface is a winter the maze spent getting worse. There is no way to pause it, and no way to farm the surface risk-free forever.

> **Balance note on "exponential".** A true exponential runs away hard and will turn a run from survivable to hopeless within a few days of the knee. That may be exactly what you want, given the score is days survived and death is meant to be inevitable — but it's worth checking in playtesting whether the ending feels like *pressure* or like *a wall*. If it's a wall, the usual fixes are a lower `C`, a soft cap, or a logistic curve that flattens at a brutal-but-playable ceiling. Flagged in §18.

**Implementation note.** Three stored numbers plus the day counter, only one of which the player ever sees:

```
day            // authoritative. Ticks at the normal rate everywhere. Score + unlock gates.
seasonalDrift  // extra season-time accrued while in the maze. Only ever increases.
discoveryDay   // unset until the first entrance is found
entryDay       // unset until the first time the player goes inside

season     = seasonOf(day + seasonalDrift)   // wraps modulo year length
escalation = escalationOf(day, discoveryDay, entryDay)   // per the curve above; ignores drift
```

The day counter stays clean and exploit-proof, the drift makes the maze feel temporally wrong, and the two event anchors put the difficulty curve's start in the player's hands.

### 4.3 Time Dilation — the seasons run fast, the day counter does not

**Resolved approach.** The maze should *feel* like it plays by different temporal rules without becoming a mathematically optimal way to farm the scoreboard.

- **The day counter ticks at the normal rate, everywhere.** Time below buys you no extra days survived, and no faster escalation. This closes the exploit outright.
- **The seasons accelerate while you are below.** Time in the maze accrues *seasonal drift* (§4.2) on top of the normal day tick, so spend too long down there and you climb out into a season you did not plan for — descend in late summer, surface into deep winter. The drift is permanent for the run; you never get that season back.
- **Consequences land at your base, not on the counter.** Outdoor crops dead and frosted over. Unmaintained structures damaged. Stores spoiled or raided. The window you were saving for has closed. The punishment for greed is *logistical*, which is exactly right for a survival game.
- **Hunger, thirst and stamina drain faster below.** Diegetically this is **stress and constant movement** — you are running, hiding, and terrified, not strolling. Mechanically it is a flat elevated drain rate while in the maze; it is **not** coupled to actual player movement input, so there's no incentive to stand still and no fiddly movement-accounting system to build.

Net effect: the maze is temporally wrong, the player feels it, the scoreboard stays honest.

---

## 5. The Entrances

Entrances are **discovered, plural, and varied**. Finding a new one is real progress — it's a shortcut past known territory and a different drop point into the network.

**Each maze spawn has on average 2–4 entrances.**

**Entrance types:**

| Type | Behaviour |
|---|---|
| **The sinkhole / hole in the ground** | **One-way. You fall in and you cannot climb out.** Easy to enter — often by accident — and the entire problem becomes finding another way up. The most Backrooms-appropriate entrance in the game. |
| **Seasonal entrances** | Only accessible or even *visible* in certain seasons. A service door swamped by spring melt and frozen shut in winter, passable only in late summer. A hatch you can only reach when the ground is frozen hard enough to walk on. This ties surface season directly to maze access and forces the year to have a rhythm: prepare in the warm months, dive in the window, survive winter on what you carried back. |
| **Ventilation stacks** | Sealed with bolts. Require the screwdriver (already in roadmap Phase 6). |
| **Freight lift** | Needs power. A mid-game unlock that converts a one-way fall into a repeatable, survivable route. |

---

## 6. Death, Saves & Pausing

**Full permadeath, both worlds.** Die in the maze or die on the surface and the run is over — back to Day 0, everything gone.

**Rationale (design intent):** the item set is deliberately *not* huge. Fewer items, each one genuinely fun to use. So reaching the point in a run where you're properly kitted out and then losing it should make you want to go straight back in and get there again. The chase is the point.

**Making permadeath fair — accepted design:** the two worlds kill you differently.
- **The surface rarely kills you outright. It cripples you.** Injury, illness, frostbite, infection, a broken tool at the wrong time, an empty smokehouse in November. Surface death should almost always be the end of a *chain of the player's own compounding mistakes*, visible in hindsight, never a sudden ambush.
- **The maze is where sudden death lives.** That's the trade you accept when you go down the hole.

**Saves & pausing (open, likely to change).** The current roadmap specifies no saves and no pausing while exploring the maze. Both may need to be relaxed for a game of this length — a multi-year survival run is a lot to ask of a single sitting. If saving is added, the intended form is:

- **One save slot at a time.** One run, saved in place, wiped on death. No branching, no backups, no rolling back a bad decision.
- **You can only save at a dedicated camp in the overworld.** Saving is a *place*, not a menu option. You cannot save in the maze, and you cannot save standing in open wilderness in the middle of a blizzard.

That second rule does a lot of work for free:

- **No save-scumming a dive.** Once you go down the hole you are committed until you come back up. The maze keeps its teeth entirely.
- **It makes the surface camp mean something mechanically**, not just as a shelter — it's the only place the run can be put down and picked up again.
- **Quitting has a cost.** Wanting to stop playing means physically getting yourself somewhere safe first, which is itself a survival decision, and a very thematic one.
- **It reinforces the nomadic loop** (§9) — your seasonal camps become your save points, so where you site them is a real strategic choice rather than pure resource logistics.

*(Open: what counts as a "dedicated camp" — probably basic shelter tier and above, so a bare fire and bedroll isn't enough. Also open: what happens if the player quits without saving. Standard approach is to leave the run where it was at the last save, which is consistent with everything above.)*

The **no-pause rule may survive even if saving is added**, since it's doing real work for the horror and survial realism.

---

## 7. Progression & Day Gates

Systems and items unlock at **day thresholds**, so the late game gets *more* interesting rather than just harder.

- **Maze entrances cannot spawn before Day 20.** Twenty days of pure wilderness survival first. This teaches the survival systems properly, makes the discovery of the first entrance a genuine event, and means the horror half is earned rather than handed over. Day 20 gates *availability* only — the difficulty curve doesn't start until you actually find an entrance, and doesn't reach its real pace until you go in (§4.2). The maze is always at its gentlest when you first meet it, however long you took to get there.
- **Other systems and items unlock at later day counts.** (Specific gates TBD.) The dog is mid-game. Deeper maze systems, the chemical lab, hydroponics, and the advanced tech tiers sit further out still.

This also gives the run a *shape*: early game is bushcraft, mid game is the first dives, late game is a fully-equipped operation running supply lines between two worlds.

---

## 8. Survival Systems

**Hunger, thirst, temperature** are core, and they **carry into the maze** rather than switching off — at an elevated drain rate (§4.3).

**Food below is nearly nonexistent.** There is no game to hunt, no clean water, and no fire. The only edible thing down there is **mushrooms**, and they will not sustain you for long — enough to stretch a dive, never enough to live on. *(To be expanded: nutritional value, the split between normal and magic mushrooms, whether spoilage applies, whether water can be found at all.)*

**This is what makes surface preparation load-bearing.** A dive is provisioned before it starts: smoked and dried food, full waterskins, charged batteries, working gear. The homestead isn't decoration — it's the staging ground, and a badly-prepared dive is lost before you enter.

**Sleep.** Sleeping advances the day and triggers maze regeneration in the shifting zone (as per roadmap Phase 4). Because your bed is now on the surface, **sleeping safely at home still shifts the maze** — reinforcing that the underworld clock cannot be paused. Sleeping *in* the maze needs separate, much riskier treatment: a bedroll in a cleared, barricaded room, high risk, but you don't lose your hard-won position. Failing to sleep still causes sluggishness, hallucinations, and passing out.

**Fire.** Renewable and central above; **nothing burns below** (gas pockets, or simply nothing to burn). This forcibly separates the two toolkits and stops surface solutions from trivialising maze problems.

---

## 9. Surface Shelter — Nomadic, Not Monolithic

**There is no mega-base.** Building one permanent fortress and turtling in it is explicitly not the game. You move with the seasons the way nomadic humans actually did — following game, water, and warmth — so the natural pattern is **a camp for each season** rather than one home for all of them.

This also quietly solves the permadeath problem: the player never sinks hundreds of hours into a single structure that one bad dive erases. Investment is spread thin and always partly disposable by design.

**Shelter tiers:**

| Tier | Build cost | Behaviour |
|---|---|---|
| **Basic camp** | Minutes | No shelter. A fire, a bedroll, somewhere to process a kill. Disposable — make one anywhere, abandon it without a second thought. |
| **Basic shelter** | Hours | Lean-to, debris hut, tarp. Real protection from weather, still cheap enough to leave behind when the season turns. |
| **Log cabin** | Days of in-game work | Permanent, immovable, genuinely good — the only realistic way to survive a hard winter. Takes serious time to build and **must be maintained across the seasons**. |

**The cabin maintenance rule matters.** You cannot build a cabin, wander off for three seasons, and return to a warm house in November. Left unattended it degrades — roof damage, rot, weather, animals — and you come back to a repair job, not a home. Winter shelter has to be *kept*, which means your seasonal circuit has to actually route back through it, which is what turns nomadic movement into a planning problem rather than random wandering.

---

## 10. Maze Camps

**Maze bases are forward camps, not homes.** A stash chest, a barricaded room, a light left burning, a bedroll. Establishing a camp deep in the network is a mid-game achievement rather than the opening move. Rope, doors and barricades stop being about building a home and become about **securing a route** — which is a better use for all three.

Existing base rules carry over where they still apply: camps must be placed in rooms, entrance/exit count is a real trade-off (too few and you get cornered, too many and you can't defend), no camps in radiation zones, and the "something is off with this room" warning when placing inside a regeneration zone.

---

## 11. Crafting — Two Tech Tiers

Two trees that should feel completely different to use.

**Surface (classical / bushcraft):** cordage, flint, hide, bone, wood, tannin, clay, sinew. Slow. Process-driven — things must be dried, cured, tanned, smoked. Nothing is instant. Materials are abundant; the cost is time and labour.

**Underworld (old-world tech):** batteries, wire, scrap metal, circuit boards, chemicals, components. Fast to assemble once you have the parts. The cost is the risk of getting them.

### 11.1 What "limited" actually means down there

The maze is **limited, not permanently finite** — the distinction matters:

- **Renewable, but only in the maze.** The **chemical lab** (batteries, chemicals) and **hydroponics** (better and more reliable food than the surface can consistently produce) are ongoing production systems, and they can *only* be run underground. Once established they keep producing — which is precisely why the maze stays relevant for the whole run instead of being strip-mined and abandoned.
- **Limited-chance salvage.** Most maze items are scarcer than anything on the surface, and some are rare enough that you may get **only one realistic chance at them in a given run**. Miss the chance, or lose the item beyond repair, and that capability is gone **for the rest of that save** — not gone from the game. Permadeath rerolls the dice; a new run is a new set of chances.

### 11.2 Durability & repair
Every item runs on **Minecraft-style durability** — use it enough and it breaks. Some tools can be **repaired at a dedicated repair/work table** rather than being lost forever, so gear management is an ongoing cost rather than a cliff.

Salvaged tech is **tied to the maze**, not doomed by it: batteries need the chemical lab, and the lab is underground. That circularity — the maze funds the equipment required to survive the maze — is the load-bearing structure of the late game.

**The interesting space is where the tiers cross.** A flint spear is cheap, renewable, and weak. A machete found below is superb, wears down, and can only be maintained by going back for what maintains it.

---

## 12. The Dog

Found in the **overworld, mid-game**. On the surface it hunts and tracks. **It can be taken down into the maze**, where it becomes an early-warning system against flanking mobs and can track scent through corridors.

Because it can die permanently down there, the decision to bring it is a genuinely hard one every single dive. Cheapest high-impact feature in the design.

---

## 13. Maps

The same crafting recipe produces two completely different relationships with the player:

- **Surface maps are permanently true.** Terrain never moves. A map you draw of the wilderness is good forever, and mapping the land is real, durable progress.
- **Maze maps rot.** The layout shifts every time you sleep. Your map starts decaying the moment you draw it, and the older it is the more it lies.

**Survey maps of the wild are found below.** The bunker was a research and survey station (§14), so its archive contains **old-world maps of the surface** — cached resource locations, water sources, old trails, terrain that would otherwise take many in-game weeks to scout. This is a clean, contamination-free reason for the underworld to reward the overworld: *you go down into the dark to learn about the light.*

Existing map items (Classic Map, Magic Map, Infected Map, Computer Map) continue to apply to the maze; the survey maps are a new surface-facing category.

---

## 14. Lore & The Archive

**Backstory:** the bunker network was built and inhabited by the **last survivors of the "old world"**, before an apocalypse that **nobody actually knows the details of**. It ended a long time ago. The surface healed. The network did not.

**Nothing is confirmed.** Lore arrives as rumours and fragments in found text documents, competing with each other:
- **The Great Storm**
- **The Floods**
- **The Fires**
- **Radiation** — the most plausible account, and the one that fits the radiation still present in the network

Let the documents disagree. An unresolved apocalypse is more unsettling than a stated one, and it costs nothing to maintain.

**The bunker as records archive.** It was a research and survey station, which justifies both the technology and the paperwork. Found down there: survey maps of the wilderness (§13), research logs, personnel records, incident reports, and — occasionally, rarely — traces of people who came down more recently than the old world. **Including hints about your wife.**

Keep the wife thread as a slow, sparse drip. It should never become a fetch quest with a marker; it's the reason the player reads the documents at all.

---

## 15. Terrain, Water & Visual Polish for the Overworld

> **Exploratory — nothing here is fully committed except what's marked settled.** Earlier drafts of this section explored terrain **height** as a rendered field — a heightmap, hydraulic/thermal erosion, quantised cliff bands with footprint accounting, ramps for connectivity, and a posterised hillshade shader to light it in a way that didn't muddy pixel art. **That whole track has been cut.** Height was never a gameplay input (§15.5's old "overhang test" already showed it could only ever be a scalar shading field, not real 3D — no overhangs, no traversal freedom, nothing an inventory or a recipe could touch), and the cost of doing it convincingly — a global erosion pass, cliff-footprint generation, connectivity-guaranteeing ramps, a bespoke lighting shader — was large for a purely visual, non-load-bearing payoff. **The overworld is flat, the same way the maze is flat.** Depth comes from mixed-projection sprites (§15.4) and lighting/animation polish (§15.6), not from geometry.

What survives from the original plan is the part that was always gameplay, not geometry: water needs to exist for drinking, fishing, orientation and seasonal freeze/thaw (§8, §13); biomes need to place vegetation and animals sensibly; and the flat world still deserves the rendering effects that don't depend on height at all.

### 15.1 Water bodies: rivers & lakes

Rivers and lakes are staying in the game — they're too load-bearing to cut (drinking water, fishing sites, crossings as route decisions, frozen rivers as winter roads, spring melt as a hazard, and §13's "follow the water downstream" as free orientation in a world where maps are a scarce crafted good).

What's genuinely open is **whether rivers need a simulated flow direction**, and that's worth deciding deliberately rather than defaulting into it:

| Approach | What it is | Trade |
|---|---|---|
| **A — Authored, directional** | River paths generated as domain-warped curves running from a source region to a lake or the map edge, with an implied downstream direction carried for animation (current, foam drifting one way) and possibly gameplay (freezing solid, a future raft or current push). | Gives back the orientation payoff and the "frozen river as a road" beat. Costs more: a river is then a *connected, planned* object, not a chunk-local decoration (see §15.5). |
| **B — Non-directional** | Lakes and river-*shaped* noise features (Poisson-seeded blobs, meandering bands) with no consistent flow. | Cheapest — chunk-local, no global planning needed. Reads as scenery you cross rather than geography you navigate by; this is the Minecraft model the original draft dismissed, but in a 2D top-down game with no boats or current-based traversal, "does it actually need to flow" is a fair question, not a concession. |

Flagged as an open question in §18: does anything in this game actually *consume* flow direction (a raft, a current that pushes the player, water-powered crafting), or is it purely a visual/orientation cue? If the latter, Option A can be faked cheaply — author the path once, animate the texture along it — without ever computing true flow accumulation.

Lakes: Poisson-disc-seeded blob shapes, either standalone or fed by a river's endpoint.

### 15.2 Biomes & moisture

Keep the **Whittaker diagram** approach — a 2D lookup of temperature × moisture → biome type (grass, forest, scrub, wetland, tundra…) → search `Whittaker biome diagram`. Without a height field there's no orographic rain shadow to drive moisture, so it comes from a simpler, still-legible model: latitude for temperature, a coarse moisture noise field plus a bonus near rivers/lakes/coast for wetness. Cruder than the elevation-driven version, but still gives a world where biome placement feels governed rather than random, which is the actual payoff — a veteran player learns that grasslands cluster round water and reads an unfamiliar map faster for it.

### 15.3 Distribution: resources & set pieces

- **Poisson-disc sampling (Bridson's algorithm)** for resources and landmarks — random placement with a guaranteed minimum spacing, so the world reads as naturally uneven (blue noise) rather than clumped-with-dead-zones (white noise). → search `Poisson disc sampling Bridson`. This is the exploration-pacing lever: it guarantees "something worth finding within N minutes of walking" without a visible grid.
- **Hand-authored prefabs** (ruins, standing stones, abandoned cabins, **maze entrances**) placed at sites passing constraint checks: correct biome, minimum distance from siblings, near or far from water. Noise produces variety; authored set pieces produce *memory* — nobody remembers a hill, everybody remembers the lightning-split tree by the ford.

### 15.4 The current maze rendering approach, and why it carries over

The game already uses **mixed projection**, and the reasoning behind it applies to the overworld unchanged.

Current state:

| Element | Projection | Where |
|---|---|---|
| **Corridors** | **Flat top-down.** Bitmasked tile drawn at `(x, y)`, no projected face. | `maze_renderer.cpp` ~L126-150 |
| **Rooms with visible floor below** | **3/4 oblique.** Roof edge at `y-2`, upper wallpaper at `y-1`, base wallpaper at `y`. | `maze_renderer.cpp` ~L180-220 |
| **Props** (doors, lockers, benches) | **3/4.** Door drawn 16×29 anchored at its base; furniture shows front faces. | `item_renderer.cpp`, `maze_renderer.cpp` |
| **Player** | **3/4.** Front-facing sprite — you see the body, not the top of the head. | `player_renderer.cpp` |

**Mixed projection is the standard convention, not a compromise.** Stardew Valley, Zelda: A Link to the Past, Pokémon and Chrono Trigger all draw the ground plane top-down and give a projected face only to things that need to read as *vertical*. No 2D game does consistent optical geometry. The rule is "project only what needs to read as vertical," because the goal is legibility, not correctness. The overworld's trees, rocks, cabins and set pieces should follow the same rule: flat ground, projected props — no heightmap required to make that convention work.

### 15.5 World scale, seed & the edge

**Settled: a new world seed per run.** Permadeath rerolls the world. This keeps discovery fresh and stops run #20 from being a memorised optimal route, which matters when the score is days survived.

**What carries across runs is not the map, it is literacy.** Water collects in basins, biomes cluster round it, resources thin out with distance from camp. A veteran reads an unfamiliar map quickly because they understand the generator's logic, not because they've memorised a layout.

**Resolution decoupling still matters, for a smaller reason than before.** The world doesn't need a two-resolution split to afford erosion any more — but it still benefits from one, because the world is meant to be large (§15's original ~8,000–16,000-tile target still stands) and a coarse pass keeps that affordable: biome/moisture assignment and river placement run once, on a coarse grid, at world creation, and are stored in the save; per-chunk detail (tree/rock/resource scatter) is generated on demand from `hash(seed, chunkX, chunkY)` and never stored, exactly as originally planned. The maze's toroidal 1D-array grid is a different representation entirely and shares no world-storage code with this.

**Whether the world is strictly finite now depends on the §15.1 decision.** If rivers stay non-directional (Option B), they're chunk-local noise and place no constraint on how the world is generated — it could in principle tile forever. If rivers go directional (Option A), a connected source-to-outlet path is a global object in the same way flow accumulation used to be, and the world needs the same finite, coarse-grid-then-detail treatment the original erosion argument made. Either way, the design argument for a large-but-finite world stands on its own regardless of the water decision: nomadic seasonal camps (§9), a maze that spawns near you (§5), a wife to find, and ~90 days of surviving in one place all depend on the world being *knowable*, the way Valheim, Don't Starve and Unreal World's worlds are.

**The edge.** Not a cliff — an abrupt drop reads as an artificial wall, and there's no heightmap to raise one anyway. Leaning **an impassable boundary biome** — dense highland forest, rock scree, whatever reads as "you don't go further" — with the boundary's radius perturbed by low-frequency noise so it's irregular rather than a legible circle. Thematically it's still the mountain ring: an enclosed valley, which is why nobody leaves and why the search for your wife stays local. A coastline is the alternative (prettier, adds a beach biome and fishing) but needs an answer for swimming around the edge. Either way the boundary should sit far enough out that most players never reach it and it stays soft knowledge rather than something bumped into.

### 15.6 Visual polish & effects — where the payoff actually is

With height off the table, this is now the section carrying the overworld's visual ambition, and it's worth treating as the real plan rather than a leftover list. Roughly in order of payoff per line of code:

- **Palette unification + atlas JSON.** The current sprite sheets come from several artists with different palettes; snapping everything to one shared ramp does more for the game's look than any single new asset, and it's a prerequisite for the lighting work below reading cleanly.
- **Y-sorted render queue.** The current draw order is three fixed layers (maze → player → items in `playing_state.cpp` ~L432-435), fine for a tile maze but not for a forest where trees, rocks, the dog, animals and the player need to interleave with each other. Collect drawables into one queue, sort by base Y in world pixels (not tile row, or entities pop between tiles) — `O(k log k)`, or `O(k)` with a bucket sort since baseY is bounded by screen height.
- **8-bit autotiling.** The existing `tileMap[16]` is a 4-bit cardinal-only mask, fine for solid walls; biome borders (grass/sand/water) need the corner-aware 47-tile "blob" set or diagonal transitions look wrong.
- **Banded/posterised lighting.** The old hillshade-specific version of this trick is gone with the heightmap, but the underlying principle still applies everywhere a continuous light term meets pixel art: quantise into 3-5 discrete steps and map each to a chosen palette colour rather than a continuous multiply, so day/night transitions and torchlight shade in clean bands instead of muddy in-betweens. → search `posterized lighting pixel art`, `pixel art palette ramp shading`.
- **Day/night colour ramp**, using the banding above.
- **Weather** — rain, fog, snow, a fog-density uniform. Turns the same terrain into a visibly different place and plugs straight into seasons and temperature.
- **Wind sway** — offset the upper pixels of grass and tree sprites by `sin(time + worldPos)`. Roughly twenty lines, and the world stops feeling like a diorama.
- **Animated water with edge foam** — the direct payoff of §15.1's rivers/lakes; even non-directional water reads as alive once it animates.
- **Ambient particles** (pollen, leaves, snow, fireflies) and per-biome colour grading.

### 15.7 A staged path

| Stage | Work | Payoff |
|---|---|---|
| **0** | Palette unification + atlas JSON. | Unblocks everything else; fixes existing asset incoherence. |
| **1** | Biome/moisture assignment + water placement (coarse grid), per-chunk detail scatter. | The world exists and reads as governed rather than random. |
| **2** | Render polish pass — Y-sort, autotiling, banded lighting/day-night, weather, wind sway, animated water, particles, per-biome grading. | This *is* the visual identity of the overworld. No further stages are planned — there's no elevation pass waiting behind this one. |

### 15.8 Reference games

| Game | What to look at |
|---|---|
| **Unreal World** | The direct genre inspiration, now also the closest structural match: fixed finite world, rivers and lakes that structure travel, fishing and settlement, drawn in very plain top-down tiles. No elevation-driven relief. |
| **Necesse** | Closest genre and aesthetic sibling (2D top-down pixel survival, procgen biomes) — its terrain is flat, which used to be a limitation to note and is now simply the model. |
| **Stardew Valley** | Reference for *considered* flat pixel art — palette discipline, readable tile borders, mixed-projection props — with the cliff-specific lesson dropped since there's no heightmap here. |
| **Minecraft** | The counter-example for directional rivers (they wander, dead-end, never consistently reach an ocean, and nobody navigates by them) — but a fair *model* if §15.1 settles on Option B, since this game has no boats or current-based traversal either. |
| **Factorio** | Noise-blob water, correctly so — water there is purely an obstacle. Useful reminder that directional flow only pays for itself when something in the game actually consumes the direction. |

---

## 16. What This Changes in the Existing Roadmap

Deltas against [roadmap.md](roadmap.md). Nothing below is deleted — it's relocated or reframed.

| Roadmap item | Change |
|---|---|
| **Vision (§1)** | Game is now two worlds, hub-and-spoke. Maze is one half, not the whole game. |
| **Permadeath (Phase 4)** | Unchanged in severity — still a full reset from any death, in either world. Now explicitly covers surface death, with surface threats tuned to cripple rather than instantly kill. |
| **No saves / no pausing (Phase 4, §1)** | Flagged as likely to change. If saving is added: single slot, wiped on death, and **only possible at a dedicated overworld camp** — never in the maze. No-pause in the maze likely survives regardless. |
| **Day counter (Phase 4)** | Stays a single counter from Day 0 of the run — score and unlock gates read it directly, including the Day 20 entrance-spawn gate. Surface reads it cyclically (seasons). **Maze difficulty no longer reads it at all** — escalation is event-anchored in three stages: nothing until you find an entrance, slow/linear from discovery, exponential from first entry. One hidden accumulator (`seasonalDrift`) makes seasons run fast while below. |
| **Sleep (Phase 4)** | Bed moves to the surface. Sleeping above still shifts the maze. Maze sleep becomes a separate high-risk mechanic. |
| **Base placement (Phase 4)** | Splits in two: nomadic seasonal surface shelter (camp → shelter → cabin, with maintenance), and maze forward camps under the existing placement rules. Explicitly no mega-base. |
| **Lotka-Volterra (Phase 4)** | Repurposed to the **surface ecosystem** — real predator/prey populations (wolves and deer) rising and falling across seasons, so over-hunting a valley genuinely depletes it. A more honest use of the model than mob spawn caps. |
| **Crafting (Phases 3/5)** | Splits into two tiers: abundant-but-slow surface bushcraft, and scarce-but-powerful old-world tech. Chemical lab and hydroponics stay underground as renewable production anchors. Durability + repair table added. |
| **Torch/battery (Phase 5)** | Elevated to a core asymmetry. Fire above, batteries below, batteries only made in the maze's chemical lab. |
| **Maps (Phase 3)** | Existing maze maps unchanged. New category: surface survey maps found in the archive. |
| **Dog (Phase 6)** | Promoted from "advanced system" to a mid-game surface unlock that can be taken below. |
| **Radiation (Phase 3)** | Stays entirely underground. Explicitly does **not** surface. |
| **New: wilderness half** | Terrain/biome generation, seasons, weather, hunting, fishing, foraging, shelter building and maintenance, temperature — all new, and not yet phased. Generation and rendering approach explored in §15 (flat, no elevation — water, biomes and rendering polish only). |
| **Timeline** | ~105 days is obsolete. Multi-year project; phases need re-planning from scratch. |

---

## 17. Gameplay Risks to Watch

Distinct from the open questions below: these are not decisions waiting to be made, they are **known weak points in the design that only playtesting can resolve**. Each one is a way this could be structurally sound on paper and still not fun.

### 17.1 The wilderness half must be fun second-to-second — no interlock fixes this
**The biggest remaining risk in the whole design.** Hunting, fishing, foraging and processing degrade into menu-driven chores very easily. Unreal World is deeply admired and also famously dry. Every interlock in this document assumes the surface loop is worth playing on its own terms; if tracking an animal, setting a trap, landing a fish or building a shelter doesn't feel good *in the hand*, the surface becomes homework between dives and the prep loop turns into friction rather than tension.

This is an execution problem, not a structural one, which means no amount of further design work solves it. **Prototype the moment-to-moment feel of one surface activity early** — before building breadth — and be willing to conclude the answer is "hunting needs to be a real mechanic, not a menu."

### 17.2 Carry capacity is load-bearing
Triage is what makes preparation matter: food *or* batteries, not both. Several interlocks — prep-before-descent (§8), the battery economy (§11), dive-length pressure (§4.3) — go slack simultaneously if inventory ends up generous. Treat carry capacity as a core balance lever, not a convenience setting, and resist the very strong late-development pull toward expanding it.

### 17.3 The maze has to stay frightening after mastery
Horror decays with exposure. Shifting layouts help, and the tonal boundary (§2) does real work by resetting the player's baseline every time they surface — but familiarity still erodes dread. Escalating numbers alone will not sustain it: a Day 200 maze that is merely *more* of the Day 40 maze is a chore with higher stats. Plan for **novelty injection at depth** — new room types, new rules, things that behave differently rather than harder.

### 17.4 Twenty days before the hook is a retention risk
The Day 20 gate is excellent for immersion and earns the reveal properly. It is also a long time to ask a new player to wait, and a serious problem for a Steam demo where you have roughly fifteen minutes to convince someone.

Mitigation worth designing early: let players *know* there is something underground long before they can reach it — a rumour in the wife's belongings, a distant concrete structure visible but sealed, a document, a sound. Curiosity can be seeded on Day 1 even if access is gated to Day 20. **The gate should withhold the maze, not the knowledge that a maze exists.**

### 17.5 The battery death spiral needs to be legible, not sudden
Spending batteries to go get batteries (§11.1) is one of the strongest tensions in the design — but only if the player can *see it coming*. If they discover they're below the threshold only when they're already stranded, it reads as an unfair loss rather than a bad decision. The player needs enough information to recognise "I have two more dives in me" several days out.

### 17.6 The two halves must not become sequential
The intended shape is an oscillation — prepare, dive, recover, repeat, with rising amplitude. The failure mode is that the game becomes *front-loaded wilderness followed by all-maze*, with the surface reduced to a restock stop once the player is equipped. Watch for this in long playtests specifically; if late-run players stop caring about seasons, the surface has stopped being a game and become a shop.

---

## 18. Open Questions

- **Exact seasonal acceleration ratio while below.** How many surface days should a long dive cost? Needs playtesting against how long a satisfying dive actually is.
- **Escalation curve constants** (`A` linear rate, `B`/`C` exponential rate — §4.2). Specifically: does a true exponential end runs with *pressure* or with *a wall*? If it's a wall, drop `C`, add a soft cap, or switch to a logistic curve that flattens at a brutal-but-playable ceiling.
- **Does discovering further entrances after the first do anything to escalation?** Recommend no — only the first discovery and first entry anchor the curve, or scouting becomes self-punishing.
- **Mushroom sustenance numbers.** How much does a mushroom actually buy you, is there any water source below at all, and does carried food spoil at the accelerated rate?
- **Does *any* lore persist across runs?** A persistent lore/document codex (knowledge carries over, items never do) would let the wife thread build across runs instead of resetting every death. Worth deciding early — it changes how documents are written.
- **Does the no-pause rule survive** if single-slot saving is added? (Recommend yes — it's doing real work for the horror, and camp-only saving already makes the maze unsaveable.)
- **What shelter tier qualifies as a save point?** Recommend basic shelter and above, so a bare campfire doesn't count and siting a proper camp is a real decision.
- **What forces movement across the surface map?** Seasons and resource depletion are the stated drivers. Needs concrete numbers — how fast a valley depletes, how far you must range, how many seasonal camps a run realistically sustains.
- **Cabin degradation rate.** Fast enough that abandoning one has teeth, slow enough that maintenance isn't a chore. This number decides whether the nomadic loop feels like planning or like babysitting.
- **Level 2 descent** (roadmap Phase 6) — still in, and now reads as a third layer below the maze. Needs re-siting within the new structure.
- **Do maze entrance locations persist across the shifting zones,** or can a regeneration event bury the way you came in? (Very good horror, potentially very unfair.)
- **Do rivers need simulated directional flow, or is an authored source-to-outlet path (or no direction at all) enough?** (§15.1) Turns on whether anything in the game actually consumes flow direction — a raft, a current, water-powered crafting — versus it being purely a visual/orientation cue. This also decides whether the water layer places any constraint on how large or "infinite" the world could be (§15.5).
- **World size.** ~8,000 tiles across (~33 min to cross) or ~16,000 (~67 min)? Larger costs little at generation time given the coarse/fine split (§15.5), but a world too large for one run to meaningfully explore wastes the biome and water placement work.
- **Mountain ring or coastline** at the world edge? (§15.5) Leaning mountain ring (as an impassable boundary biome, not a raised heightmap edge), but a coast adds a beach biome and sea fishing.
- **Tile size needs a single committed spec before any art is bought or drawn.** Currently `player_renderer.hpp` uses `TILE_SIZE = 16` for sprite cells while `maze.hpp` defaults `cellSize = 32` for world cells. Fine as-is, but the overworld spec (tile size, 3/4 convention, master palette, atlas format) should be written down explicitly rather than inherited.
- **Engine decision.** At multi-year, two-genre scope, the from-scratch C++/raylib build costs significantly more than it did at the original scope. Not urgent, but should be a conscious choice rather than an inherited default.

---

## 19. Copyright & Licensing

The repository currently has **no licence file**. That should be fixed before the project is public or shown around.

**What to add — a proprietary, all-rights-reserved declaration.** A `LICENSE` file at the repo root stating that the source, assets, design documents and game concept are copyright the author, all rights reserved: no copying, modification, redistribution, or commercial use without written permission. Explicitly cover **assets and design docs**, not just code — art, audio, text, lore and this document are separately copyrightable and are the parts most worth stealing.

**Points worth knowing:**

- **Copyright is automatic; the licence file is what makes it unambiguous.** Work with no licence is legally "all rights reserved" by default, but an explicit file removes any argument that it was offered as open source, and gives you something concrete to point at.
- **Do *not* use MIT/Apache/GPL here.** Every standard open-source licence grants exactly the redistribution rights you're trying to prevent. This needs a custom proprietary notice, not an OSI licence.
- **A public GitHub repo grants viewing and forking regardless.** GitHub's Terms of Service let any user view and fork public repositories, licence or not. If protecting the work actually matters, **keep the repository private until release** — that is the only practical protection. A licence file deters honest people and gives you legal footing; it does not stop anyone determined.
- **Third-party dependencies have their own terms you must honour when distributing builds.** Raylib (zlib/libpng), Dear ImGui (MIT), rlImGui, and GoogleTest (BSD-3-Clause) are all permissive and fine for commercial closed-source use, but MIT and BSD-3 **require their copyright notices to be included with binary distributions**. Plan for a `THIRD_PARTY_NOTICES.md` shipped alongside the game. Your own proprietary licence covers your code, not theirs.
- **Add a short copyright header** to source files, or at minimum a copyright line in `README.md`, so the claim travels with any copied fragment.
- **Keep dated evidence of authorship.** Git history with real timestamps is already good evidence of when and by whom the work was created. If this ever goes commercial, formal registration is worth looking into in your jurisdiction — that's a question for an actual lawyer, not this document.
