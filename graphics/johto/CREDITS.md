# Johto HGSS map and artwork sources

New Bark and Route 29 adapt the geographic layout of Pokémon HeartGold /
SoulSilver to this GBA engine. This is a 2D adaptation: the DS camera,
3D buildings, animation and height effects are not reproduced.

## Terrain and events

Reference: [pret/pokeheartgold](https://github.com/pret/pokeheartgold), commit
`6988158691eb6178cff697cda0428045ddcba5fb`.

The checked-in `tools/generations_maps/hgss_reference.json` records the
32×32 New Bark terrain and three 32×32 Route 29 sectors from `files/a/0/6/5`,
plus door positions from `057_T20.json` and `030_R29.json` under
`files/fielddata/eventdata/zone_event`. The route is 96×32; its east connection
joins New Bark's west side. Terrain includes the original path, grass,
trees, water and directional ledge placement. Buildings use the original
ground-floor entrances, with a separate exterior entrance to Elm's study.

## Community building artwork

Original artwork credited to **SirMalo** by the community sheet maintainer:
[HGSS for RMXP](https://www.deviantart.com/sirmaio/art/HGSS-for-RMXP-837574941).
RPG Maker XP arrangement by **jls-dchrn**:
[HGSS_pokemon_tileset](https://github.com/jls-dchrn/HGSS_pokemon_tileset),
commit `905308eb7151f664da66bd96705ff3a45c0a7a21`.

The three `source/*.png` files are crops of
`Buildings/Buildings_hgss_transparent.png`. Crop rectangles and the source
sheet's SHA-256 are recorded in the reference JSON. The generator downsamples
these doubled sprites to native resolution, removes the yellow background,
recolors shadows and quantizes each facade into a GBA palette. The gate
reuses the lab facade within the secondary tileset's 384-tile budget.
The source repository supplies no license file; these credits do not assert
a new license for its artwork. Pokémon artwork belongs to its respective
rights holders.

Terrain tile patterns and interior furniture come from this repository's
existing FRLG tilesets. The three wind turbine sprites are drawn directly
by the generator for this adaptation. Interiors are newly composed layouts.

## Rebuilding

Run from the repository root with Python 3 and Pillow:

```sh
python3 tools/generations_maps/generate.py
python3 tools/generations_maps/generate.py --check
python3 tools/generations_maps/generate.py --preview /tmp/johto-maps
python3 tools/generations_maps/validate.py
```

Normal ROM builds need neither Pillow nor downloads: generated exterior
maps, palette files, metatiles and tile images are checked in. Interiors
are edited as their own binary layouts. Validation uses only Python's
standard library and runs in the test ROM build Action.
