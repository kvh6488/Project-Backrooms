# The Wilderness — Overworld / Underworld Integration Design

> **Living document** — the design for merging the wilderness survival game into Project Backrooms as a single, coherent title. Last updated: **03-09-2026**.
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

## 15. Terrain Generation & Rendering the Overworld

> **Exploratory — nothing here is a committed decision.** This section records the current thinking on how the wilderness is generated and drawn, the specific problems that surfaced while working through it, and the candidate fixes, so the reasoning does not have to be rediscovered later. The only thing treated as settled is **a new world seed per run** (§15.3). Everything else is options with a leaning.

### 15.1 The generation pipeline

Layered. Each layer reads the one above it, which is what makes the world feel *governed* rather than noisy. Search terms are given so the visual output of each algorithm can be looked up directly.

**Layer 1 — Base elevation.** Plain fBm (fractal Brownian motion over Perlin/Simplex noise) gives rolling hills, but it is soft and samey — no ridgelines, no drainage, a lumpy duvet. Two corrections:

- **Ridged multifractal noise** — `1 - abs(noise)` accumulated per octave. Creases become sharp ridges instead of round bumps. → search `ridged multifractal noise terrain`
- **Domain warping** — offset the sample coordinates by a second noise field before sampling. Turns symmetric blobs into organic meanders, very cheaply. → search `domain warping inigo quilez`
- **Radial falloff mask** multiplied against the height, giving a natural world boundary instead of an invisible wall. → search `island falloff map procedural generation`

*Gameplay effect:* ridgelines are barriers, so passes and valleys become the only ways through. Travel turns into route-finding rather than walking in a straight line.

**Layer 2 — Erosion.** The biggest single visual upgrade, and what makes terrain read as real rather than generated.

- **Hydraulic erosion (droplet simulation)** — drop hundreds of thousands of virtual raindrops; each rolls downhill, erodes on steep slopes, deposits where it flattens, dies after ~30 steps. Channels carve themselves because droplets converge into the same low paths. → search `Sebastian Lague hydraulic erosion`
- **Thermal erosion** — material above a talus angle slides downhill, softening peaks and building scree. → search `thermal erosion terrain talus angle`

*Gameplay effect:* erosion generates the interesting places for free. Valleys are where water is, where game concentrates, where travel is easy, and where camps get sited.

**Layer 3 — Hydrology.** Rivers and lakes.

- **D8 flow direction + flow accumulation** — each cell drains to its steepest lower neighbour; accumulate upstream cell counts; cells above a threshold are streams, higher still are rivers. One sort plus one linear pass. → search `D8 flow accumulation algorithm`
- **Priority-flood depression filling** — resolves sinks so water has somewhere to go; the filled basins become lakes. → search `priority flood depression filling DEM`

*Gameplay effect:* the strongest landmark type available. Drinking water, fishing sites, crossings and fords as route decisions, frozen rivers as winter roads, spring melt as a hazard. Critically, rivers give the map **direction** — "follow the water downstream" is orientation without an item, which matters because maps are a scarce crafted good (§13).

**Layer 4 — Climate.** Temperature from latitude plus an **environmental lapse rate** (~-6.5 °C per 1000 m). Moisture from a prevailing wind traced across the map, dumping rain as air rises over ridges and arriving dry on the lee side — **orographic rain shadow**. → search `rain shadow effect diagram`

*Gameplay effect:* dry scrub appears behind mountain ranges and wet forest on the windward side, with nothing placed by hand. Players cannot articulate the rule but feel that the world obeys one. Feeds the temperature survival system (§8) directly.

**Layer 5 — Biomes.** **Whittaker diagram** — a 2D lookup of temperature × precipitation → biome type. → search `Whittaker biome diagram`

*Gameplay effect:* biome placement becomes predictable in the good way. A veteran reads a ridgeline and knows roughly what is on the other side.

