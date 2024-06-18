# SaltyNX

Background process for the Nintendo Switch for file/code modification

Created by: https://github.com/shinyquagsire23

This fork includes many QoL improvements and beside plugins support also supports patches. 

Since 0.7.0 version NX-FPS and ReverseNX-RT are an intergral part of SaltyNX Core. This allows us to run them in 64-bit games not compatible with plugins.
To use ReverseNX-RT download newest overlay from [HERE](https://github.com/masagrator/ReverseNX-RT/releases)

![GitHub all releases](https://img.shields.io/github/downloads/masagrator/SaltyNX/total?style=for-the-badge)
---

Patches pattern:
- filename is symbol of function with filetype `.asm64`,
- inside file write with hex editor instructions that you want to overwrite for this function,
- put this file either to `SaltySD/patches` to make it work for every game, or to `SaltySD/patches/*titleid*` to make it work for specific game.

For additional functions you need SaltyNX-Tool

https://github.com/masagrator/SaltyNX-Tool

It is required to have FW installed at least 10.0.0 version

No technical support for:
- Atmosphere forks
- SX OS
- Kosmos
- ReinX

Known issues:
- Instability with some homebrews and sysmodules (like emuiibo),
- You need to have at least Hekate 5.0.2 if you don't want issues related to Hekate,
- 32 bit games are unsupported,
- Cheats using directly heap addresses may not work properly while using plugins.

# How to download release:

For Atmosphere >=0.10.1 just put folders from archive to root of your sdcard.

For Atmosphere <=0.9.4 and any other CFW rename `contents` folder to `titles`

For SX OS remember to rename `atmosphere` folder to `sxos`

Remember to restart Switch

---

# Thanks to
- `Cooler3D` for developing and releasing publicly first tool allowing changing refresh rate of display in HOS. Code for changing refresh rate in SaltyNX is based on reverse engineered code from his tool.

# List of titles not compatible with plugins/patches

| Title | plugins/all | Why? |
| ------------- | ------------- | ------------- |
| Alien: Isolation | plugins | Heap related |
| Azure Striker Gunvolt: Striker Pack | all | 32-bit game, not supported |
| Baldur's Gate and Baldur's Gate II: Enhanced Editions | all | 32-bit game, not supported |
| CelDamage HD | all | 32-bit game, not supported |
| Company of Heroes Collection | plugins | heap related |
| DEADLY PREMONITION Origins | all | 32-bit game, not supported |
| Dies irae Amantes amentes For Nintendo Switch | all | 32-bit game, not supported |
| Donkey Kong Country: Tropical Freeze | all | From second level heap issues |
| EA SPORTS FC 24 | plugins | heap related |
| Goat Simulator | all | 32-bit game, not supported |
| Gothic | all | 32-bit game, not supported |
| Grandia Collection | all | Only launcher is 64-bit, games are 32-bit |
| Grid: Autosport | plugins | Heap related |
| Immortals Fenyx Rising | plugins | Heap related |
| LIMBO | all | 32-bit game, not supported |
| Luigi's Mansion 3 | plugins | Heap related |
| Mario Kart 8 | all | 32-bit game, not supported |
| Mario Strikers: Battle League | plugins | Heap related |
| Megadimension Neptunia VII | all | 32-bit game, not supported |
| Moero Chronicle Hyper | all | 32-bit game, not supported |
| Moero Crystal H | all | 32-bit game, not supported |
| Monster Hunter Generations Ultimate | all | 32-bit game, not supported |
| New Super Mario Bros. U Deluxe | all | 32-bit game, not supported |
| Ni no Kuni: Wrath of the White Witch | all | 32-bit game, not supported |
| Olympic Games Tokyo 2020 – The Official Video Game™ | plugins | Heap related |
| Pikmin 3 Deluxe | all | 32-bit game, not supported |
| Planescape: Torment and Icewind Dale | all | 32-bit game, not supported |
| Plants vs. Zombies: Battle for Neighborville | plugins | Heap related |
| Radiant Silvergun | all | 32-bit game, not supported |
| Sherlock Holmes and The Hound of The Baskervilles | all | 32-bit game, not supported |
| Stubbs the Zombie in Rebel Without a Pulse | all | heap related |
| The Lara Croft Collection | plugins | heap related |
| Tokyo Mirage Session #FE Encore | all | 32-bit game, not supported |
| Valkyria Chronicles | all | 32-bit game, not supported |
| Witcher 3 GOTY (version 3.2) | all | heap related |
| World of Goo | all | 32-bit game, not supported |
| YouTube | plugins | Unknown |

Titles other than 32-bit are added to exceptions.txt which is treated as Black list, you can find it in root of repo. SaltyNX reads it from SaltySD folder. `X` at the beginning of titleid means that this game will not load any patches and plugins. `R` at the beginning of titleid means that this game will not load any patches and plugins if romfs mod for this game is installed.

32-bit games are ignored by default for patches and plugins.
