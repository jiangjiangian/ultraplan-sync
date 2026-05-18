# Character Sprite Attribution

The 31 PNGs under `school_uniform_3/` (18 female + 13 male) are derived from
**PIPOYA FREE RPG Character Sprites 32x32**, pack subfolder
*Japanese school characters / school uniform 3*.

- Author: **Pipoya**
- Source: https://pipoya.itch.io/pipoya-free-rpg-character-sprites-32x32
- License (per pack page): free for commercial and personal use, free to edit;
  **may not be redistributed or resold as standalone assets**.

Each sprite is a 96x128 PNG laid out as 3 columns (walk frames) x 4 rows
(directions: down, left, right, up). Filenames were normalised from the
original `su3 Student fmale NN.png` / `su3 Student male NN.png` pattern to
`female_NN.png` / `male_NN.png` so runtime paths stay free of spaces.

The 3 NPC sprites under `npc/` come from the same Pipoya pack, picked from
the Female and Male subfolders as role anchors:

| File                  | Source                          | NPC role        |
| --------------------- | ------------------------------- | --------------- |
| `npc/suit_senior.png` | `Male/Male 01-1.png`           | 西裝學長        |
| `npc/ta.png`          | `Male/Male 05-1.png`           | 助教            |
| `npc/shop_auntie.png` | `Female/Female 15-1.png`       | 福利社阿姨      |

The other two NPC archetypes reuse sheets already curated under
`school_uniform_3/`: `female_03.png` for 學霸 and `male_02.png` for 苦主.