**Layer 6 — Distribution.** **Poisson-disc sampling (Bridson's algorithm)** for resources and landmarks — random placement with a guaranteed minimum spacing. Blue noise spreads evenly but irregularly, which is what nature looks like; white noise clumps and leaves dead zones. → search `Poisson disc sampling Bridson`, `blue noise vs white noise distribution`

*Gameplay effect:* this is the exploration pacing lever. It lets us guarantee "something worth finding within N minutes of walking" without the result feeling gridded.

**Layer 7 — Set pieces.** Hand-authored prefabs (ruins, standing stones, abandoned cabins, **maze entrances**) placed at sites that pass constraint checks: flat enough, correct biome, minimum distance from siblings, near or far from water.

**Wave Function Collapse** will come up in any search on this (→ `wave function collapse algorithm`, lots of illustrative gifs) but is probably the **wrong tool here** — slow, hard to art-direct, better suited to structured interiors than open terrain. Constrained prefab placement is simpler and more controllable.

*Gameplay effect:* noise produces variety; authored set pieces produce **memory**. Nobody remembers a hill. Everybody remembers the lightning-split tree by the ford.

### 15.2 Resolution decoupling — why the world can still be large

The global algorithms (erosion, flow accumulation, rain shadow) are expensive and cannot be chunked. The naive conclusion is that the world must therefore be small. It does not, if the world is generated at **two resolutions**:

**Coarse grid — global systems.** ~2048 × 2048 cells, where **1 coarse cell = 8 game tiles**. Erosion, flow accumulation, depression filling, rain shadow and biome assignment all run here. That is ~4M cells, roughly 16 MB, and erosion takes a second or two. Generated once at world creation and stored in the save.

**Fine detail — per chunk, on demand.** When the player approaches a chunk, bicubically interpolate the coarse heightmap and add a couple of octaves of detail noise, then scatter trees, rocks and resources. Never stored — regenerated deterministically from `hash(seed, chunkX, chunkY)`, with only player modifications persisted as diffs.

**Resulting scale.** 2048 coarse cells × 8 tiles = **16,384 tiles across**. At the current player speed (130 px/s over 32 px cells ≈ **4 tiles/second**) that is roughly a **67-minute walk** edge to edge. Halving it to ~8,000 tiles gives ~33 minutes. The full ~268M fine tiles never exist in memory at once; the expensive global work only ever sees 4M cells.

Rivers computed at coarse resolution come out ~8 tiles wide, which then meander within that corridor using detail noise rather than running as a straight 8-wide band.

**Note:** this is a different world representation from the maze. The maze's implicit grid (`y * width + x` over a fixed toroidal 250×150) does not extend to a chunked world; the overworld needs a hash map of chunk coordinate → chunk with LRU eviction. The two halves share almost no world-storage code.

### 15.3 Why the world is not infinite

Two of the layers above are **mathematically global**, not local:

- **Erosion** — droplets do not respect chunk boundaries. Eroding one chunk requires its neighbours, which require theirs, transitively the entire map. Chunk-local erosion produces visible seams and physically wrong drainage.
- **Flow accumulation** — a river's size is a function of every cell upstream of it. In an infinite world that basin is unbounded, so discharge is not computable.

Everything else chunks fine: height from noise is samplable at any coordinate, hillshade needs only four neighbours, cliffs are local, climate-noise biomes are local. So the honest trade is precisely:

> **an infinite world, OR eroded terrain with real rivers — not both.**

The middle option is fake rivers from ridged, domain-warped noise. They look like rivers but do not flow — no consistent downhill, no confluences, no source-to-mouth. This is what Minecraft does, and it is why its rivers are scenery you cross rather than geography you navigate by. It costs the "follow the water downstream" payoff entirely.

**The design argument is stronger than the engineering one.** Infinity is anti-thetical to this game. Nomadic seasonal camps (§9), a maze that spawns near you (§5), a wife to find, and ~90 days of surviving in one place all depend on the world being *knowable*. Valheim, Don't Starve and Unreal World all use large finite worlds for exactly this reason.

**Settled: a new world seed per run.** Permadeath rerolls the world. This keeps discovery fresh and stops run #20 from being a memorised optimal route, which matters when the score is days survived.

The consequence is worth stating explicitly, because it shapes how the generator should be tuned: **what carries across runs is not the map, it is literacy.** Ridges mean passes. Water runs downhill. The lee side of a range is dry. A veteran reads an unfamiliar map quickly because they understand the generator's physics. That skill curve only exists because the generation is physically grounded rather than arbitrary noise — which is the real argument for doing hydrology properly.

### 15.4 What happens at the edge of the map

Not cliffs — an abrupt drop reads as an artificial wall. Two natural options:

- **A ring of impassable mountains.** Multiply the heightmap by a radial falloff that *rises* toward the edge, with **the falloff radius perturbed by low-frequency noise** so the basin is irregular and lopsided rather than a circle. A perfect circle is instantly legible as generated.
- **A coastline.** The same falloff inverted, land fading to sea. Prettier, adds a beach biome and sea fishing, but needs an answer for boats and for players swimming out.

**Leaning: the mountain ring.** It is thematically exact — you live in an enclosed valley, the mountains are why nobody leaves and why the search for your wife stays local, and it is structurally the Glade. It also avoids the "swim around the edge" problem.

Either way the edge should be something most players never reach. At half an hour or more of walking, the boundary becomes soft knowledge ("the mountains ring the valley") rather than a barrier that gets bumped into.

### 15.5 Height is a field, not a third dimension

Worth stating plainly because it affects how the code is structured: **adding terrain height does not make this a 3D game.**

A heightmap is a **scalar field over 2D space** — `h(x, y)`, one number per tile. Entities keep an `(x, y)` position and their z is *derived*: `z = h(x, y)`. z is not an independent degree of freedom, so it is not a coordinate.

**The overhang test.** In a genuinely 3D world you can have two different things at two different z values over the same `(x, y)` — a bridge with a road under it, a cave beneath a hill. A heightmap physically cannot represent that, because there is exactly one height per column. That impossibility is the definition of 2.5D.

So the height field buys exactly three things, none of which is a coordinate:

1. **Shading input** — hillshade reads the gradient (§15.9).
2. **A traversal rule** — cannot climb a tile more than N levels above you.
3. **Derived data** — temperature via lapse rate, water flow direction, camp-siting flatness, animal density in valleys.

`Vector2 m_position` stays a `Vector2`. Collision stays 2D. Nothing about the engine becomes 3D.

*For contrast:* if the maze later gains multiple **floors** — Level 0, Level 1, each with independent contents — that *would* be genuinely 3D data, since two things can occupy the same `(x, y)` at different z. That is what Dwarf Fortress does with z-levels, rendering one horizontal slice at a time. The Level 2 descent in the open questions is closer to that model than to a heightmap.

### 15.6 The current maze rendering approach, and why it is correct

The game already uses **mixed projection**, and the reasoning behind it should be preserved rather than "cleaned up" by a later pass.

Current state:

| Element | Projection | Where |
|---|---|---|
| **Corridors** | **Flat top-down.** Bitmasked tile drawn at `(x, y)`, no projected face. | `maze_renderer.cpp` ~L126-150 |
| **Rooms with visible floor below** | **3/4 oblique.** Roof edge at `y-2`, upper wallpaper at `y-1`, base wallpaper at `y`. | `maze_renderer.cpp` ~L180-220 |
| **Props** (doors, lockers, benches) | **3/4.** Door drawn 16×29 anchored at its base; furniture shows front faces. | `item_renderer.cpp`, `maze_renderer.cpp` |
| **Player** | **3/4.** Front-facing sprite — you see the body, not the top of the head. | `player_renderer.cpp` |

**Mixed projection is the standard convention, not a compromise.** Stardew Valley, Zelda: A Link to the Past, Pokémon and Chrono Trigger all draw the ground plane top-down and give a projected face only to things that need to read as *vertical*. No 2D game does consistent optical geometry. The rule is "project only what needs to read as vertical," because the goal is legibility, not correctness — players do not notice the inconsistency, and would notice if consistency were enforced and it read badly.

**Why the corridors are flat is a real technical constraint.** The Zelda-style projection needs somewhere to project *into*. Corridor walls are deliberately allowed to be **one cell thick**, so drawing a face at `y-1` and `y-2` would paint over the corridor above. Flat corridors are forced by the wall thickness the maze generator preserves, not an oversight.

**It also produced a gameplay win.** Flat, ambiguous corridors versus volumetric, solid-feeling rooms is a genuine perceptual difference: disorientation in the corridors, "this is a *place*" in the rooms. That serves the horror and should be kept.

### 15.7 The cliff footprint problem

This is the core rendering problem the overworld introduces, and it does not exist in the maze.

> **A cliff is a boundary *between* tiles — zero width in world space. But drawing it needs vertical screen space. Where does that space come from?**

It comes from world tiles. **The cliff face has a footprint**: the tiles immediately south of a plateau edge become impassable cliff-wall tiles. That is not a hack — real cliffs have footprints too — but it means the world must be *generated with the footprint accounted for*, exactly as the maze is generated with wall thickness accounted for.

**One detail makes this much cheaper than it first appears: only south-facing edges need a full face.** The camera looks from the south, so north, east and west plateau edges only need a thin edge-cap tile — a lip. The cost is roughly one row of tiles along the southern boundary of each plateau, not a border around everything.

**Three options:**

| Option | Description | Trade |
|---|---|---|
| **(1) No cliffs** | Height is continuous, everything walkable. Height only feeds hillshade, temperature and water flow. | Zero footprint problem. Keeps all visual relief, loses "terrain as barrier." Genuinely viable — for a survival game that is a much smaller loss than it would be for an adventure game. |
| **(2) Cliffs everywhere** | Quantise height into bands, face every band boundary. | Maximum drama, maximum complexity. Plateau shapes start dictating world layout. |
| **(3) Mostly (1), with rare deliberate cliffs** | Gentle terrain continuously walkable and hillshaded only. Cliff faces appear **only** where the gradient exceeds a high threshold — perhaps 5% of the world. | **Leaning strongly toward this.** Because cliffs are rare their footprint is never a constraint; there is always room. And rare cliffs are far more dramatic than ubiquitous ones. |

**Option 3 is structurally the same decision already made in the maze:** flat where the terrain is ambiguous and open, projected where it is solid and blocking. Consistency of *approach* is worth more here than consistency of projection.

### 15.8 Ramps and connectivity — an algorithm the project already has

If most terrain is walkable slope, ramps are unnecessary in the general case; you simply walk uphill. Ramps only matter where a hard cliff has been placed, and the guarantee needed there is **connectivity** — which is the same problem the maze generator already solves for disconnected islands:

1. Quantise height into bands; mark high-gradient boundaries as impassable cliff.
2. Flood-fill from the player's spawn.
3. Any region that comes back unreachable — find the lowest-gradient point on its boundary and carve a ramp there.

That is the **existing tunnel-borer BFS and connected-component pruning**, pointed at a different grid. Same guarantee, same code shape, and it reuses an algorithm already documented in the roadmap's algorithmic showcase.

### 15.9 Rendering elevation: hillshade, and the pixel-art trap

**Hillshade.** Because the terrain is generated, a continuous height field is available, so relief shading can be *computed* rather than hand-authored — the same technique GIS uses for topographic maps:

```
h = heightAt(x, y)
n = normalize(vec3(h(x-1,y) - h(x+1,y), h(x,y-1) - h(x,y+1), 2.0))
L = clamp(dot(n, sunDir), 0, 1)
```

Every hill, gully and ridge then shades itself with no additional art, and rotating `sunDir` with time of day re-lights the whole landscape — the same valley looks different at dawn and dusk. This is the answer to "3D gets elevation for free": so does this, just through shading instead of geometry.

**The trap: continuous lighting and pixel art fight each other.** Multiplying a smooth `0.0-1.0` shading term over sprites produces muddy in-between colours and destroys the hard edges that make pixel art read.

**The fix is posterisation.** Quantise the hillshade into 3-5 discrete steps and map each step to an actual colour on a defined palette ramp, rather than doing a continuous multiply. Slopes then shade in clean bands using colours an artist would have chosen, and everything stays on-palette. → search `pixel art palette ramp shading`, `posterized lighting pixel art`

This is a strong argument for doing a **palette unification pass first** — hillshading essentially requires a defined ramp to snap to. It is also the fix for the existing asset incoherence: the current sprite sheets come from several different artists with different palettes, and quantising them all to one shared ramp would do more for the game's look than any new art.

**Related rendering work, in rough order of payoff per line of code:**

- **Y-sorted render queue.** The current draw order is three fixed layers (maze → player → items in `playing_state.cpp` ~L432-435), so the player always draws over wall faces and items always draw over the player. That is fine for a tile maze; a forest needs trees, rocks, cliffs, the dog, animals and the player to interleave *with each other*. Requires collecting drawables into one queue and sorting by **base Y in world pixels** (not tile row, or entities pop between tiles). `O(k log k)`, or `O(k)` with a bucket sort since baseY is bounded by screen height. **Cheap now with three renderers, expensive after thirty entity types exist.**
- **8-bit autotiling.** The existing `tileMap[16]` is a 4-bit cardinal-only mask. Fine for solid walls; grass/sand/water/cliff borders need the corner-aware 47-tile "blob" set or diagonal transitions look wrong.
- **Weather** (rain, fog, snow, plus a fog-density uniform) — turns the same terrain into a visibly different place, multiplying apparent content, and plugs straight into seasons and temperature.
- **Wind sway** — offset the upper pixels of grass and tree sprites by `sin(time + worldPos)`. Roughly twenty lines, and the world stops feeling like a diorama.
- **Day/night colour ramp**, ambient particles (pollen, leaves, snow, fireflies), animated water with edge foam, per-biome colour grading.

### 15.10 A staged path, because this is a lot at once

The complexity is real, and the mitigation is that each stage stands alone and is independently shippable:

| Stage | Work | Payoff |
|---|---|---|
| **0** | Palette unification + atlas JSON (replace hardcoded source rects). | Unblocks everything else; fixes existing asset incoherence. |
| **1** | Height field + hillshade shader. No cliffs, nothing impassable. | Roughly a day's work, and **most of the visual payoff**. Could stop here permanently and still have a good-looking game. |
| **2** | Quantise height, south-facing cliff faces, impassability, connectivity check. | Terrain as barrier. |
| **3** | Ramps, then hydrology (flow accumulation → rivers). | Navigation, water as a resource, seasonal river behaviour. |
| **4** | Erosion, weather, day/night, wind. | Polish and apparent content. |

**Do Stage 1, look at it, and only then decide whether Stage 2 is worth it.** Hillshade alone may read as enough elevation.

Note also the split worth keeping in mind: **hydrology is gameplay, erosion is looks.** Flow accumulation alone gives rivers on un-eroded terrain, so hydrology can land well before erosion does.

### 15.11 Reference games

The combination described here — hydrology-driven procedural generation *plus* considered 2D pixel rendering — is genuinely uncommon. Most games have one or the other, which is a point in favour rather than a warning.

| Game | What to look at |
|---|---|
| **Dwarf Fortress** | The generation reference. Erosion, rain shadow, river networks flowing mountains-to-ocean with named tributaries and correct confluences. Rendered as flat tile slices. → `dwarf fortress world generation map` |
| **Unreal World** | The direct genre inspiration. Fixed finite world with hills, rivers, lakes and mires that structure travel, fishing and settlement. Very plain top-down tiles. |
| **Rise to Ruins** | Probably the closest single reference — 2D pixel art, procedurally generated terrain with real elevation, and water that actually flows downhill and pools. |
| **Songs of Syx** | Procgen world with mountains and elevation, 2D top-down. |
| **Stardew Valley** | The reference for *how to draw cliffs* in 2D 3/4 — discrete elevation levels, cliff faces, ramps. Study the cliff tiles specifically. Hand-authored maps. |
| **Zelda: A Link to the Past** | Where the cliff convention originates. Two elevation levels, ledges you can drop off but not climb. |
| **Necesse** | Closest genre and aesthetic sibling (2D top-down pixel survival, procgen biomes), though its terrain is essentially flat. |
| **Minecraft** | The counter-example. Infinite, so rivers are noise features that wander, dead-end and never connect to oceans consistently. Nobody navigates by them. → `minecraft river biome map overhead` |
| **Factorio** | Noise-blob water, and correctly so — water there is purely an obstacle. Useful reminder that hydrology only pays for itself when water is a *resource*, which here it is. |

Putting a Dwarf Fortress world map beside a Minecraft overworld map makes the difference immediate: one has drainage basins you can trace, the other has blue squiggles.

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
| **New: wilderness half** | Terrain/biome generation, seasons, weather, hunting, fishing, foraging, shelter building and maintenance, temperature — all new, and not yet phased. Generation and rendering approach explored in §15. |
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
- **World size.** ~8,000 tiles across (~33 min to cross) or ~16,000 (~67 min)? Larger costs nothing at generation time given the coarse/fine split (§15.2), but a world too large for one run to meaningfully explore wastes the erosion and hydrology work.
- **Cliffs: option (1) or option (3)?** (§15.7) If (3), what gradient threshold, and what fraction of the world should end up impassable? Worth deciding only after Stage 1 hillshade is on screen.
- **Mountain ring or coastline** at the world edge? (§15.4) Leaning mountain ring, but a coast adds a beach biome and sea fishing.
- **Is erosion worth implementing at all**, or does flow accumulation on un-eroded terrain look good enough in 2D? (§15.10) Erosion's value is largely visual, and much of it may not survive the top-down projection.
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
